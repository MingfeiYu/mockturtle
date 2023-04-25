#include <iostream>

#include <mockturtle/algorithms/cut_rewriting_on_scene.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_mingc_rewrite.hpp>
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

template<class Ntk = mockturtle::xag_network>
struct num_and
{
	uint32_t operator()( Ntk const& ntk, mockturtle::node<Ntk> const& n ) const
	{
		return ntk.is_and( n ) ? 1u : 0u;
	}
};

template<class Ntk = mockturtle::xag_network>
struct count_gc_approx
{
	uint32_t operator()( Ntk const& ntk, mockturtle::node<Ntk> const& n ) const
	{
		if ( !ntk.is_and( n ) )
		{
			return 0u;
		}
		else
		{
			mockturtle::fanout_view<Ntk> ntk_fanout{ ntk };
			bool is_po{ false };
			bool connect_to_inv_or_xor{ false };
			bool has_multiple_output{ false };

			if ( ntk_fanout.fanout( n ).size() == 1 )
			{
				auto const next = ( ntk_fanout.fanout( n ) )[0];
				if ( ntk_fanout.is_xor( next ) )
				{
					connect_to_inv_or_xor = true;
				}
				else
				{
					ntk_fanout.foreach_fanin( next, [&]( auto const& fi ) {
						if ( ntk_fanout.is_complemented( fi ) )
						{
							connect_to_inv_or_xor = true;
						}
					} );
				}
			}
			else if ( ntk_fanout.fanout( n ).size() == 0 )
			{
				is_po = true;
			}
			else
			{
				has_multiple_output = true;
			}

			if ( is_po || connect_to_inv_or_xor || has_multiple_output )
			{
				// either a root of a group or an individual 
				bool is_individual{ true };

				ntk_fanout.foreach_fanin( n, [&]( auto const& fi ) {
					auto const child = ntk_fanout.get_node( fi );
					if ( ntk_fanout.is_and( child ) && !ntk_fanout.is_complemented( fi ) && ntk_fanout.fanout( child ).size() == 1u )
					{
						// not an individual 
						is_individual = false;
					}
				} );

				if ( is_individual )
				{
					return 2u;
				}
			}

			// either an internal node, a root, or a leave of a group 
			return 1u;
		}
	}
};

using merge_view = mockturtle::topo_view<mockturtle::xag_network>;

void count_and_size_rec( merge_view& xag, merge_view::node const& f, merge_view::node const& root )
{
	if ( xag.value( f ) == 0u ) 
	{
		if ( f != root )
		{
			if ( xag.fanout_size( f ) == 1u )
			{
				/* remark this node as traversed */
				xag.incr_value( f );
				/* update the size of this AND clique */
				xag.incr_value( root );
				/* recursively trace its fanin */
				xag.foreach_fanin( f, [&]( auto const& fi ) {
					auto const child = xag.get_node( fi );
					if ( xag.is_and( child ) && !xag.is_complemented( fi ) )
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

uint32_t count_gc( merge_view& xag )
{
	uint32_t gc{ 0u };
	xag.clear_values();

	xag.foreach_gate_reverse( [&]( auto const& f ) {
		if ( xag.is_and( f ) && xag.value( f ) == 0u )
		{
			/* The root node of a potentially larger AND */
			//++num_and_aft;
			count_and_size_rec( xag, f, f );
			gc += xag.value( f );
			//garble_cost_aft_each = xag.value( f );
			//and_size.emplace_back( garble_cost_aft_each );
			//max_fan_in = ( xag.value( f ) > max_fan_in ) ? xag.value( f ) : max_fan_in;
		}
	} );
	xag.clear_values();

	return gc;
}

void recursive_gc_cost( merge_view const& xag_merge, merge_view::node const& n, merge_view::node const& root )
{
	if ( xag_merge.value( n ) && !xag_merge.visited( n ) ) 
	{
		if ( n != root )
		{
			if ( xag_merge.fanout_size( n ) == 1u )
			{
				/* remark this node as traversed */
				xag_merge.set_visited( n, ( xag_merge.visited( n ) + 1 ) );
				/* update the size of this AND clique */
				xag_merge.set_visited( root, ( xag_merge.visited( root ) + 1 ) );
			}
		}
		else
		{
			xag_merge.set_visited( n, 2u );
		}

		/* recursively trace its fanin */
		xag_merge.foreach_fanin( n, [&]( auto const& ni ) {
			auto const child = xag_merge.get_node( ni );
			if ( xag_merge.is_and( child ) && !xag_merge.is_complemented( ni ) )
			//if ( xag.value( child ) && xag_merge.is_and( child ) && !xag_merge.is_complemented( ni ) )
			{
				recursive_gc_cost( xag_merge, child, root );
			}
		} );
	}
}

struct gc_cost
{
  uint32_t operator()( mockturtle::xag_network const& xag ) const
  {
    uint32_t gc{ 0u };
    merge_view xag_merge{ xag };
    xag_merge.clear_visited();

    xag_merge.foreach_gate_reverse( [&]( auto const& n ) {
    	if ( xag_merge.value( n ) && xag_merge.is_and( n ) && !xag_merge.visited( n ) )
    	{
    		recursive_gc_cost( xag_merge, n, n );
    		gc += xag_merge.visited( n );
    	}
    } );

		xag_merge.clear_visited();

    return gc;
  }
};

} /* namespace detail */

int main()
{
	for ( auto benchmark_type_each{ 0u }; benchmark_type_each <= 0u; ++benchmark_type_each ) 
	{
		std::string json_name = "xag_mingc_" + std::to_string( benchmark_type_each );
		experiments::experiment<std::string, uint32_t, uint32_t, float, uint32_t, float, bool> exp_res( json_name, "benchmark", "gc_before", "gc_after", "improvement %", "iterations", "avg. runtime [s]", "equivalent" );
		uint32_t benchmark_type = benchmark_type_each;
		/* 0u - unoptimized benchmarks; 1u - optimized benchmarks from DATE20; 2u - optimized benchmarks from TCAD22 */
		uint8_t opt = 2u;
		std::cout << "[i] working on " << ( opt ? ( ( opt == 1u ) ? "DATE20 benchmarks\n" : "TCAD22 benchmarks\n" ) : "unoptimized benchmarks\n" );
		auto const benchmarks = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_benchmarks() : mpc_benchmarks() ) : epfl_benchmarks();
		//std::vector<uint32_t> const best_scores  = benchmark_type ? crypto_tcad22() : epfl_tcad22();
		std::vector<uint32_t> const best_scores  = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_date20() : mpc_date20() ) : epfl_date20();

		mockturtle::cut_rewriting_params ps_cut_rewrite;
		ps_cut_rewrite.cut_enumeration_ps.cut_size = 5u;
		ps_cut_rewrite.cut_enumeration_ps.cut_limit = 12u;
		ps_cut_rewrite.verbose = false;
		ps_cut_rewrite.progress = true;
		ps_cut_rewrite.min_cand_cut_size = 2u;

		for ( auto i{ 3u }; i <= 3u; ++i )
		{
			auto const benchmark = benchmarks[i];
			if ( best_scores[i] == 0u )
			{
				std::cout << "[i] skip " << ( opt ? "optimized " : "" ) << benchmark << std::endl;
				continue;
			}

			std::cout << "[i] processing " << ( opt ? "optimized " : "" ) << benchmark << std::endl;

			mockturtle::xag_network xag;
			auto const read_result = lorina::read_verilog( benchmark_path( benchmark_type, benchmark, opt ), mockturtle::verilog_reader( xag ) );
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

			::detail::merge_view xag_merge{ xag };
			uint32_t gc_before = ::detail::count_gc( xag_merge );

			if ( gc_before == 0u )
			{
				exp_res( benchmark, 0u, 0u, 0., 0u, 0., true );
				continue;
			}

			uint32_t gc_after = gc_before - 1u;
			uint32_t const best_score = gc_before;
			uint32_t ite_cnt = 0u;

			mockturtle::xag_mingc_rewrite_params ps;
			ps.verbose = true;
			//ps.verify_database = true;
			mockturtle::xag_mingc_rewrite_stats st;
			mockturtle::xag_mingc_rewrite_stats* pst = &st;
			mockturtle::xag_mingc_rewrite xag_rewrite( "db_gc_complete_5", ps, pst );

			clock_t begin_time = clock();

			while ( gc_before > gc_after )
			{
				if ( ite_cnt > 0u )
				{
					gc_before = gc_after;
				}
				++ite_cnt;

				xag = mockturtle::cut_rewriting_on_scene<mockturtle::xag_network, decltype( xag_rewrite ), ::detail::gc_cost>( xag, xag_rewrite, ps_cut_rewrite );
				//cut_rewriting_with_compatibility_graph( xag, xag_rewrite, ps_cut_rewrite, nullptr, ::detail::num_and<>() );
				//xag = mockturtle::cleanup_dangling( xag );

				::detail::merge_view xag_merge_tmp{ xag };
				gc_after = ::detail::count_gc( xag_merge_tmp );
			}

			mockturtle::write_bench( xag, "/Users/myu/Documents/GitHub/abc/bench" );

			const auto cec = abc_cec( xag, benchmark_type, benchmark, opt );
			assert( cec );
			float improve = ( ( static_cast<float>( best_score ) - static_cast<float>( gc_after ) ) / static_cast<float>( best_score ) ) * 100;
			exp_res( benchmark, best_score, gc_after, improve, ite_cnt, ( float( clock() - begin_time ) / CLOCKS_PER_SEC ) / ite_cnt, cec );
		}

		exp_res.save();
		exp_res.table();
	}

	return 0;
}
