#include <iostream>

#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/node_resynthesis/x1g_mingc_rewrite.hpp>
#include <mockturtle/algorithms/x1g_optimization.hpp>
#include <mockturtle/io/verilog_reader.hpp>
#include <mockturtle/io/write_bench.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <mockturtle/views/topo_view.hpp>

#include <experiments.hpp>

static const uint32_t date20_epfl[] = {
	128u, 832u, 5291u, 10913u, 890u, 7653u, 2603u, 5381u, 4672u, 1174u, 394u, 
	45u, 328u, 557u, 85u, 4695u, 323u, 93u, 4257u, 0u};

static const uint32_t date20_crypto[] = {
	5440u, 6800u, 9205u, 9048u, 92u, 92u, 92u, 92u, 9367u, 11515u, 26827u};

static const uint32_t date20_mpc[] = {
	97u, 193u, 232u, 456u, 495u, 975u, 554u, 1162u, 881u, 1919u, 1060u, 2394u, 
	7u, 15u, 21u, 55u, 104u, 275u, 16001u, 58723u};

/* skip benchmarks whose best scores are 0 */
static const uint32_t tcad22_epfl[] = {
	0u, 832u, 5132u, 8773u, 872u, 7585u, 1959u, 5217u, 4596u, 
	0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};

static const uint32_t tcad22_crypto[] = {
	5440u, 6800u, 6915u, 6833u, 84u, 87u, 84u, 87u, 9367u, 11483u, 26464u };

static const uint32_t host19_mpc[] = {
	97u, 194u, 228u, 454u, 492u, 975u, 556u, 1159u, 1079u, 2367u, 
	1516u, 3500u, 8u, 16u, 37u, 79u, 147u, 388u, 0u, 0u };//157182u, 264192u

static const uint32_t host23_epfl[] = {
	256u, 1664u, 9728u, 16358u, 1656u, 14902u, 3480u, 9942u, 8890u, 
	1658u, 556u, 76u, 656u, 942u, 140u, 7578u, 544u, 120u, 7186u, 0u };

static const uint32_t host23_crypto[] = {
	10240u, 12800u, 13144u, 12984u, 130u, 140u, 130u, 140u, 18734u, 22688u, 50540u };

static const uint32_t host23_mpc[] = {
	194u, 386u, 460u, 908u, 986u, 1946u, 1104u, 2312u, 1698u, 3692u, 2092u, 4724u, 
	14u, 28u, 40u, 110u, 208u, 546u, 28284u, 105890u };

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

std::vector<uint32_t> epfl_host23()
{
	std::vector<uint32_t> best_score;
	for ( auto i = 0u; i < 20u; ++i )
	{
		best_score.emplace_back( host23_epfl[i] );
	}
	return best_score;
}

std::vector<uint32_t> crypto_host23()
{
	std::vector<uint32_t> best_score;
	for ( auto i = 0u; i < 11u; ++i )
	{
		best_score.emplace_back( host23_crypto[i] );
	}
	return best_score;
}

std::vector<uint32_t> mpc_host23()
{
	std::vector<uint32_t> best_score;
	for ( auto i = 0u; i < 20u; ++i )
	{
		best_score.emplace_back( host23_mpc[i] );
	}
	return best_score;
}

static const std::string EPFL_benchmarks[] = {
	"adder", "bar", "div", "log2", "max", "multiplier", "sin", "sqrt", "square", "arbiter", 
	"cavlc", "ctrl" , "dec", "i2c", "int2float" , "mem_ctrl", "priority", "router", "voter", 
	"hyp"};

static const std::string CRYPTO_benchmarks[] = {
  "AES-expanded_untilsat", "AES-non-expanded_untilsat", "DES-expanded_untilsat", "DES-non-expanded_untilsat", 
  "comparator_32bit_signed_lt_untilsat", "comparator_32bit_signed_lteq_untilsat", "comparator_32bit_unsigned_lt_untilsat", "comparator_32bit_unsigned_lteq_untilsat", 
  "md5_untilsat", "sha-1_untilsat", "sha-256_untilsat"};

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
	for ( auto i = 0u; i < 11u; ++i )
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
		return fmt::format( "../experiments/{}/{}.v", ( opt ? ( ( opt == 1u ) ? "mpc_opt" : "mpc_opt" ) : "mpc_benchmarks" ), benchmark_name );
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

namespace detail
{

template<class Ntk = mockturtle::x1g_network>
struct num_onehot
{
	uint32_t operator()( Ntk const& ntk, mockturtle::node<Ntk> const& n ) const
	{
		return ntk.is_onehot( n ) ? 1u : 0u;
	}
};

} /* namespace detail */

int main()
{
	for ( auto benchmark_type_each{ 0u }; benchmark_type_each <= 2u; ++benchmark_type_each ) 
	{
		std::string json_name = "x1g_mingc_" + std::to_string( benchmark_type_each );
		experiments::experiment<std::string, uint32_t, uint32_t, float, uint32_t, float, bool> exp_res( json_name, "benchmark", "gc_before", "gc_after", "improvement %", "iterations", "avg. runtime [s]", "equivalent" );
		uint32_t benchmark_type = benchmark_type_each;
		/* 0u - unoptimized benchmarks; 1u - optimized benchmarks from DATE20; 2u - optimized benchmarks from TCAD22 */
		uint8_t opt = 2u;
		std::cout << "[i] working on " << ( opt ? ( ( opt == 1u ) ? "DATE20 benchmarks\n" : "TCAD22 benchmarks\n" ) : "unoptimized benchmarks\n" );
		auto const benchmarks = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_benchmarks() : mpc_benchmarks() ) : epfl_benchmarks();
		//std::vector<uint32_t> const best_scores  = benchmark_type ? crypto_tcad22() : epfl_tcad22();
		std::vector<uint32_t> const best_scores  = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_host23() : mpc_host23() ) : epfl_host23();

		mockturtle::cut_rewriting_params ps_cut_rewrite;
		ps_cut_rewrite.cut_enumeration_ps.cut_size = 6u;
		ps_cut_rewrite.cut_enumeration_ps.cut_limit = 12u;
		ps_cut_rewrite.verbose = false;
		ps_cut_rewrite.progress = true;
		ps_cut_rewrite.min_cand_cut_size = 2u;

		for ( auto i{ 0u }; i < benchmarks.size(); ++i )
		{
			auto const benchmark = benchmarks[i];
			if ( best_scores[i] == 0u )
			{
				std::cout << "[i] skip " << ( opt ? "optimized " : "" ) << benchmark << std::endl;
				continue;
			}

			std::cout << "[i] processing " << ( opt ? "optimized " : "" ) << benchmark << std::endl;

			/* obtain initial X1G by reading in benchmarks */
			mockturtle::x1g_network x1g;
			auto const read_result = lorina::read_verilog( benchmark_path( benchmark_type, benchmark, opt ), mockturtle::verilog_reader( x1g ) );
			if ( read_result == lorina::return_code::success )
			{
				std::cout << "[i] successfully read " << benchmark << std::endl;
			}
			else
			{
				std::cout << "[i] failed to read " << benchmark << std::endl;
				abort();
			}

			uint32_t num_oh = 0u;
			uint32_t num_oh_aft = 0u;
			x1g.foreach_gate( [&]( auto const& n ) {
				if ( x1g.is_onehot( n ) )
				{
					++num_oh;
				}
			} );
			if ( num_oh == 0u )
			{
				exp_res( benchmark, 0u, 0u, 0., 0u, 0., true );
				continue;
			}

			num_oh_aft = num_oh - 1u;
			//uint32_t const best_score = num_oh;
			uint32_t const best_score = best_scores[i];
			uint32_t ite_cnt = 0u;

			mockturtle::x1g_mingc_rewrite_params ps;
			//ps.verbose = true;
			//ps.verify_database = true;
			mockturtle::x1g_mingc_rewrite_stats st;
			mockturtle::x1g_mingc_rewrite_stats* pst = &st;
			mockturtle::x1g_mingc_rewrite x1g_rewrite( "db_gc_practical_x1g_6_mc4", ps, pst );

			clock_t begin_time = clock();
			while ( num_oh > num_oh_aft )
			{
				if ( ite_cnt > 0u )
				{
					num_oh = num_oh_aft;
				}
				++ite_cnt;

				x1g = mockturtle::cut_rewriting<mockturtle::x1g_network, decltype( x1g_rewrite ), ::detail::num_onehot<>>( x1g, x1g_rewrite, ps_cut_rewrite );
				
				num_oh_aft = 0u;
				x1g.foreach_gate( [&]( auto const& n ) {
					if ( x1g.is_onehot( n ) )
					{
						++num_oh_aft;
					}
				} );
			}

			std::cout << "[i] before post optimization, gc cost is: " << num_oh_aft * 2 << "\n";

			//x1g = mockturtle::x1g_merge_optimization( x1g );
			//num_oh_aft = 0u;
			//x1g.foreach_gate( [&]( auto const& n ) {
			//	if ( x1g.is_onehot( n ) )
			//	{
			//		++num_oh_aft;
			//	}
			//} );
			//std::cout << "[i] after operating merging, gc cost is: " << num_oh_aft * 2 << "\n";

			if ( best_score < 15000u && !( ( i == 7u ) && ( benchmark_type_each == 0u ) ) && !( ( i == 1u ) && ( benchmark_type_each == 1u ) ) )
			{
				x1g = mockturtle::x1g_dont_cares_optimization( x1g );
				num_oh_aft = 0u;
				x1g.foreach_gate( [&]( auto const& n ) {
					if ( x1g.is_onehot( n ) )
					{
						++num_oh_aft;
					}
				} );
				std::cout << "[i] after using don't cares, gc cost is: " << num_oh_aft * 2 << "\n";
			}
			else
			{
				std::cout << "[i] skip using don't cares, as the circuit is too large\n";
			}

			//mockturtle::write_bench( x1g, "/Users/myu/Documents/GitHub/abc/bench" );

			const auto cec = abc_cec( x1g, benchmark_type, benchmark, opt );
			assert( cec );
			float improve = ( ( static_cast<float>( best_score ) - static_cast<float>( num_oh_aft * 2 ) ) / static_cast<float>( best_score ) ) * 100;
			exp_res( benchmark, best_score, num_oh_aft * 2, improve, ite_cnt, ( float( clock() - begin_time ) / CLOCKS_PER_SEC ) / ite_cnt, cec );
		}

		exp_res.save();
		exp_res.table();
	}

	return 0;
}