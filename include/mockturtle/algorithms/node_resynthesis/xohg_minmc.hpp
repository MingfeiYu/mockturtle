#pragma once

#include <unordered_map>
#include <vector>
#include <iostream>
#include <string>
#include <fstream>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/hash.hpp>
#include <kitty/spectral.hpp>

#include "../../networks/x1g.hpp"
#include "../../utils/include/percy.hpp"
#include "../../utils/stopwatch.hpp"

namespace mockturtle
{

struct exact_xohg_resynthesis_minmc_params
{
	using cache_map_t = std::unordered_map<kitty::dynamic_truth_table, percy::chain_minmc, kitty::hash<kitty::dynamic_truth_table>>;
	using cache_t = std::shared_ptr<cache_map_t>;
	using blacklist_cache_map_t = std::unordered_map<kitty::dynamic_truth_table, uint32_t, kitty::hash<kitty::dynamic_truth_table>>;
	using blacklist_cache_t = std::shared_ptr<blacklist_cache_map_t>;

	cache_t cache;
	blacklist_cache_t blacklist_cache;

	bool use_contribution_clauses{ true };
	uint32_t conflict_limit{ 0 };

	bool print_stats{ false };
};

struct exact_xohg_resynthesis_minmc_stats
{
	stopwatch<>::duration time_total{ 0 };
	stopwatch<>::duration time_parse_db{ 0 };
	int64_t time_exact_synth_success{ 0 };
	int64_t time_exact_synth_fail{ 0 };
	std::vector<uint32_t> failures = std::vector<uint32_t> (4, 0u);

	void report( exact_xohg_resynthesis_minmc_params const& ps ) const
	{
		std::cout << fmt::format( "[i] total time                = {:>5.2f} secs", to_seconds( time_total ) ) << std::endl;
		std::cout << fmt::format( "[i] parse db time             = {:>5.2f} secs", to_seconds( time_parse_db ) ) << std::endl;
		std::cout << fmt::format( "[i] exact synth time ( suc. ) = {:>5.2f} secs", ( time_exact_synth_success * 1.0 / 1000000 ) ) << std::endl;
		std::cout << fmt::format( "[i] exact synth time ( fail ) = {:>5.2f} secs", ( time_exact_synth_fail * 1.0 / 1000000 ) ) << std::endl;
		std::cout << fmt::format( "[i] #functions in the cache   = {:>5}", ( ps.cache->size() ) ) << std::endl;
		std::cout << fmt::format( "[i] #functions in black list  = {:>5}", ( ps.blacklist_cache->size() ) ) << std::endl;
		std::cout << fmt::format( "[i] #failed exact synthesis   = {:>5}", ( failures.at( 0 ) ) ) << std::endl;
		std::cout << fmt::format( "[i] #timeout exact synthesis  = {:>5}", ( failures.at( 1 ) ) ) << std::endl;
		std::cout << fmt::format( "[i] #failed repr. calculation = {:>5}", ( failures.at( 2 ) ) ) << std::endl;
		std::cout << fmt::format( "[i] #failed database matching = {:>5}", ( failures.at( 3 ) ) ) << std::endl;
	}
};

template<class Ntk = x1g_network>
class exact_xohg_resynthesis_minmc
{
public:
	using signal = typename Ntk::signal;

	enum class failure_type: uint8_t
	{
		exact_synth_fail,
		exact_synth_conflict_fail,
		compute_repr_fail,
		match_db_fail
	};

public:
	explicit exact_xohg_resynthesis_minmc( std::string const& db_filename, std::string const& dir_prefix = "", exact_xohg_resynthesis_minmc_params const& ps = {}, exact_xohg_resynthesis_minmc_stats* pst = nullptr, bool use_db = true )
			: _ps( ps ), _pst( pst ), func_mc( std::make_shared<std::unordered_map<std::string, uint32_t>>() ), _use_db( use_db ), _dir_prefix( dir_prefix )
	{
		if ( _use_db )
		{
			build_db( db_filename );
		}
	}

	~exact_xohg_resynthesis_minmc()
	{
		if ( _ps.print_stats )
		{
			_st.report( _ps );
		}

		if ( _pst )
		{
			*_pst = _st;
		}
	}

	template<typename LeavesIterator, typename Fn>
	void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, kitty::dynamic_truth_table const& dont_cares, LeavesIterator begin, LeavesIterator end, Fn const& fn ) const
	{
		auto const tt = function.num_vars() < 3u ? kitty::extend_to( function, 3u ) : function;
		auto const tt_dc = dont_cares.num_vars() < 3u ? kitty::extend_to( dont_cares, 3u ) : dont_cares;
		bool const normal = kitty::is_normal( tt );

		percy::spec_minmc spec;
		spec.nr_in = tt.num_vars();
		spec.fanin_size = 3u;
		spec.verbosity = 0u;
		spec.use_contribution_clauses = _ps.use_contribution_clauses;
		spec.conflict_limit = _ps.conflict_limit;
		spec.set_output( normal ? tt : ~tt );

		if ( int triv = spec.check_triv(); triv >= 0 )
		{
			auto const output_signal = ( triv == 0 ) ? ntk.get_constant( false ) : 
																								 *( begin + ( triv - 1 ) );
			fn( normal ? output_signal : !output_signal );

			return;
		}

		bool with_dc = false ;
		if ( !kitty::is_const0( tt_dc ) )
		{
			spec.set_dont_care( tt_dc );
			with_dc = true;
		}

		kitty::dynamic_truth_table const0{ 3 };
		kitty::dynamic_truth_table a{ 3 };
		kitty::dynamic_truth_table b{ 3 };
		kitty::dynamic_truth_table c{ 3 };
		kitty::create_nth_var( a, 0 );
		kitty::create_nth_var( b, 1 );
		kitty::create_nth_var( c, 2 );
		spec.add_free_primitive( const0 );													        // 00
		spec.add_free_primitive( a );													              // aa
		spec.add_free_primitive( b );													              // cc
		spec.add_free_primitive( c );													              // f0

		spec.add_primitive(  kitty::ternary_onehot(  a,  b,  c ) );         // 16
		spec.add_primitive( ~kitty::ternary_onehot( ~a,  b,  c ) );         // d6
		spec.add_primitive( ~kitty::ternary_onehot(  a, ~b,  c ) );         // b6
		spec.add_primitive( ~kitty::ternary_onehot(  a, b,  ~c ) );         // 9e
		spec.add_primitive(  kitty::ternary_onehot( ~a, ~b,  c ) );         // 86
		spec.add_primitive(  kitty::ternary_onehot(  a, ~b, ~c ) );         // 94
		spec.add_primitive(  kitty::ternary_onehot( ~a,  b, ~c ) );         // 92
		spec.add_primitive(  kitty::ternary_onehot( ~a, ~b, ~c ) );         // 68

		//spec.add_primitive(  kitty::ternary_onehot(  const0,  b,  c ) );    // 3c
		spec.add_primitive( ~kitty::ternary_onehot( ~const0,  b,  c ) );    // fc
		spec.add_primitive(  kitty::ternary_onehot( ~const0, ~b,  c ) );    // 0c
		spec.add_primitive(  kitty::ternary_onehot( ~const0,  b, ~c ) );    // 30
		spec.add_primitive(  kitty::ternary_onehot( ~const0, ~b, ~c ) );    // c0
		//spec.add_primitive(  kitty::ternary_onehot(  a,  const0,  c ) );    // 5a
		spec.add_primitive( ~kitty::ternary_onehot(  a, ~const0,  c ) );    // fa
		spec.add_primitive(  kitty::ternary_onehot( ~a, ~const0,  c ) );    // 0a
		spec.add_primitive(  kitty::ternary_onehot(  a, ~const0, ~c ) );    // 50
		spec.add_primitive(  kitty::ternary_onehot( ~a, ~const0, ~c ) );    // a0
		//spec.add_primitive(  kitty::ternary_onehot(  a,  b,  const0 ) );    // 66
		spec.add_primitive( ~kitty::ternary_onehot(  a,  b, ~const0 ) );    // ee
		spec.add_primitive(  kitty::ternary_onehot( ~a,  b, ~const0 ) );    // 22
		spec.add_primitive(  kitty::ternary_onehot(  a, ~b, ~const0 ) );    // 44
		spec.add_primitive(  kitty::ternary_onehot( ~a, ~b, ~const0 ) );    // 88

		spec.add_free_primitive( a ^ b ^ c);                                // 96
		spec.add_free_primitive( a ^ b );                                   // 66
		spec.add_free_primitive( b ^ c );                                   // 3c
		spec.add_free_primitive( a ^ c );                                   // 5a

		auto pchain = [&]() -> std::optional<percy::chain_minmc> {
			if ( !with_dc && _ps.cache )
			{
				const auto it = _ps.cache->find( tt );
				if ( it != _ps.cache->end() )
				{
					return it->second;
				}
			}
			if ( !with_dc && _ps.blacklist_cache )
			{
				const auto it = _ps.blacklist_cache->find( tt );
				if ( it != _ps.blacklist_cache->end() && ( it->second == 0 || _ps.conflict_limit <= it->second ) )
				{
					return std::nullopt;
				}
			}

			auto [mc, valid_mc] = look_up_mc( tt );

			if ( !valid_mc )
			{
				return std::nullopt;
			}

			uint32_t upper_bound_oh = mc;
			uint32_t lower_bound_oh = ( mc + 1 ) / 2;

			percy::chain_minmc chain;
			for ( uint32_t bound_nfree = lower_bound_oh; bound_nfree <= upper_bound_oh; ++bound_nfree )
			{
				spec.set_nfree( bound_nfree );
				percy::synth_stats synth_st;

				if ( const auto result = percy::std_synthesize_minmc( spec, chain, &synth_st );
					 	 result == percy::success )
				{
					if ( bound_nfree < upper_bound_oh )
					{
						std::cout << "[i] Awesome! Achieve lower garbling cost! \n";

					}





					if( !with_dc && _ps.cache )
					{
						( *_ps.cache )[tt] = chain;

						if ( _dir_prefix != "" )
						{
							std::string cache_name = "cache_new.db";
							std::ofstream fout;
							fout.open( _dir_prefix + cache_name, std::ios::app );
							chain.write_chain( fout, tt );
							fout.close();
						}
					}

					//_st.time_exact_synth_success += synth_st.sat_time;
					return chain;
				}
				else
				{
					//_st.time_exact_synth_fail += synth_st.unsat_time;

					if ( bound_nfree == upper_bound_oh )
					{
						if ( !with_dc && _ps.blacklist_cache )
						{
							if ( result == percy::timeout )
							{
								/* failed due to conflict limitation  */
								/* record current conflict limitation */
								( *_ps.blacklist_cache )[tt] = _ps.conflict_limit;

								//++_st.failures.at( static_cast<uint8_t>( failure_type::exact_synth_conflict_fail ) );
							}
							else
							{
								/* failed due to there's no solution */
								( *_ps.blacklist_cache )[tt] = 0u;

								//++_st.failures.at( static_cast<uint8_t>( failure_type::exact_synth_fail ) );

							}
							if ( _dir_prefix != "" )
							{
								std::string cache_name = "blacklist_new.db";
								std::ofstream fout;
								fout.open( _dir_prefix + cache_name, std::ios::app );
								/* write the number of pis       */
								fout << tt.num_vars() << "\n";

        				/* write the truth table         */
        				kitty::print_hex( tt, fout );
        				fout << "\n";

								/* write the conflict limitation */
								fout << _ps.conflict_limit << "\n";
								fout.close();
							}
						}
						return std::nullopt;
					}
				}
			}
		}();

		if ( !pchain )
		{
			return;
		}
		
		std::vector<signal> signals( tt.num_vars(), ntk.get_constant( false ) );
    std::copy( begin, end, signals.begin() );

		for ( uint32_t i = 0u; i < static_cast<uint32_t> ( pchain->get_nr_steps() ); ++i )
		{
			auto const c1 = signals[pchain->get_step( i )[0]];
			auto const c2 = signals[pchain->get_step( i )[1]];
			auto const c3 = signals[pchain->get_step( i )[2]];

			switch ( pchain->get_operator( i )._bits[0] )
			{
			case 0x00:
				signals.emplace_back( ntk.get_constant( false ) );
				break;
			case 0xaa:
				signals.emplace_back( c1 );
				break;
			case 0xcc:
				signals.emplace_back( c2 );
				break;
			case 0xf0:
				signals.emplace_back( c3 );
				break;
			case 0x16:
				signals.emplace_back(  ntk.create_onehot(  c1,  c2,  c3 ) );
				break;
			case 0xd6:
				signals.emplace_back( !ntk.create_onehot( !c1,  c2,  c3 ) );
				break;
			case 0xb6:
				signals.emplace_back( !ntk.create_onehot(  c1, !c2,  c3 ) );
				break;
			case 0x9e:
				signals.emplace_back( !ntk.create_onehot(  c1,  c2, !c3 ) );
				break;
			case 0x86:
				signals.emplace_back(  ntk.create_onehot( !c1, !c2,  c3 ) );
				break;
			case 0x94:
				signals.emplace_back(  ntk.create_onehot(  c1, !c2, !c3 ) );
				break;
			case 0x92:
				signals.emplace_back(  ntk.create_onehot( !c1,  c2, !c3 ) );
				break;
			case 0x68:
				signals.emplace_back(  ntk.create_onehot( !c1, !c2, !c3 ) );
				break;
			case 0xfc:
				signals.emplace_back( !ntk.create_onehot(  ntk.get_constant( true ),  c2,  c3 ) );
				break;
			case 0x0c:
				signals.emplace_back(  ntk.create_onehot(  ntk.get_constant( true ), !c2,  c3 ) );
				break;
			case 0x30:
				signals.emplace_back(  ntk.create_onehot(  ntk.get_constant( true ),  c2, !c3 ) );
				break;
			case 0xc0:
				signals.emplace_back(  ntk.create_onehot(  ntk.get_constant( true ), !c2, !c3 ) );
				break;
			case 0xfa:
				signals.emplace_back( !ntk.create_onehot(  c1,  ntk.get_constant( true ),  c3 ) );
				break;
			case 0x0a:
				signals.emplace_back(  ntk.create_onehot( !c1,  ntk.get_constant( true ),  c3 ) );
				break;
			case 0x50:
				signals.emplace_back(  ntk.create_onehot(  c1,  ntk.get_constant( true ), !c3 ) );
				break;
			case 0xa0:
				signals.emplace_back(  ntk.create_onehot( !c1,  ntk.get_constant( true ), !c3 ) );
				break;
			case 0xee:
				signals.emplace_back( !ntk.create_onehot(  c1,  c2,  ntk.get_constant( true ) ) );
				break;
			case 0x22:
				signals.emplace_back(  ntk.create_onehot( !c1,  c2,  ntk.get_constant( true ) ) );
				break;
			case 0x44:
				signals.emplace_back(  ntk.create_onehot(  c1, !c2,  ntk.get_constant( true ) ) );
				break;
			case 0x88:
				signals.emplace_back(  ntk.create_onehot( !c1, !c2,  ntk.get_constant( true ) ) );
				break;
			case 0x3c:
				signals.emplace_back( ntk.create_xor( c2, c3 ) );
				break;
			case 0x5a:
				signals.emplace_back( ntk.create_xor( c1, c3 ) );
				break;
			case 0x66:
				signals.emplace_back( ntk.create_xor( c1, c2 ) );
				break;
			case 0x96:
				signals.emplace_back( ntk.create_xor3( c1, c2, c3 ) );
				break;
			default:
				std::cerr << "[e] unsupported operation " << kitty::to_hex( pchain->get_operator( i ) ) << std::endl;
				assert( false );
				break;
			}
		}

		auto const output_signal = signals.back();

		fn( normal ? output_signal : !output_signal );
	}

	template<typename LeavesIterator, typename Fn>
	void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
	{
		const kitty::dynamic_truth_table tt = function.num_vars() < 3u ? kitty::extend_to( function, 3u ) : function;

		operator()( ntk, tt, tt.construct(), begin, end, fn );
	}

private:
std::tuple<uint32_t, bool> look_up_mc( kitty::dynamic_truth_table const& tt ) const
{
	uint32_t mc = 0u;
	bool valid_mc = false;
	/* Solution 1: if matching failed, a failure would be generated */
	if ( _use_db )
	{
		const auto tt_lookup = tt.num_vars() < 6u ? kitty::extend_to( tt, 6u ) : tt;
		const auto spectral = kitty::exact_spectral_canonization_limit( tt_lookup, 100000 );

		if ( !spectral.second )
		{
			( *_ps.blacklist_cache )[tt] = static_cast<uint8_t>( failure_type::compute_repr_fail );
			//++_st.failures.at( static_cast<uint8_t>( failure_type::compute_repr_fail ) );
			mc = 0u;
			valid_mc = false;
			return { mc, valid_mc };
		}
		
		kitty::dynamic_truth_table tt_repr = spectral.first;

		auto search = func_mc->find( kitty::to_hex( tt_repr ) );
		if ( search != func_mc->end() )
		{
			mc = search->second;
			valid_mc = true;
			return { mc, valid_mc };
		}

		( *_ps.blacklist_cache )[tt] = static_cast<uint8_t>( failure_type::match_db_fail );
		//++_st.failures.at( static_cast<uint8_t>( failure_type::match_db_fail ) );
		mc = 0u;
		valid_mc = false;
		return { mc, valid_mc };
	}

	/* Solution 2: forget about database */
	else
	{
		/* need a func. to quickly figure out mc of a given tt */
		const auto tt_lookup = tt.num_vars() < 5u ? kitty::extend_to( tt, 5u ) : tt;
		mc = kitty::get_spectral_mc( tt_lookup );
		valid_mc = true;
		return { mc, valid_mc };
	}
}

void build_db( std::string const& db_filename )
{
	stopwatch t_total( _st.time_total );
	stopwatch t_parse_db( _st.time_parse_db );

	std::ifstream db( db_filename.c_str(), std::ifstream::in );

	std::string line;
	unsigned pos{ 0u };

	while ( std::getline( db, line ) )
	{
		pos = static_cast<unsigned>( line.find( '\t' ) );

    pos += 18u;
    const auto repr = line.substr( pos, 16u );

    pos += 17u;
		auto mc = std::stoul( line.substr( pos, 1u ) );

		func_mc->emplace( repr, static_cast<uint32_t>( mc ) );
	}

	std::cout << func_mc->size() << " representatives in the database\n";
}

public:
	exact_xohg_resynthesis_minmc_stats _st;

private:
	exact_xohg_resynthesis_minmc_params _ps;
	exact_xohg_resynthesis_minmc_stats* _pst{ nullptr };
	bool _use_db;
	std::shared_ptr<std::unordered_map<std::string, uint32_t>> func_mc;
	std::string _dir_prefix;
}; /* exact_xohg_resynthesis_minmc */

} /* namespace mockturtle */