#include <mockturtle/algorithms/xag2x1g.hpp>
#include <mockturtle/io/verilog_reader.hpp>
#include <mockturtle/io/write_bench.hpp>

#include <experiments.hpp>

using merge_view = mockturtle::fanout_view<mockturtle::topo_view<mockturtle::xag_network>>;

static const uint32_t host23_epfl[] = {
	256u, 1664u, 9728u, 16358u, 1656u, 14902u, 3480u, 9942u, 8890u, 
	1658u, 556u, 76u, 656u, 942u, 140u, 7578u, 544u, 120u, 7186u, 0u };

static const uint32_t host23_crypto[] = {
	10240u, 12800u, 13144u, 12984u, 130u, 140u, 130u, 140u, 18734u, 22688u, 50540u };

static const uint32_t host23_mpc[] = {
	194u, 386u, 460u, 908u, 986u, 1946u, 1104u, 2312u, 1698u, 3692u, 2092u, 4724u, 
	14u, 28u, 40u, 110u, 208u, 546u, 28284u, 105890u };

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
bool abc_cec_quick_test( Ntk const& ntk, std::string const& filepath )
{
	mockturtle::write_bench( ntk, "/tmp/test.bench" );
	std::string abc_path = "/Users/myu/Documents/GitHub/abc/";
	std::string command = fmt::format( "{}abc -q \"cec -n {} /tmp/test.bench\"", abc_path, filepath );

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

void count_and_size_rec( merge_view& xag, mockturtle::xag_network::node const& f, mockturtle::xag_network::node const& root )
{
	if ( xag.value( f ) == 0u ) 
	{
		if ( f != root )
		{
			if ( xag.fanout( f ).size() == 1u )
			{
				/* remark this node as traversed */
				xag.incr_value( f );
				/* update the size of this AND group */
				xag.incr_value( root );
				/* recursively trace its fanin */
				xag.foreach_fanin( f, [&]( auto const& fi ) {
					auto const child = xag.get_node( fi );
					if ( xag.is_and( child ) && !xag.is_complemented( fi ) )
					//if ( xag.is_and( child ) )
					{
						count_and_size_rec( xag, child, root );
					}
				} );
			}
		}
		else
		{
			xag.set_value( f, 2u );
			/* recursively trace its fanin */
			xag.foreach_fanin( f, [&]( auto const& fi ) {
				auto const child = xag.get_node( fi );
				if ( xag.is_and( child ) && !xag.is_complemented( fi ) )
				{
					count_and_size_rec( xag, child, f );
				}
			} );
		}
	}
}

template<class Ntk = mockturtle::xag_network>
struct num_and
{
	uint32_t operator()( Ntk const& ntk, mockturtle::node<Ntk> const& n ) const
	{
		return ntk.is_and( n ) ? 1u : 0u;
	}
};

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
	/* 0u - epfl; 1u - crypto; 2u - mpc */
	//
	for ( auto benchmark_type_each{ 0u }; benchmark_type_each <= 0u; ++benchmark_type_each ) 
	{
		std::string json_name = "garble_xag2x1g_" + std::to_string( benchmark_type_each );
		experiments::experiment<std::string, uint32_t, uint32_t, uint32_t, float, float, float, bool> exp_res( json_name, "benchmark", "SOTA", "gc_before", "gc_after", "improvement ( before ) %", "improvement ( SOTA ) %", "avg. runtime [s]", "equivalent" );
		uint32_t benchmark_type = benchmark_type_each;
		// 0u - unoptimized benchmarks; 1u - optimized benchmarks from DATE20; 2u - optimized benchmarks from TCAD22 //
		uint8_t opt = 2u;
		std::cout << "[i] working on " << ( opt ? ( ( opt == 1u ) ? "DATE20 benchmarks\n" : "TCAD22 benchmarks\n" ) : "unoptimized benchmarks\n" );
		auto const benchmarks = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_benchmarks() : mpc_benchmarks() ) : epfl_benchmarks();
		std::vector<uint32_t> const best_scores  = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_host23() : mpc_host23() ) : epfl_host23();
		float time_avr = 0;

		for ( auto i = 9u; i < benchmarks.size(); ++i )
		{
			auto const benchmark = benchmarks[i];
			auto const best_score = best_scores[i];
			if ( best_score == 0u )
			{
				std::cout << "[i] skip " << ( opt ? "optimized " : "" ) << benchmark << std::endl;
				continue;
			}

			std::cout << "[i] processing " << ( opt ? "optimized " : "" ) << benchmark << std::endl;

			mockturtle::xag_network xag;
			
			auto const read_result = lorina::read_verilog( benchmark_path( benchmark_type, benchmark, opt ), mockturtle::verilog_reader( xag ) );
			assert( read_result == lorina::return_code::success );
			( void )read_result;

			uint32_t garble_cost_bfr = 0u;
			uint32_t garble_cost_aft = 0u;

			mockturtle::topo_view xag_topo{ xag };
			merge_view xag_merge{ xag_topo };
			xag_merge.clear_values();

			xag_merge.foreach_gate_reverse( [&]( auto const& n ) {
				if ( xag_merge.is_and( n ) && xag_merge.value( n ) == 0u )
				{
					::detail::count_and_size_rec( xag_merge, n, n );
					garble_cost_bfr += xag_merge.value( n );
				}
			} );
			xag_merge.clear_values();

			const clock_t begin_time = clock();
			mockturtle::x1g_network x1g = mockturtle::map_xag2x1g( xag );
			x1g.foreach_gate( [&]( auto n ) {
				if ( x1g.is_onehot( n ) )
				{
					++garble_cost_aft;
				}
			} );
			garble_cost_aft *= 2;

			//mockturtle::write_bench( x1g, "/Users/myu/Documents/GitHub/abc/bench" );
			//const auto cec = abc_cec( x1g, benchmark_type, benchmark, opt );
			//assert( cec );

			time_avr = float( clock() - begin_time ) / CLOCKS_PER_SEC;

			float improve_bfr = ( ( static_cast<float> ( garble_cost_bfr ) - static_cast<float> ( garble_cost_aft ) ) / static_cast<float> ( garble_cost_bfr ) ) * 100;
			float improve_sota = ( ( static_cast<float> ( best_scores[i] ) - static_cast<float> ( garble_cost_aft ) ) / static_cast<float> ( best_scores[i] ) ) * 100;

			exp_res( benchmark,best_scores[i], garble_cost_bfr, garble_cost_aft, improve_bfr, improve_sota, time_avr , true );
		}

		exp_res.save();
		exp_res.table();
		//std::cout << "[i] spent " << time_avr / 10 << " seconds in average. \n";
	} 
	//
	/*
	mockturtle::xag_network xag;
			
	auto const read_result = lorina::read_verilog( "../experiments/epfl_opt/toy.v", mockturtle::verilog_reader( xag ) );
	assert( read_result == lorina::return_code::success );
	( void )read_result;

	uint32_t garble_cost_bfr = 0u;
	uint32_t garble_cost_aft = 0u;

	mockturtle::topo_view xag_topo{ xag };
	merge_view xag_merge{ xag_topo };
	xag_merge.clear_values();

	//uint32_t group_cnt{ 0u };

	xag_merge.foreach_gate_reverse( [&]( auto const& n ) {
		if ( xag_merge.is_and( n ) && xag_merge.value( n ) == 0u )
		{
			::detail::count_and_size_rec( xag_merge, n, n );
			//std::cout << "[m] root of group " << ++group_cnt << " is node " << n << "\n";
			//std::cout << "[m] GC of this group is " << xag_merge.value( n ) << "\n";
			garble_cost_bfr += xag_merge.value( n );
		}
	} );
	xag_merge.clear_values();

	mockturtle::x1g_network x1g = mockturtle::map_xag2x1g( xag );
	x1g.foreach_gate( [&]( auto n ) {
		if ( x1g.is_onehot( n ) )
		{
			++garble_cost_aft;
		}
	} );
	garble_cost_aft *= 2;

	std::cout << "GC of merged XAG is: " << garble_cost_bfr << "\n";
	std::cout << "GC of mapped X1G is: " << garble_cost_aft << "\n";

	auto cec = abc_cec_quick_test( x1g, "../experiments/epfl_opt/toy_simple.v" );
	assert( cec );
	*/

	return 0;
}
