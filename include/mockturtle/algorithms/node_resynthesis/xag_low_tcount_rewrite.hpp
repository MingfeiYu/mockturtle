#pragma once

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
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/utils/stopwatch.hpp>
#include <mockturtle/views/cut_view.hpp>

namespace mockturtle
{

struct xag_low_tcount_rewrite_params
{
	bool verbose{ false };
	bool verify_database{ false };
	uint32_t exhaustive_dc_limit{ 10u };
};

struct xag_low_tcount_rewrite_stats
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

class xag_low_tcount_rewrite
{
public:
	xag_low_tcount_rewrite( std::string const& dbname, xag_low_tcount_rewrite_params const& ps = {}, 
													xag_low_tcount_rewrite_stats* pst = nullptr )
		: ps_( ps ),
			pst_( pst ),
			pfunc_tcount_( std::make_shared<decltype( pfunc_tcount_ )::element_type>() ),
      pclassify_cache_( std::make_shared<decltype( pclassify_cache_ )::element_type>() )
	{
		build_db( dbname );
	}

	virtual ~xag_low_tcount_rewrite()
	{
		if ( ps_.verbose )
		{
			pst_->report();
		}
	}

	template<typename LeavesIterator, typename Fn>
	void operator()( xag_network& xag, kitty::dynamic_truth_table func, kitty::dynamic_truth_table const& dc,
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
					( *this )( xag, func, begin, end, fn );
				}
			}
			else
			{
				kitty::for_each_one_bit( dc, [&]( auto bit ) {
					kitty::flip_bit( func, bit );
					( *this )( xag, func, begin, end, fn );
					kitty::flip_bit( func, bit );
				} );
				( *this )( xag, func, begin, end, fn );
			}
		}
		else
		{
			( *this )( xag, func, begin, end, fn );
		}
	}

	template<class TT, typename LeavesIterator, typename Fn>
	void operator()( xag_network& xag, TT const& func,
									 LeavesIterator begin, LeavesIterator end, Fn&& fn, bool rewrite = true ) const
	{
		stopwatch t_total( pst_->time_total );

		const auto func_ext = kitty::extend_to<5u>( func );
		std::vector<kitty::detail::spectral_operation> trans;
		kitty::static_truth_table<5u> real_repr;
		uint32_t tcount{ 0u };

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
			//real_repr = std::get<0>( cache_it->second );
			//trans = std::get<1>( cache_it->second );
		}
		else
		{
			pst_->cache_miss++;
			const auto spectral = call_with_stopwatch( pst_->time_classify, [&]() {
					return kitty::exact_spectral_canonization_limit( func_ext, 100000, [&trans]( auto const& ops ) {
					std::copy( ops.begin(), ops.end(), std::back_inserter( trans ) );
				} );
			} );
			//const auto real_repr = call_with_stopwatch( pst_->time_classify, [&]() {
			//		return kitty::exact_spectral_canonization( func_ext, [&trans]( auto const& ops ) {
			//		std::copy( ops.begin(), ops.end(), std::back_inserter( trans ) );
			//	} );
			//} );
			
			//pclassify_cache_->insert( std::make_pair( func_ext, std::make_tuple( real_repr, trans ) ) );
			pclassify_cache_->insert( std::make_pair( func_ext, std::make_tuple( spectral.second, spectral.first, trans ) ) );
			real_repr = spectral.first;

			if ( !spectral.second )
			{
				pst_->classify_abort++;
				return;
			}
		}

		auto search = pfunc_tcount_->find( kitty::to_hex( real_repr ) );
		xag_network::signal po_db_repr;
		if ( search != pfunc_tcount_->end() )
		{
			std::string db_repr_str = std::get<0>( search->second );
			tcount = std::get<1>( search->second );
			if ( !rewrite )
			{
				fn( xag.get_constant( false ), tcount );
				return;
			}
			po_db_repr = std::get<2>( search->second );

			kitty::static_truth_table<5u> db_repr;
			kitty::create_from_hex_string( db_repr, db_repr_str );

			call_with_stopwatch( pst_->time_classify, [&]() {
				return kitty::exact_spectral_canonization( db_repr, [&trans]( auto const& ops ) {
					std::copy( ops.rbegin(), ops.rend(), std::back_inserter( trans ) );
				} );
			} );
		}
		/*
		else if ( kitty::is_const0( real_repr ) )
		{
			if ( !kitty::is_const0( func_ext ) )
			{
				bool po_inv{ false };
				std::vector<xag_network::signal> pis( 5u, xag.get_constant( false ) );
				std::copy( begin, end, pis.begin() );
				std::unordered_map<xag_network::node, bool> record;
				for ( auto const& pi: pis )
				{
					// Bool indicates if the signal shall be considered or not 
					record.insert( std::make_pair( pi.index, false ) );
				}

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
						auto entry1 = record.find( pis[v1].index );
						entry1->second = true;
						auto entry2 = record.find( pis[v2].index );
						entry2->second = true;
						break;
					}
					case kitty::detail::spectral_operation::kind::disjoint_translation:
					{
						const auto v1 = log2( op._var1 );
						auto entry = record.find( pis[v1].index );
						entry->second = true;
						break;
					}
					default:
						abort();
					}
				}

				//std::cout << "[m] function 0x";
				//kitty::print_hex( func_ext );
				//std::cout << " is the X" << ( po_inv ? "N" : "" ) << "ORed result of signals: ";

				std::vector<xag_network::signal> po_xors;
				for ( auto const& pi: pis )
				{
					auto entry = record.find( pi.index );
					if ( entry->second )
					{
						po_xors.emplace_back( pi );
						//std::cout << ( xag.is_complemented( pi ) ? "!" : "" ) << pi.index << " ";
					}
				}
				//std::cout << "\n";

				xag_network::signal po = xag.create_nary_xor( po_xors );
				fn( ( po_inv ? !po : po ), 0u );
				return;
			}
			else
			{
				po_db_repr = db_.get_constant( false );
			}
		}
		*/
		else
		{
			pst_->unknown_func_abort++;
			return;
		}

		bool po_inv{ false };
		std::vector<xag_network::signal> po_xors;
		std::vector<xag_network::signal> pis( 5u, xag.get_constant( false ) );
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
				pis[v1] = xag.create_xor( pis[v1], pis[v2] );
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

		xag_network::signal po;
		if ( db_.is_constant( db_.get_node( po_db_repr ) ) )
		{
			po = xag.get_constant( db_.is_complemented( po_db_repr ) );
		}
		else
		{
			cut_view<xag_network> db_partial{ db_, db_pis_, po_db_repr };
			po = cleanup_dangling( db_partial, xag, pis.begin(), pis.end() ).front();
		}

		for ( auto const& po_xor: po_xors )
		{
			po = xag.create_xor( po, po_xor );
		}

		//std::cout << "[m] cost of current cut is " << tcount << "\n";
		fn( ( po_inv ? !po : po ), tcount );
		//fn( po_inv ? !po : po );
	}

private:
	void build_db( std::string const& dbname )
	{
		stopwatch t_total( pst_->time_total );
		stopwatch t_parse_db( pst_->time_parse_db );

		db_pis_.resize( 5u );
		std::generate( db_pis_.begin(), db_pis_.end(), [&]() { return db_.create_pi(); } );

		std::ifstream db;
		db.open( dbname, std::ios::in );
		std::string line;
		uint32_t pos{ 0u };

		while ( std::getline( db, line ) )
		{
			pos = static_cast<uint32_t>( line.find( 'x' ) );
			const std::string repr_real_str = line.substr( ++pos, 8u );
			pos += 9u;
			line.erase( 0, pos );
			pos = line.find( 'x' );
			std::string repr_db_str = line.substr( ++pos, 8u );
			pos += 9u;
			line.erase( 0, pos );
			pos = line.find( ' ' );
			const uint32_t num_vars = std::stoul( line.substr( 0, pos++ ) );
			line.erase( 0, pos );
			pos = line.find( ' ' );
			const uint32_t tcount = std::stoul( line.substr( 0, pos++ ) );
			line.erase( 0, pos );

			std::vector<xag_network::signal> signals_tcount_opt( db_pis_.begin(), db_pis_.begin() + num_vars );

			while ( line.size() > 3 )
			{
				uint32_t signal_1, signal_2;
				pos = line.find( ' ' );
				signal_1 = std::stoul( line.substr( 0, pos++ ) );
				line.erase( 0, pos );
				pos = line.find( ' ' );
				signal_2 = std::stoul( line.substr( 0, pos++ ) );
				line.erase( 0, pos );

				xag_network::signal signal1, signal2;
	    	if ( signal_1 == 0u )
	    	{
					signal1 = db_.get_constant( false );
	    	}
	    	else if ( signal_1 == 1u )
	    	{
					signal1 = db_.get_constant( true );
	    	}
	    	else
	    	{
	    		signal1 = signals_tcount_opt[signal_1 / 2 - 1] ^ ( signal_1 % 2 != 0 );
	    	}
	    	if ( signal_2 == 0u )
	    	{
					signal2 = db_.get_constant( false );
	    	}
	    	else if ( signal_2 == 1u )
	    	{
					signal2 = db_.get_constant( true );
	    	}
	    	else
	    	{
	    		signal2 = signals_tcount_opt[signal_2 / 2 - 1] ^ ( signal_2 % 2 != 0 );
	    	}

	    	if ( signal_1 > signal_2 )
	    	{
	    		signals_tcount_opt.emplace_back( db_.create_xor( signal1, signal2 ) );
	    	}
	    	else
	    	{
	    		signals_tcount_opt.emplace_back( db_.create_and( signal1, signal2 ) );
	    	}
			}

			const uint32_t signal_po = std::stoul( line );
			const xag_network::signal po = signals_tcount_opt[signal_po / 2 - 1] ^ ( signal_po % 2 != 0 );
			db_.create_po( po );

			if ( ps_.verify_database )
			{
				cut_view<xag_network> db_partial{ db_, db_pis_, po };
				kitty::static_truth_table<5u> repr_real, repr_db;
				kitty::create_from_hex_string( repr_real, repr_real_str );
				kitty::create_from_hex_string( repr_db, repr_db_str );

				auto result = simulate<kitty::static_truth_table<5u>>( db_partial )[0];
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

			pfunc_tcount_->insert( std::make_pair( repr_real_str, std::make_tuple( repr_db_str, tcount, po ) ) );
		}
		std::cout << "[i] load db finished\n";
		std::cout << "[i] " << ( db_._storage )->nodes.size() << " nodes in the database XAG\n";
	}

private:
	xag_network db_;
	std::vector<xag_network::signal> db_pis_;
	std::shared_ptr<std::unordered_map<std::string, std::tuple<std::string, uint32_t, xag_network::signal>>> pfunc_tcount_;
	std::shared_ptr<std::unordered_map<kitty::static_truth_table<5u>, std::tuple<bool, kitty::static_truth_table<5u>, std::vector<kitty::detail::spectral_operation>>, kitty::hash<kitty::static_truth_table<5u>>>> pclassify_cache_;
	//std::shared_ptr<std::unordered_map<kitty::static_truth_table<5u>, std::tuple<kitty::static_truth_table<5u>, std::vector<kitty::detail::spectral_operation>>, kitty::hash<kitty::static_truth_table<5u>>>> pclassify_cache_;

	xag_low_tcount_rewrite_params const& ps_;
	//xag_low_tcount_rewrite_stats st_;
	xag_low_tcount_rewrite_stats* pst_{ nullptr };
};

} /* namespace mockturtle */