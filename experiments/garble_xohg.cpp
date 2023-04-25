#include <string>
#include <fmt/format.h>
#include <fstream>

#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/networks/x1g.hpp>
#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/algorithms/node_resynthesis/xohg_minmc.hpp>
#include <mockturtle/algorithms/node_resynthesis/x1g_npn.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/io/verilog_reader.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/utils/include/percy.hpp>
#include <mockturtle/views/topo_view.hpp>

#include <kitty/kitty.hpp>

#include <experiments.hpp>

static const uint32_t date20_epfl[] = {
	128u, 832u, 5291u, 10913u, 890u, 7653u, 2603u, 5381u, 4672u, 1174u, 394u, 
	45u, 328u, 557u, 85u, 4695u, 323u, 93u, 4257u, 0u};

static const uint32_t date20_crypto[] = {
	32u, 64u, 5440u, 6800u, 92u, 92u, 92u, 92u, 9205u, 9048u, 9367u, 1689u, 
	11515u, 26827u};

static const uint32_t date20_mpc[] = {
	97u, 193u, 232u, 456u, 495u, 975u, 554u, 1162u, 881u, 1919u, 1060u, 2394u, 
	7u, 15u, 21u, 55u, 104u, 275u, 16001u, 58723u};

/* skip benchmarks whose best scores are 0 */
static const uint32_t tcad22_epfl[] = {
	0u, 832u, 5132u, 8773u, 872u, 7585u, 1959u, 5217u, 4596u, 
	0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};

static const uint32_t tcad22_crypto[] = {
	0u, 0u, 5440u, 6800u, 84u, 87u, 84u, 87u, 6915u, 6833u, 
	9367u, 1689u, 11483u, 26464u};

static const uint32_t host19_mpc[] = {
	97u, 194u, 228u, 454u, 492u, 975u, 556u, 1159u, 1079u, 2367u, 
	1516u, 3500u, 8u, 16u, 37u, 79u, 147u, 388u, 0u, 0u};//157182u, 264192u

std::vector<uint32_t> epfl_date20()
{
	std::vector<uint32_t> best_score;
	for ( auto i = 0u; i < 20u; ++i )
	{
		best_score.emplace_back( date20_epfl[i] );
	}
	return best_score;
}

std::vector<uint32_t> crypto_date20()
{
	std::vector<uint32_t> best_score;
	for ( auto i = 0u; i < 14u; ++i )
	{
		best_score.emplace_back( date20_crypto[i] );
	}
	return best_score;
}

std::vector<uint32_t> mpc_date20()
{
	std::vector<uint32_t> best_score;
	for ( auto i = 0u; i < 20u; ++i )
	{
		best_score.emplace_back( date20_mpc[i] );
	}
	return best_score;
}

std::vector<uint32_t> epfl_tcad22()
{
	std::vector<uint32_t> best_score;
	for( auto i = 0u; i < 20u; ++i )
	{
		best_score.emplace_back( tcad22_epfl[i] );
	}
	return best_score;
}

std::vector<uint32_t> crypto_tcad22()
{
	std::vector<uint32_t> best_score;
	for( auto i = 0u; i < 14u; ++i )
	{
		best_score.emplace_back( tcad22_crypto[i] );
	}
	return best_score;
}

std::vector<uint32_t> mpc_host19()
{
	std::vector<uint32_t> best_score;
	for( auto i = 0u; i < 20u; ++i )
	{
		best_score.emplace_back( host19_mpc[i] );
	}
	return best_score;
}

static const std::string EPFL_benchmarks[] = {
	"adder", "bar", "div", "log2", "max", "multiplier", "sin", "sqrt", "square", "arbiter", 
	"cavlc", "ctrl" , "dec", "i2c", "int2float" , "mem_ctrl", "priority", "router", "voter", 
	"hyp"};

static const std::string CRYPTO_benchmarks[] = {
  "adder_32bit_untilsat", "adder_64bit_untilsat", "AES-expanded_untilsat", "AES-non-expanded_untilsat", 
  "comparator_32bit_signed_lt_untilsat", "comparator_32bit_signed_lteq_untilsat", "comparator_32bit_unsigned_lt_untilsat", "comparator_32bit_unsigned_lteq_untilsat", 
  "DES-expanded_untilsat", "DES-non-expanded_untilsat", "md5_untilsat", "mult_32x32_untilsat", "sha-1_untilsat", 
  "sha-256_untilsat"};

static const std::string MPC_benchmarks[] = {
  "auction_N_2_W_16", "auction_N_2_W_32", "auction_N_3_W_16", "auction_N_3_W_32", "auction_N_4_W_16", "auction_N_4_W_32", 
  "knn_comb_K_1_N_8", "knn_comb_K_1_N_16", "knn_comb_K_2_N_8", "knn_comb_K_2_N_16", "knn_comb_K_3_N_8", "knn_comb_K_3_N_16", 
  "voting_N_1_M_3", "voting_N_1_M_4", "voting_N_2_M_2", "voting_N_2_M_3", "voting_N_2_M_4", "voting_N_3_M_4", 
  "stable_matching_comb_Ks_4_S_8", "stable_matching_comb_Ks_8_S_8"};

std::vector<std::string> epfl_benchmarks()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 20u; ++i )
	{
		result.emplace_back( EPFL_benchmarks[i] );
	}

	return result;
}

std::vector<std::string> crypto_benchmarks()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 14u; ++i )
	{
		result.emplace_back( CRYPTO_benchmarks[i] );
	}

	return result;
}

std::vector<std::string> mpc_benchmarks()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 20u; ++i )
	{
		result.emplace_back( MPC_benchmarks[i] );
	}

	return result;
}

std::string benchmark_path( uint32_t benchmark_type, std::string const& benchmark_name, uint8_t opt )
{
	switch( benchmark_type )
	{
	case 0u:
		return fmt::format( "../experiments/{}/{}.v", ( opt ? ( ( opt == 1u ) ? "epfl_opt" : "epfl_tcad22" ) : "epfl_benchmarks" ), benchmark_name );
	case 1u:
		return fmt::format( "../experiments/{}/{}.v", ( opt ? ( ( opt == 1u ) ? "crypto_opt" : "crypto_tcad22" ) : "crypto_benchmarks" ), benchmark_name );
	case 2u:
		return fmt::format( "../experiments/{}/{}.v", ( opt ? ( ( opt == 1u ) ? "mpc_opt" : "mpc_host19" ) : "mpc_benchmarks" ), benchmark_name );
	default:
		std::cout << "Unspecified type of benchmark. \n";
		abort();
	}

}

template<class Ntk>
bool abc_cec( Ntk const& ntk, uint32_t const& benchmark_type, std::string const& benchmark, uint8_t opt )
{
	mockturtle::write_bench( ntk, "/tmp/test.bench" );
	std::string abc_path = "/Users/myu/Documents/GitHub/abc/";
	std::string command = fmt::format( "{}abc -q \"cec -n {} /tmp/test.bench\"", abc_path, benchmark_path( benchmark_type, benchmark, opt ) );

	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype( &pclose )> pipe( popen( command.c_str(), "r" ), pclose );

	if ( !pipe )
	{
		throw std::runtime_error( "popen() failed" );
	}

	while ( fgets( buffer.data(), buffer.size(), pipe.get() ) != nullptr )
	{
		result += buffer.data();
	}

	return result.size() >= 23 && result.substr( 0u, 23u ) == "Networks are equivalent";
}

void load_cache( mockturtle::exact_xohg_resynthesis_minmc_params::cache_t pcache_db, std::string const& dir_prefix )
{
	std::string cache_name = dir_prefix + "cache.db";
	std::ifstream f( cache_name, std::ios::in );
	if ( !f )
	{
		std::cout << "[i] no database for cache detected\n";
		std::cout << "[i] skip cache loading\n";
		return;
	}

	std::cout << "[i] loading cache\n";
	
	bool done = false;
	while ( !done )
	{
		percy::chain_minmc chain;
		std::optional<kitty::dynamic_truth_table> ttOpt = chain.read_from_file( f );
		if ( ttOpt )
		{
			( *pcache_db )[*ttOpt] = chain;
		}
		else
		{
			done = true;
		}
	}

	f.close();
	std::cout << "[i] cache loaded\n";
	std::cout << "[i] " << ( *pcache_db ).size() << " functions in the cache. \n";
}

void load_blacklist( mockturtle::exact_xohg_resynthesis_minmc_params::blacklist_cache_t pblacklist_db, std::string const& dir_prefix )
{
	std::string blacklist_name = dir_prefix + "blacklist.db";
	std::ifstream f( blacklist_name, std::ios::in );
	if ( !f )
	{
		std::cout << "[i] no database for blacklist detected\n";
		std::cout << "[i] skip blacklist loading\n";
		return;
	}

	std::cout << "[i] loading blacklist\n";

	bool done = false;
	while ( !done )
	{
		char buffer[1024] = {0};
		if ( f >> buffer )
		{
			/* read the number of pis       */
			uint32_t num_pis = static_cast<uint32_t>( std::atoi( buffer ) );

      /* read the truth table         */
      kitty::dynamic_truth_table tt( num_pis );
      f >> buffer;
      kitty::create_from_hex_string( tt, buffer );

      /* read the conflict limitation */
      f >> buffer;
      uint32_t conflict_limitation = static_cast<uint32_t>( std::atoi( buffer ) );

      /* store this entry */
      ( *pblacklist_db )[tt] = conflict_limitation;
		}
		else
		{
			done = true;
		}
	}

	f.close();
	std::cout << "[i] blacklist loaded\n";
	std::cout << "[i] " << ( *pblacklist_db ).size() << " functions in the blacklist. \n";
}

namespace detail
{
template<class Ntk = mockturtle::xag_network>
struct num_and
{
	uint32_t operator()( Ntk const& ntk, mockturtle::node<Ntk> const& n ) const
	{
		return ntk.is_and( n ) ? 1u : 0u;
	}
};

template<class Ntk = mockturtle::x1g_network>
struct num_oh
{
	uint32_t operator()( Ntk const& ntk, mockturtle::node<Ntk> const& n ) const
	{
		return ntk.is_onehot( n ) ? 1u : 0u;
	}
};
}

int main()
{
	for ( auto benchmark_type_each = 2u; benchmark_type_each <= 2u; ++benchmark_type_each ) 
	{
		std::string json_name = "garble_xohg" + std::to_string( benchmark_type_each );
		experiments::experiment<std::string, bool, uint32_t, uint32_t, float, uint32_t, float, bool> exp_res( json_name, "benchmark", "optimized", "previous_best", "num_oh_after", "improvement %", "iterations", "avg. runtime [s]", "equivalent" );
		//uint32_t benchmark_type = 0u; /* 0u - epfl benchmark; 1u - crypto benchmark; 2u - mpc benchmark */
		uint32_t benchmark_type = benchmark_type_each;
		/* 0u - unoptimized benchmarks; 1u - optimized benchmarks from DATE20; 2u - optimized benchmarks from TCAD22 */
		uint8_t opt = 1u;
		std::cout << "[i] working on " << ( opt ? ( ( opt == 1u ) ? "DATE20 benchmarks\n" : "HOST19 benchmarks\n" ) : "unoptimized benchmarks\n" );
		auto const benchmarks = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_benchmarks() : mpc_benchmarks() ) : epfl_benchmarks();
		//std::vector<uint32_t> const best_scores  = benchmark_type ? crypto_tcad22() : epfl_tcad22();
		std::vector<uint32_t> const best_scores  = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_date20() : mpc_date20() ) : epfl_date20();
		//if ( opt == 2u )
		//{
		//	best_scores = benchmark_type ? crypto_tcad22() : epfl_tcad22();
		//}
		//else
		//{
		//	best_scores = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_date20() : mpc_date20() ) : epfl_date20();
		//}
		auto const dir_prefix = benchmark_type ? ( ( benchmark_type == 1u ) ? "../experiments/databases/crypto/" : "../experiments/databases/mpc/" ) : "../experiments/databases/epfl/";

		mockturtle::cut_rewriting_params ps_cut_rew;
		ps_cut_rew.cut_enumeration_ps.cut_size = 5u;
		ps_cut_rew.cut_enumeration_ps.cut_limit = 12u;
		ps_cut_rew.verbose = false;
		ps_cut_rew.progress = true;
		ps_cut_rew.min_cand_cut_size = 2u;

		for ( auto i = 19u; i < benchmarks.size(); ++i )
		{
			auto const benchmark = benchmarks[i];
			auto const best_score = best_scores[i];
			if ( best_score == 0u )
			{
				std::cout << "[i] skip " << ( opt ? "optimized " : "" ) << benchmark << std::endl;
				continue;
			}

			std::cout << "[i] processing " << ( opt ? "optimized " : "" ) << benchmark << std::endl;

			auto const dir_prefix_benchmark = dir_prefix + benchmark + "/";
			mockturtle::exact_xohg_resynthesis_minmc_params::cache_t pcache_db = std::make_shared<mockturtle::exact_xohg_resynthesis_minmc_params::cache_map_t>();
			mockturtle::exact_xohg_resynthesis_minmc_params::blacklist_cache_t pblacklist_db = std::make_shared<mockturtle::exact_xohg_resynthesis_minmc_params::blacklist_cache_map_t>();
			if ( ps_cut_rew.cut_enumeration_ps.cut_size > 4u )
			{
				load_cache( pcache_db, dir_prefix_benchmark );
				load_blacklist( pblacklist_db, dir_prefix_benchmark );
			}

			mockturtle::x1g_network x1g;
			
			/* Obtain initial X1G by reading in benchmarks */
			//auto const read_result = lorina::read_aiger( benchmark_path( benchmark_type, benchmark, opt ), mockturtle::aiger_reader( x1g ) );
			auto const read_result = lorina::read_verilog( benchmark_path( benchmark_type, benchmark, opt ), mockturtle::verilog_reader( x1g ) );
			assert( read_result == lorina::return_code::success );
			( void )read_result;


			if ( read_result == lorina::return_code::success )
			{
				std::cout << "[i] successfully read " << benchmark << std::endl;
			}
			else
			{
				std::cout << "[i] failed to read " << benchmark << std::endl;
			}


			uint32_t num_oh = 0u;
			uint32_t num_oh_aft = 0u;

			x1g.foreach_gate( [&]( auto f ) {
				if ( x1g.is_onehot( f ) )
				{
					++num_oh;
				}
			} );

			if ( num_oh == 0 )
			{
				exp_res( benchmark, opt, 0u, 0u, 0., 0u, 0., true );
				continue;
			}
			num_oh_aft = num_oh - 1u;

			clock_t begin_time;
			uint32_t ite_cnt = 0u;
			if ( ps_cut_rew.cut_enumeration_ps.cut_size > 4u )
			{
				mockturtle::exact_xohg_resynthesis_minmc_params ps_xohg_resyn;
				ps_xohg_resyn.print_stats = true;
				ps_xohg_resyn.conflict_limit = 1000000u;
				ps_xohg_resyn.cache = pcache_db;
				ps_xohg_resyn.blacklist_cache = pblacklist_db;

				mockturtle::exact_xohg_resynthesis_minmc_stats* pst_xohg_resyn = nullptr;
				bool use_db = ( ps_cut_rew.cut_enumeration_ps.cut_size == 5u ) ? false : true;

				mockturtle::exact_xohg_resynthesis_minmc xohg_resyn( "../experiments/db", dir_prefix_benchmark, ps_xohg_resyn, pst_xohg_resyn, use_db );

				begin_time = clock();
				while ( num_oh > num_oh_aft )
				{
					if ( ite_cnt > 0u )
					{
						num_oh = num_oh_aft;
					}
					++ite_cnt;
					num_oh_aft = 0u;

					x1g = mockturtle::cut_rewriting<mockturtle::x1g_network, decltype( xohg_resyn ), ::detail::num_oh<mockturtle::x1g_network>>( x1g, xohg_resyn, ps_cut_rew );

					x1g.foreach_gate( [&]( auto f ) {
						if ( x1g.is_onehot( f ) )
						{
							++num_oh_aft;
						}
					} );
				}
			}
			else
			{
				mockturtle::x1g_npn_resynthesis<mockturtle::x1g_network> x1g_resyn;

				begin_time = clock();
				while ( num_oh > num_oh_aft )
				{
					if ( ite_cnt > 0u )
					{
						num_oh = num_oh_aft;
					}
					++ite_cnt;
					num_oh_aft = 0u;


					//mockturtle::cut_rewriting<mockturtle::x1g_network, decltype( x1g_resyn ), ::detail::num_oh<mockturtle::x1g_network>>( x1g, x1g_resyn, ps_cut_rew, nullptr );
	    		
	    		//
	    		mockturtle::exact_library_params _exact_lib_params;
	    		_exact_lib_params.verbose = false;
	    		_exact_lib_params.np_classification = false;
	    		mockturtle::exact_library<mockturtle::x1g_network, decltype( x1g_resyn ), 4u, ::detail::num_oh<mockturtle::x1g_network>> lib( x1g_resyn, _exact_lib_params );
	    		mockturtle::map_params _map_params;
	    		_map_params.skip_delay_round = false;
	    		_map_params.area_flow_rounds = 3u;
	    		_map_params.ela_rounds = 3u;
	    		_map_params.enable_logic_sharing = true;
	    		_map_params.verbose = true;
	    		x1g = mockturtle::map( x1g, lib, _map_params );
	    		//

					x1g = mockturtle::cleanup_dangling( x1g );

					x1g.foreach_gate( [&]( auto f ) {
						if ( x1g.is_onehot( f ) )
						{
							++num_oh_aft;
						}
					} );
				}
			}

			mockturtle::write_bench( x1g, "/Users/myu/Documents/GitHub/abc/bench" );

			const auto cec = abc_cec( x1g, benchmark_type, benchmark, opt );
			//assert( cec );

			float improve = ( ( static_cast<float> ( best_score ) - static_cast<float> ( num_oh_aft ) ) / static_cast<float> ( best_score ) ) * 100;

			exp_res( benchmark, opt, best_score, num_oh_aft, improve, ite_cnt, ( float( clock() - begin_time ) / CLOCKS_PER_SEC ) / ite_cnt, cec );
		}

		exp_res.save();
		exp_res.table();
	}

	return 0;
}
