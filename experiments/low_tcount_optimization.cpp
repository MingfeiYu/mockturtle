#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/functional_reduction.hpp>
#include <mockturtle/algorithms/merge_split_xag.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_low_tcount_rewrite.hpp>
#include <mockturtle/algorithms/rewriting.hpp>
#include <mockturtle/io/verilog_reader.hpp>
#include <mockturtle/io/write_bench.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <mockturtle/views/topo_view.hpp>

#include <experiments.hpp>

static const uint32_t tcad22_epfl[] = {
	512u, 3328u, 20144u, 34532u, 3406u, 30194u, 7642u, 20610u, 18244u, 4608u, 1444u, 
	172u, 1312u, 2118u, 314u, 17690u, 1238u, 312u, 16362u };

std::vector<uint32_t> epfl_tcad22()
{
	std::vector<uint32_t> best_score;
	for( auto i = 0u; i < 19u; ++i )
	{
		best_score.emplace_back( tcad22_epfl[i] );
	}
	return best_score;
}

static const std::string EPFL_benchmarks[] = {
	"adder", "bar", "div", "log2", "max", "multiplier", "sin", "sqrt", "square", "arbiter", 
	"cavlc", "ctrl" , "dec", "i2c", "int2float" , "mem_ctrl", "priority", "router", "voter" };

std::vector<std::string> epfl_benchmarks()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 19u; ++i )
	{
		result.emplace_back( EPFL_benchmarks[i] );
	}

	return result;
}

static const uint32_t tcad22_crypto[] = {
	21120u, 26400u, 27126u, 26860u, 306u, 326u, 306u, 326u,37468u, 45800u, 104760u };

std::vector<uint32_t> crypto_tcad22()
{
	std::vector<uint32_t> best_score;
	for( auto i = 0u; i < 11u; ++i )
	{
		best_score.emplace_back( tcad22_crypto[i] );
	}
	return best_score;
}

static const std::string CRYPTO_benchmarks[] = {
	"AES-expanded_untilsat", "AES-non-expanded_untilsat", "DES-expanded_untilsat", "DES-non-expanded_untilsat", 
  "comparator_32bit_signed_lt_untilsat", "comparator_32bit_signed_lteq_untilsat", "comparator_32bit_unsigned_lt_untilsat", "comparator_32bit_unsigned_lteq_untilsat", 
  "md5_untilsat", "sha-1_untilsat", "sha-256_untilsat" };

std::vector<std::string> crypto_benchmarks()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 11u; ++i )
	{
		result.emplace_back( CRYPTO_benchmarks[i] );
	}

	return result;
}

std::string benchmark_path( std::string const& benchmark_name, bool epfl )
{
	return fmt::format( "../experiments/{}/{}.v", ( epfl ? "epfl_tcad22" : "crypto_tcad22" ), benchmark_name );
}

template<class Ntk>
bool abc_cec( Ntk const& ntk, std::string const& benchmark, bool epfl )
{
	mockturtle::write_bench( ntk, "/tmp/test.bench" );
	std::string abc_path = "/Users/myu/Documents/GitHub/abc/";
	std::string command = fmt::format( "{}abc -q \"cec -n {} /tmp/test.bench\"", abc_path, benchmark_path( benchmark, epfl ) );

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

using merge_view = mockturtle::fanout_view<mockturtle::topo_view<mockturtle::xag_network>>;

void recursive_t_count_cal( merge_view const& xag_merge, merge_view::node const& n, merge_view::node const& root, std::vector<std::uint32_t>& counted )
{
	if ( xag_merge.value( n ) && !counted[n - 1] ) 
	{
		if ( n != root )
		{
			bool fanout_size_is_1 = ( xag_merge.fanout_size( n ) == 1u );
			if ( !fanout_size_is_1 )
			{
				uint32_t fanout_size{ 0u };
				//std::cout << "[e] fanouts of node " << n << " are: ";
				xag_merge.foreach_fanout( n, [&]( auto const& no ) {
					if ( xag_merge.value( no ) )
					{
						++fanout_size;
						//std::cout << "node " << n << " ";
					}
				} );
				//std::cout << "\n";
				fanout_size_is_1 = ( fanout_size == 1u );
			}
			if ( fanout_size_is_1 )
			//if ( xag_merge.fanout_size( n ) == 1u )
			{
				/* remark this node as traversed */
				counted[n - 1] = 1u;
				/* update the size of this AND clique */
				counted[root - 1] += 1u;
			}
			//else
			//{
			//	std::cout << "[m] meet the border at node " << n << ", which has more than one fanout\n";
			//}
		}
		else
		{
			counted[n - 1] = 2u;
		}

		/* recursively trace its fanin */
		xag_merge.foreach_fanin( n, [&]( auto const& ni ) {
			auto const child = xag_merge.get_node( ni );
			if ( xag_merge.value( child ) && xag_merge.is_and( child ) && !xag_merge.is_complemented( ni ) )
			{
				//std::cout << "[e] proceed from node " << n << " to its fanin node " << child << "\n";
				recursive_t_count_cal( xag_merge, child, root, counted );
			}
			//else
			//{
			//	std::cout << "[m] meet the border at node " << child << ", which is not AND or inactive\n";
			//}
		} );
	}
}

struct t_count_cal
{
	uint32_t operator()( mockturtle::xag_network const& xag ) const
	{
		uint32_t t_count{ 0u };
		/* const-0 is not taken into consideration */
		std::vector<uint32_t> counted( xag._storage->nodes.size() - 1 );
		mockturtle::topo_view<mockturtle::xag_network> xag_topo{ xag };
		merge_view xag_merge{ xag_topo };

		xag_merge.foreach_gate_reverse( [&]( auto const& n ) {
			if ( xag_merge.is_and( n ) && xag_merge.value( n ) && !counted[n - 1] )
			{
				recursive_t_count_cal( xag_merge, n, n, counted );
				uint32_t fanin_size = counted[n - 1];
				while ( fanin_size > 2u )
				{
					t_count += 6u;
					fanin_size -= 2u;
				}
				if ( fanin_size > 1u )
				{
					t_count += 4u;
					--fanin_size;
				}
			}
		} );

		return t_count;
	}
};

struct num_and
{
	uint32_t operator()( mockturtle::xag_network const& xag, mockturtle::xag_network::node n ) const
	{
		return ( xag.is_and( n ) ? 1u : 0u );
	}
};

uint32_t num_xor( mockturtle::xag_network const& xag )
{
	uint32_t num_xor{ 0u };
	xag.foreach_gate( [&]( auto const& n ) {
		if ( xag.is_xor( n ) )
		{
			++num_xor;
		}
	} );

	return num_xor;
}

using merge_view = mockturtle::fanout_view<mockturtle::topo_view<mockturtle::xag_network>>;

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

uint32_t count_gc( mockturtle::xag_network const& xag )
{
	uint32_t gc{ 0u };
	mockturtle::topo_view<mockturtle::xag_network> xag_topo{ xag };
	merge_view xag_merge{ xag_topo };
	xag_merge.clear_values();

	xag_merge.foreach_gate_reverse( [&]( auto const& f ) {
		if ( xag_merge.is_and( f ) && xag_merge.value( f ) == 0u )
		{
			/* The root node of a potentially larger AND */
			count_and_size_rec( xag_merge, f, f );
			gc += xag_merge.value( f );
		}
	} );
	xag_merge.clear_values();

	return gc;
}


} /* namespace detail */

int main()
{
	//std::string json_name = "tcount_opt_epfl";
	//std::string json_name = "tcount_opt_epfl_strict";
	std::string json_name = "tcount_opt_epfl_cut_filter_vote";
	experiments::experiment<std::string, uint32_t, uint32_t, uint32_t, uint32_t, float, uint32_t, float, bool> exp_res( json_name, "benchmark", "T-count_before", "T-count_after", "XOR_before", "XOR_after", "improvement %", "iterations", "rewriting time [s]", "equivalent" );
	bool epfl{ true };
	std::cout << "[i] working on " << ( epfl ? "EPFL benchmarks\n" : "CRYPTO benchmarks\n" );
	auto const benchmarks = epfl ? epfl_benchmarks() : crypto_benchmarks();
	std::vector<uint32_t> const best_scores  = epfl ? epfl_tcad22() : crypto_tcad22();

	mockturtle::rewrite_params ps_cut_rewrite;
	ps_cut_rewrite.cut_enumeration_ps.cut_size = 5u;
	ps_cut_rewrite.cut_enumeration_ps.cut_limit = 12u;
	ps_cut_rewrite.verbose = false;
	ps_cut_rewrite.progress = true;
	ps_cut_rewrite.min_cand_cut_size = 2u;
	ps_cut_rewrite.allow_zero_gain = true;

	for ( auto i{ 0u }; i < benchmarks.size(); ++i )
	{
		auto const benchmark = benchmarks[i];
		if ( best_scores[i] == 0u )
		{
			std::cout << "[i] skip " << benchmark << std::endl;
			continue;
		}

		std::cout << "[i] processing " << benchmark << std::endl;

		mockturtle::xag_network xag;

		auto const read_result = lorina::read_verilog( benchmark_path( benchmark, epfl ), mockturtle::verilog_reader( xag ) );
		assert( read_result == lorina::return_code::success );

		if ( read_result == lorina::return_code::success )
		{
			std::cout << "[i] successfully read " << benchmark << std::endl;
		}
		else
		{
			std::cout << "[i] failed to read " << benchmark << std::endl;
		}
		uint32_t t_count_before = best_scores[i];
		uint32_t XOR_before = detail::num_xor( xag );

		//uint32_t t_count_before = mockturtle::merge_split_xag( xag );
		//std::cout << "[i] The T-count of the starting point XAG is " << t_count_before << "\n";

		if ( t_count_before == 0u )
		{
			exp_res( benchmark, 0u, 0u, 0u, 0u, 0., 0u, 0., true );
			continue;
		}

		uint32_t const best_score = t_count_before;
		uint32_t t_count_after = t_count_before - 1u;
		uint32_t ite_cnt = 0u;

		mockturtle::xag_low_tcount_rewrite_params ps;
		ps.verbose = true;
		//ps.verify_database = true;
		mockturtle::xag_low_tcount_rewrite_stats st;
		mockturtle::xag_low_tcount_rewrite_stats* pst = &st;
		//mockturtle::xag_low_tcount_rewrite xag_rewrite( "db_tcount_5_mc", ps, pst );
		mockturtle::xag_low_tcount_rewrite xag_rewrite( "db_tcount_5_mc_xor_opt", ps, pst );
		//mockturtle::xag_low_tcount_rewrite xag_rewrite( "db_tcount_5_xor_opt", ps, pst );

		clock_t begin_time = clock();

		//std::cout << "[i] before optimization, T-count is " <<  mockturtle::merge_split_xag( xag ) << "\n";

		while ( t_count_before > t_count_after )
		{
			if ( ite_cnt > 0u )
			{
				t_count_before = t_count_after;
			}
			++ite_cnt;

			mockturtle::xag_network xag_new = mockturtle::rewrite<mockturtle::xag_network, decltype( xag_rewrite ), detail::num_and>( xag, xag_rewrite, ps_cut_rewrite );
			t_count_after = mockturtle::merge_split_xag( xag_new );
			xag = ( t_count_after > t_count_before ) ? xag : xag_new;
			//std::cout << "[i] After " << ite_cnt << " round, T count is " << t_count_before << "\n";
		}

		float rewriting_time = float( clock() - begin_time ) / CLOCKS_PER_SEC;
		t_count_after = mockturtle::merge_split_xag( xag );
		uint32_t XOR_after = detail::num_xor( xag );
		const auto cec = abc_cec( xag, benchmark, epfl );
		assert( cec );
		float improve = ( ( static_cast<float>( best_score ) - static_cast<float>( t_count_after ) ) / static_cast<float>( best_score ) ) * 100;
		exp_res( benchmark, best_score,t_count_after, XOR_before, XOR_after, improve, ite_cnt, rewriting_time, cec );
		mockturtle::write_verilog( xag, ( "../experiments/benchmarks/cut_filter_vote/" + benchmark + ".v" ) );
	}

	exp_res.save();
	exp_res.table();

	return 0;
}