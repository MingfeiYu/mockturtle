#include <string>
#include <fmt/format.h>

#include <mockturtle/networks/xag.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/io/verilog_reader.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/views/topo_view.hpp>
#include <mockturtle/views/fanout_view.hpp>

#include <experiments.hpp>

using merge_view = mockturtle::fanout_view<mockturtle::topo_view<mockturtle::xag_network>>;

static const uint32_t date20_epfl[] = {
	128u, 832u, 5291u, 10913u, 890u, 7653u, 2603u, 5381u, 4672u, 1174u, 394u, 
	45u, 328u, 557u, 85u, 4695u, 323u, 93u, 4257u, 0u};

static const uint32_t date20_crypto[] = {
	32u, 64u, 5440u, 6800u, 92u, 92u, 92u, 92u, 9205u, 9048u, 9367u, 1689u, 
	11515u, 26827u};

static const uint32_t date20_mpc[] = {
	97u, 193u, 232u, 456u, 495u, 975u, 554u, 1162u, 881u, 1919u, 1060u, 2394u, 
	7u, 15u, 21u, 55u, 104u, 275u, 16001u, 58723u};

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

std::vector<std::string> crypto_benchmarks()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 14u; ++i )
	{
		result.emplace_back( CRYPTO_benchmarks[i] );
	}

	return result;
}

std::vector<std::string> epfl_benchmarks()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 20u; ++i )
	{
		result.emplace_back( EPFL_benchmarks[i] );
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

std::string benchmark_path( uint32_t benchmark_type, std::string const& benchmark_name, bool opt )
{
	switch( benchmark_type )
	{
	case 0u:
		return fmt::format( "../experiments/{}/{}.v", ( opt ? "epfl_opt" : "epfl_benchmarks" ), benchmark_name );
	case 1u:
		return fmt::format( "../experiments/{}/{}.v", ( opt ? "crypto_opt" : "crypto_benchmarks" ), benchmark_name );
	case 2u:
		return fmt::format( "../experiments/{}/{}.v", ( opt ? "mpc_opt" : "mpc_benchmarks" ), benchmark_name );
	default:
		std::cout << "Unspecified type of benchmark. \n";
		abort();
	}

}

void count_and_size_rec( merge_view & xag, mockturtle::xag_network::node const& f, mockturtle::xag_network::node const& root )
{
	if ( xag.value( f ) == 0u ) 
	{
		if ( f != root )
		{
			if ( xag.fanout( f ).size() == 1u )
			{
				/* remark this node as traversed */
				xag.incr_value( f );
				/* update the size of this AND clique */
				xag.incr_value( root );
				/* recursively trace its fanin */
				xag.foreach_fanin( f, [&]( auto const& fi ) {
					auto const child = xag.get_node( fi );
					if ( xag.is_and( child ) )
					{
						count_and_size_rec( xag, child, root );
					}
				} );
			}
		}
		else
		{
			xag.incr_value( f );
			xag.incr_value( f );
			/* recursively trace its fanin */
			xag.foreach_fanin( f, [&]( auto const& fi ) {
				auto const child = xag.get_node( fi );
				if ( xag.is_and( child ) )
				{
					count_and_size_rec( xag, child, f );
				}
			} );
		}
	}
}

namespace detail
{
template<class Ntk = mockturtle::xag_network>
struct num_and
{
	uint32_t operator()( Ntk const& ntk, mockturtle::node<Ntk> const& n ) const
	{
		return ntk.is_and( n ) ? 1 : 0;
	}
};
}

int main()
{
	for ( auto benchmark_type_each = 0u; benchmark_type_each <= 2u; ++benchmark_type_each ) 
	{
		std::string json_name = "garble_xag" + std::to_string( benchmark_type_each );
		experiments::experiment<std::string, uint32_t, uint32_t, uint32_t, uint32_t, float, uint32_t, float> exp_res( json_name, "benchmark", "num_and_before", "num_and_after", "garble_cost_before", "garble_cost_after", "improvement %", "maximum fanin size", "avg. runtime [s]" );
		//uint32_t benchmark_type = 0u; /* 0u - epfl benchmark; 1u - crypto benchmark; 2u - mpc benchmark */
		uint32_t benchmark_type = benchmark_type_each;
		bool opt = true;
		auto const benchmarks = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_benchmarks() : mpc_benchmarks() ) : epfl_benchmarks();
		auto const best_scores = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_date20() : mpc_date20() ) : epfl_date20();

		for ( auto i = 0u; i < benchmarks.size(); ++i )
		{
			/*
			if ( i < 2u )
			{
				continue;
			}
			else if ( i > 5u )
			{
				return 0;
			}
			*/


			auto const benchmark = benchmarks[i];
			auto const best_score = best_scores[i];

			std::cout << "[i] processing " << ( opt ? "optimized " : "" ) << benchmark << std::endl;

			mockturtle::xag_network xag;
			
			//auto const read_result = lorina::read_aiger( benchmark_path( benchmark_type, benchmark, opt ), mockturtle::aiger_reader( x1g ) );
			auto const read_result = lorina::read_verilog( benchmark_path( benchmark_type, benchmark, opt ), mockturtle::verilog_reader( xag ) );
			assert( read_result == lorina::return_code::success );
			( void )read_result;

			if ( !opt )
			{
				mockturtle::xag_npn_resynthesis<mockturtle::xag_network> xag_resyn;
				mockturtle::exact_library<mockturtle::xag_network, decltype( xag_resyn ), 4u, ::detail::num_and<mockturtle::xag_network>> lib( xag_resyn );
				xag = mockturtle::map( xag, lib );
			}

			uint32_t num_and_bfr = 0u;
			uint32_t num_and_aft = 0u;
			uint32_t garble_cost_bfr = 0u;
			uint32_t garble_cost_aft = 0u;
			uint32_t max_fan_in = 0u;
			//std::vector<uint32_t> and_size;

			xag.foreach_gate( [&]( auto f ) {
				if ( xag.is_and( f ) )
				{
					++num_and_bfr;
				}
			} );
			num_and_bfr = opt ? best_score : num_and_bfr;
			garble_cost_bfr = num_and_bfr * 2;

			const clock_t begin_time = clock();
			merge_view xag_merge{xag};
			xag_merge.clear_values();

			xag_merge.foreach_gate_reverse( [&]( auto const& f ) {
				//if ( ( xag_merge.is_and( f ) && xag_merge.value( f ) == 0u ) ||
				//		 ( xag_merge.is_and( f ) && xag_merge.fanout( f ).size() > 1u ) )
				if ( xag_merge.is_and( f ) && xag_merge.value( f ) == 0u )
				{
					// this is the root node of a potentially larger AND
					++num_and_aft;
					count_and_size_rec( xag_merge, f, f );
					garble_cost_aft += xag_merge.value( f );

					//and_size.emplace_back( garble_cost_aft_each );
					max_fan_in = ( xag_merge.value( f ) > max_fan_in ) ? xag_merge.value( f ) : max_fan_in;
				}

			} );

			xag_merge.clear_values();

			float improve = ( ( static_cast<float> ( garble_cost_bfr ) - static_cast<float> ( garble_cost_aft ) ) / static_cast<float> ( garble_cost_bfr ) ) * 100;

			exp_res( benchmark, num_and_bfr, num_and_aft, garble_cost_bfr, garble_cost_aft, improve, max_fan_in, ( float( clock() - begin_time ) / CLOCKS_PER_SEC ) );

			/*
			if ( i == 5u )
			{
				exp_res.save();
				exp_res.table();
			}
			*/
		}

		exp_res.save();
		exp_res.table();
	}
	
	return 0;
}
