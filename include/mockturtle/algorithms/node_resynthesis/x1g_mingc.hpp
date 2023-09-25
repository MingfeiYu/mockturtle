#pragma once

#include <algorithm>
#include <cstdint>
#include <fmt/format.h>
#include <fstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <kitty/bit_operations.hpp>
#include <kitty/constructors.hpp>
#include <kitty/hash.hpp>
#include <kitty/operations.hpp>
#include <kitty/print.hpp>
#include <kitty/spectral.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/x1g.hpp>
#include <mockturtle/utils/stopwatch.hpp>
#include <mockturtle/views/cut_view.hpp>

namespace mockturtle
{

struct x1g_mingc_rewrite_params
{
	bool verbose{ false };
	bool verify_database{ false };
	uint32_t exhaustive_dc_limit{ 10u };
};

struct x1g_mingc_rewrite_stats
{
	stopwatch<>::duration time_total{ 0 };
	stopwatch<>::duration time_parse_db{ 0 };
	stopwatch<>::duration time_classify{ 0 };
	stopwatch<>::duration time_construct{ 0 };
	uint32_t cache_hit{ 0u };
	uint32_t cache_miss{ 0u };
	uint32_t classify_abort{ 0u };
	uint32_t unknown_func_abort{ 0u };
	uint32_t dont_cares{ 0u };

	void report() const
	{
		std::cout << fmt::format( "[i] total time     = {:>5.2f} secs\n", to_seconds( time_total ) );
    std::cout << fmt::format( "[i] parse db time  = {:>5.2f} secs\n", to_seconds( time_parse_db ) );
    std::cout << fmt::format( "[i] classify time  = {:>5.2f} secs\n", to_seconds( time_classify ) );
    std::cout << fmt::format( "[i] - aborts       = {:>5}\n", classify_abort );
    std::cout << fmt::format( "[i] construct time = {:>5.2f} secs\n", to_seconds( time_construct ) );
    std::cout << fmt::format( "[i] cache hits     = {:>5}\n", cache_hit );
    std::cout << fmt::format( "[i] cache misses   = {:>5}\n", cache_miss );
    std::cout << fmt::format( "[i] unknown func.  = {:>5}\n", unknown_func_abort );
    std::cout << fmt::format( "[i] don't cares    = {:>5}\n", dont_cares );
	}
};

class x1g_mingc_rewrite
{
public:
	x1g_mingc_rewrite( std::string const& dbname, x1g_mingc_rewrite_params const& ps = {}, 
	                   x1g_mingc_rewrite_stats* pst = nullptr )
		: ps_( ps ),
		  pst_( pst ), 
		  pfunc_gc_( std::make_shared<decltype( pfunc_gc_ )::element_type>() ), 
		  pclassify_cache_( std::make_shared<decltype( pclassify_cache_ )::element_type>() ), 
		  pblacklist_cache_( std::make_shared<decltype( pblacklist_cache_ )::element_type>() )
	{
		fmt::print( "[i] started reading in database\n" );
		build_db( dbname );
		fmt::print( "[i] finished reading in database ( size : {} )\n", db_.num_pos() );
	}

	virtual ~x1g_mingc_rewrite()
	{
		if ( ps_.verbose )
		{
			pst_->report();
		}
	}

	template<typename LeavesIterator, typename Fn>
	void operator()( x1g_network& x1g, kitty::dynamic_truth_table func, kitty::dynamic_truth_table const& dc, 
	                 LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
	{
		if ( !kitty::is_const0( dc ) )
		{
			const auto num_dc_bit = kitty::count_ones( dc );
			pst_->dont_cares += num_dc_bit;

			if ( num_dc_bit <= ps_.exhaustive_dc_limit )
			{
				std::vector<uint8_t> dc_bits;
				kitty::for_each_one_bit( dc, [&]( auto bit ) {
					dc_bits.emplace_back( bit );
					kitty::clear_bit( func, bit );
				} );

				for ( auto i{ 0u }; i < ( 1u << dc_bits.size() ); ++i )
				{
					for ( auto j{ 0u }; j < dc_bits.size(); ++j )
					{
						if ( ( i >> j ) & 1 )
						{
							kitty::set_bit( func, dc_bits[j] );
						}
						else
						{
							kitty::clear_bit( func, dc_bits[j] );
						}
					}
					( *this )( x1g, func, begin, end, fn );
				}
			}
			else
			{
				kitty::for_each_one_bit( dc, [&]( auto bit ) {
					kitty::flip_bit( func, bit );
					( *this )( x1g, func, begin, end, fn );
					kitty::flip_bit( func, bit );
				} );
				( *this )( x1g, func, begin, end, fn );
			}
		}
		else
		{
			( *this )( x1g, func, begin, end, fn );
		}
	}

	template<class TT, typename LeavesIterator, typename Fn>
	void operator()( x1g_network& x1g, TT const& func, 
	                 LeavesIterator begin, LeavesIterator end, Fn&& fn, bool rewrite = true ) const
	{
		stopwatch t_total( pst_->time_total );

		const auto func_ext = kitty::extend_to<6u>( func );
		std::vector<kitty::detail::spectral_operation> trans;
		kitty::static_truth_table<6u> real_repr;

		const auto cache_it = pclassify_cache_->find( func_ext );
		if ( cache_it != pclassify_cache_->end() )
		{
			pst_->cache_hit++;
			if ( !std::get<0>( cache_it->second ) )
			{
				return;
			}
			real_repr = std::get<1>( cache_it->second );
			trans = std::get<2>( cache_it->second );
		}
		else
		{
			if ( !pblacklist_cache_->empty() )
			{
				const auto blacklist_it = std::find( pblacklist_cache_->begin(), pblacklist_cache_->end(), func_ext );
				if ( blacklist_it != pblacklist_cache_->end() )
				{
					pst_->classify_abort++;
					return;
				}
			}

			pst_->cache_miss++;
			const auto spectral = call_with_stopwatch( pst_->time_classify, [&]() {
					return kitty::exact_spectral_canonization_limit( func_ext, 100000, [&trans]( auto const& ops ) {
					std::copy( ops.begin(), ops.end(), std::back_inserter( trans ) );
				} );
			} );
			
			pclassify_cache_->insert( std::make_pair( func_ext, std::make_tuple( spectral.second, spectral.first, trans ) ) );
			real_repr = spectral.first;

			if ( !spectral.second )
			{
				pblacklist_cache_->emplace_back( func_ext );
				pst_->classify_abort++;
				return;
			}
		}

		auto search = pfunc_gc_->find( kitty::to_hex( real_repr ) );
		x1g_network::signal po_db_repr;
		if ( search != pfunc_gc_->end() )
		{
			uint8_t gc = std::get<0>( search->second );
			std::string db_repr_str = std::get<1>( search->second );
			po_db_repr = std::get<2>( search->second );

			if ( !rewrite )
			{
				fn( db_.get_constant( false ), gc / 2 );
				return;
			}

			kitty::static_truth_table<6u> db_repr;
			kitty::create_from_hex_string( db_repr, db_repr_str );

			call_with_stopwatch( pst_->time_classify, [&]() {
				return kitty::exact_spectral_canonization( db_repr, [&trans]( auto const& ops ) {
					std::copy( ops.rbegin(), ops.rend(), std::back_inserter( trans ) );
				} );
			} );
		}
		else if ( kitty::is_const0( real_repr ) )
		{
			po_db_repr = db_.get_constant( false );
		}
		else
		{
			pst_->unknown_func_abort++;
			//fmt::print( "[e] {} is an unknown function\n", kitty::to_hex( real_repr ) );
			return;
		}

		bool po_inv{ false };
		std::vector<x1g_network::signal> po_xors;
		std::vector<x1g_network::signal> pis( 6u, x1g.get_constant( false ) );
		std::copy( begin, end, pis.begin() );
		stopwatch t_construct( pst_->time_construct );

		for ( auto const& op: trans )
		{
			switch ( op._kind )
			{
			case kitty::detail::spectral_operation::kind::permutation:
			{	
				const auto v1 = log2( op._var1 );
				const auto v2 = log2( op._var2 );
				std::swap( pis[v1], pis[v2] );
				break;
			}
			case kitty::detail::spectral_operation::kind::input_negation:
			{	
				const auto v1 = log2( op._var1 );
				pis[v1] = !pis[v1];
				break;
			}
			case kitty::detail::spectral_operation::kind::output_negation:
			{	
				po_inv = !po_inv;
				break;
			}
			case kitty::detail::spectral_operation::kind::spectral_translation:
			{
				const auto v1 = log2( op._var1 );
				const auto v2 = log2( op._var2 );
				pis[v1] = x1g.create_xor( pis[v1], pis[v2] );
				break;
			}
			case kitty::detail::spectral_operation::kind::disjoint_translation:
			{
				const auto v1 = log2( op._var1 );
				po_xors.emplace_back( pis[v1] );
				break;
			}
			default:
				abort();
			}
		}

		x1g_network::signal po;
		if ( db_.is_constant( db_.get_node( po_db_repr ) ) )
		{
			po = x1g.get_constant( db_.is_complemented( po_db_repr ) );
		}
		else
		{
			cut_view<x1g_network> db_partial{ db_, db_pis_, po_db_repr };
			po = cleanup_dangling( db_partial, x1g, pis.begin(), pis.end() ).front();
		}

		for ( auto const& po_xor: po_xors )
		{
			po = x1g.create_xor( po, po_xor );
		}

		fn( ( po_inv ? !po : po ), 0u );
	}

private:
	void build_db( std::string const& dbname )
	{
		stopwatch t_total( pst_->time_total );
		stopwatch t_parse_db( pst_->time_parse_db );

		db_pis_.resize( 6u );
		std::generate( db_pis_.begin(), db_pis_.end(), [&]() { return db_.create_pi(); } );

		std::ifstream db_gc;
		db_gc.open( dbname, std::ios::in );
		std::string line;
		uint32_t pos{ 0u };;

		while ( std::getline( db_gc, line ) )
		{
			pos = static_cast<uint32_t>( line.find( 'x' ) );
			const std::string repr_real_str = line.substr( ++pos, 16u );
			pos += 17u;
			line.erase( 0, pos );
			pos = line.find( 'x' );
			std::string repr_db_str = line.substr( ++pos, 16u );
			pos += 17u;
			line.erase( 0, pos );
			pos = line.find( ' ' );
			const uint8_t num_vars = std::stoul( line.substr( 0, pos++ ) );
			line.erase( 0, pos );
			pos = line.find( ' ' );
			const uint8_t gc = std::stoul( line.substr( 0, pos++ ) );
			line.erase( 0, pos );

			std::vector<x1g_network::signal> signals_gc_opt( db_pis_.begin(), db_pis_.begin() + num_vars );

			while ( line.size() > 3 )
			{
				std::vector<x1g_network::signal> children;
				for ( auto child{ 0u }; child < 3u; ++child )
				{
					uint32_t signal_data;
					pos = line.find( ' ' );
					signal_data = std::stoul( line.substr( 0, pos++ ) );
					line.erase( 0, pos );

					x1g_network::signal signal_;
					if ( signal_data == 0u )
		    	{
						signal_ = db_.get_constant( false );
		    	}
		    	else if ( signal_data == 1u )
		    	{
						signal_ = db_.get_constant( true );
		    	}
		    	else
		    	{
		    		signal_ = signals_gc_opt[signal_data / 2 - 1] ^ ( signal_data % 2 != 0 );
		    	}
		    	children.emplace_back( signal_ );
				}

				if ( children[0].index > children[1].index )
				{
					signals_gc_opt.emplace_back( db_.create_xor3( children[0], children[1], children[2] ) );
				}
				else
				{
					signals_gc_opt.emplace_back( db_.create_onehot( children[0], children[1], children[2] ) );
				}
			}

			const uint32_t signal_data_po = std::stoul( line );
			const x1g_network::signal signal_po = signals_gc_opt[signal_data_po / 2 - 1] ^ ( signal_data_po % 2 != 0 );
			db_.create_po( signal_po );

			if ( ps_.verify_database )
			{
				cut_view<x1g_network> db_partial{ db_, db_pis_, signal_po };
				kitty::static_truth_table<6u> repr_real, repr_db;
				kitty::create_from_hex_string( repr_real, repr_real_str );
				kitty::create_from_hex_string( repr_db, repr_db_str );

				auto result = simulate<kitty::static_truth_table<6u>>( db_partial )[0];
				if ( result != repr_db )
				{
					std::cerr << "[w] incorrect implementation for " << repr_db_str << "; "
										<< "whose function is " << kitty::to_hex( result ) << std::endl;
					repr_db_str = kitty::to_hex( result );

					const auto repr_correct = kitty::exact_spectral_canonization( result );
					if ( repr_correct != repr_real )
					{
						std::cerr << "[e] representative does not match\n";
						abort();
					}
				}
			}

			pfunc_gc_->insert( std::make_pair( repr_real_str, std::make_tuple( gc, repr_db_str, signal_po ) ) );
		}
	}

private:
	x1g_network db_;
	std::vector<x1g_network::signal> db_pis_;
	std::shared_ptr<std::unordered_map<std::string, std::tuple<uint8_t, std::string, x1g_network::signal>>> pfunc_gc_;
	std::shared_ptr<std::unordered_map<kitty::static_truth_table<6u>, std::tuple<bool, kitty::static_truth_table<6u>, std::vector<kitty::detail::spectral_operation>>, kitty::hash<kitty::static_truth_table<6u>>>> pclassify_cache_;
	std::shared_ptr<std::vector<kitty::static_truth_table<6u>>> pblacklist_cache_;

	x1g_mingc_rewrite_params const& ps_;
	x1g_mingc_rewrite_stats* pst_{ nullptr };
};

} /* namespace mockturtle */