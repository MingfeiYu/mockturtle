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

static const uint32_t mc_npn_4[] = {
	0u, 3u, 2u, 2u, 3u, 1u, 3u, 2u, 2u, 3u, 2u, 2u, 3u, 1u, 3u, 2u, 1u, 3u, 
	2u, 2u, 3u, 0u, 2u, 3u, 3u, 2u, 2u, 3u, 3u, 2u, 2u, 3u, 2u, 3u, 2u, 2u, 
	3u, 2u, 3u, 3u, 2u, 2u, 3u, 3u, 2u, 2u, 3u, 3u, 2u, 2u, 3u, 2u, 3u, 2u, 
	3u, 2u, 2u, 3u, 3u, 2u, 2u, 3u, 2u, 3u, 3u, 2u, 3u, 2u, 2u, 3u, 2u, 3u, 
	3u, 2u, 3u, 2u, 2u, 3u, 3u, 2u, 2u, 2u, 3u, 1u, 2u, 3u, 3u, 2u, 2u, 3u, 
	3u, 2u, 3u, 2u, 2u, 3u, 2u, 3u, 3u, 2u, 3u, 2u, 2u, 1u, 3u, 2u, 2u, 2u, 
	3u, 1u, 2u, 3u, 3u, 2u, 2u, 3u, 2u, 3u, 2u, 2u, 1u, 1u, 3u, 3u, 2u, 2u, 
	3u, 2u, 3u, 1u, 2u, 3u, 3u, 2u, 3u, 3u, 2u, 2u, 1u, 3u, 2u, 2u, 3u, 1u, 
	3u, 2u, 2u, 3u, 2u, 3u, 3u, 2u, 3u, 2u, 2u, 3u, 3u, 1u, 1u, 2u, 3u, 2u, 
	2u, 3u, 2u, 3u, 3u, 2u, 2u, 2u, 2u, 3u, 3u, 2u, 2u, 2u, 3u, 2u, 2u, 2u, 
	0u, 2u, 3u, 3u, 2u, 2u, 3u, 3u, 2u, 2u, 3u, 2u, 3u, 3u, 3u, 2u, 2u, 3u, 
	3u, 2u, 2u, 3u, 3u, 2u, 2u, 2u, 2u, 1u, 2u, 3u, 2u, 2u, 1u, 1u, 3u, 2u, 
	2u, 1u, 1u, 1u, 0u, 0u};

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

std::vector<uint32_t> npn_4_mc()
{
	std::vector<uint32_t> best_score;
	for ( auto i = 0u; i < 221u; ++i )
	{
		best_score.emplace_back( mc_npn_4[i] );
	}
	return best_score;
}

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

static const std::string npn_4_input[] = {
  "0000", "0001", "0003", "0006", "0007", "000f", "0016", "0017", "0018", "0019", "001b", "001e", "001f", "003c", 
	"003d", "003f", "0069", "006b", "006f", "007e", "007f", "00ff", "0116", "0117", "0118", "0119", "011a", "011b", 
	"011e", "011f", "012c", "012d", "012f", "013c", "013d", "013e", "013f", "0168", "0169", "016a", "016b", "016e", 
	"016f", "017e", "017f", "0180", "0181", "0182", "0183", "0186", "0187", "0189", "018b", "018f", "0196", "0197", 
	"0198", "0199", "019a", "019b", "019e", "019f", "01a8", "01a9", "01aa", "01ab", "01ac", "01ad", "01ae", "01af", 
	"01bc", "01bd", "01be", "01bf", "01e8", "01e9", "01ea", "01eb", "01ee", "01ef", "01fe", "033c", "033d", "033f", 
	"0356", "0357", "0358", "0359", "035a", "035b", "035e", "035f", "0368", "0369", "036a", "036b", "036c", "036d", 
	"036e", "036f", "037c", "037d", "037e", "03c0", "03c1", "03c3", "03c5", "03c6", "03c7", "03cf", "03d4", "03d5", 
	"03d6", "03d7", "03d8", "03d9", "03db", "03dc", "03dd", "03de", "03fc", "0660", "0661", "0662", "0663", "0666", 
	"0667", "0669", "066b", "066f", "0672", "0673", "0676", "0678", "0679", "067a", "067b", "067e", "0690", "0691", 
	"0693", "0696", "0697", "069f", "06b0", "06b1", "06b2", "06b3", "06b4", "06b5", "06b6", "06b7", "06b9", "06bd", 
	"06f0", "06f1", "06f2", "06f6", "06f9", "0776", "0778", "0779", "077a", "077e", "07b0", "07b1", "07b4", "07b5", 
	"07b6", "07bc", "07e0", "07e1", "07e2", "07e3", "07e6", "07e9", "07f0", "07f1", "07f2", "07f8", "0ff0", "1668", 
	"1669", "166a", "166b", "166e", "167e", "1681", "1683", "1686", "1687", "1689", "168b", "168e", "1696", "1697", 
	"1698", "1699", "169a", "169b", "169e", "16a9", "16ac", "16ad", "16bc", "16e9", "177e", "178e", "1796", "1798", 
	"179a", "17ac", "17e8", "18e7", "19e1", "19e3", "19e6", "1bd8", "1be4", "1ee1", "3cc3", "6996"};

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

std::vector<std::string> npn_4()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 222u; ++i )
	{
		result.emplace_back( npn_4_input[i] );
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

std::string benchmark_path( uint32_t benchmark_type, std::string const& benchmark_name, uint8_t opt )
{
	switch( benchmark_type )
	{
	case 0u:
		return fmt::format( "../experiments/{}/{}.v", ( opt ? ( ( opt == 1u ) ? "epfl_opt" : "epfl_tcad22" ) : "epfl_benchmarks" ), benchmark_name );
	case 1u:
		return fmt::format( "../experiments/{}/{}.v", ( opt ? ( ( opt == 1u ) ? "crypto_opt" : "crypto_tcad22" ) : "crypto_benchmarks" ), benchmark_name );
	case 2u:
		return fmt::format( "../experiments/{}/{}.v", ( opt ? ( ( opt == 1u ) ? "mpc_opt" : "4-input-npn-xag" ) : "mpc_benchmarks" ), benchmark_name );
	default:
		std::cout << "Unspecified type of benchmark. \n";
		abort();
	}
}

void count_and_size_rec( merge_view& xag, mockturtle::xag_network::node const& f, mockturtle::xag_network::node const& root )
{
	if ( xag.value( f ) == 0u ) 
	{
		if ( f != root )
		{
			//std::cout << "with fanout: "; 
			//for ( auto const fanout_each: xag.fanout( f ) )
			//{
			//	std::cout << fanout_each << ", ";
			//}
			//std::cout << "(" << xag.fanout( f ).size() << ")\n";

			if ( xag.fanout( f ).size() == 1u )
			{
				//std::cout << "AND clique larger than 2 detected! \n";
				/* remark this node as traversed */
				xag.incr_value( f );
				/* update the size of this AND clique */
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
			//std::cout << "Root node " << f << ":\n";
			xag.set_value( f, 2u );
			/* recursively trace its fanin */
			xag.foreach_fanin( f, [&]( auto const& fi ) {
				auto const child = xag.get_node( fi );
				if ( xag.is_and( child ) && !xag.is_complemented( fi ) )
				//if ( xag.is_and( child ) )
				{
					//std::cout << "child node " << child << " is an AND, ";
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
		return ntk.is_and( n ) ? 1u : 0u;
	}
};
}

int main()
{
	bool test = true; 
	if ( !test )
	{
	for ( auto benchmark_type_each = 0u; benchmark_type_each <= 1u; ++benchmark_type_each ) 
	{
		std::string json_name = "garble_xag" + std::to_string( benchmark_type_each );
		experiments::experiment<std::string, uint32_t, uint32_t, uint32_t, uint32_t, float, uint32_t, float> exp_res( json_name, "benchmark", "num_and_before", "num_and_after", "garble_cost_before", "garble_cost_after", "improvement %", "maximum fanin size", "avg. runtime [s]" );
		//uint32_t benchmark_type = 0u; /* 0u - epfl benchmark; 1u - crypto benchmark; 2u - mpc benchmark */
		uint32_t benchmark_type = benchmark_type_each;
		uint8_t opt = 1u;
		std::cout << "[i] working on " << ( opt ? ( ( opt == 1u ) ? "DATE20 benchmarks\n" : "TCAD22 benchmarks\n" ) : "unoptimized benchmarks\n" );
		auto const benchmarks = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_benchmarks() : npn_4() ) : epfl_benchmarks();
		auto const best_scores = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_tcad22() : npn_4_mc() ) : epfl_tcad22();

		for ( auto i = 0u; i < benchmarks.size(); ++i )
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
			
			//auto const read_result = lorina::read_aiger( benchmark_path( benchmark_type, benchmark, opt ), mockturtle::aiger_reader( x1g ) );
			auto const read_result = lorina::read_verilog( benchmark_path( benchmark_type, benchmark, opt ), mockturtle::verilog_reader( xag ) );
			assert( read_result == lorina::return_code::success );
			( void )read_result;

			/* for non-optimized benchmarks, operate a round of cut-rewriting */
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
			mockturtle::topo_view<mockturtle::xag_network> xag_topo{xag};
			mockturtle::fanout_view<mockturtle::topo_view<mockturtle::xag_network>> xag_merge{xag_topo};
			xag_merge.clear_values();

			xag_merge.foreach_gate_reverse( [&]( auto const& f ) {
				//if ( ( xag_merge.is_and( f ) && ( xag_merge.fanout( f ).size() > 1u || 
				//																	xag_merge.value( f ) == 0u ) ) )
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
		}

		exp_res.save();
		exp_res.table();
	}
	}
	else
	{
		std::cout << "[i] processing test case\n";

		mockturtle::xag_network xag;
		//auto const read_result = lorina::read_verilog( "../experiments/epfl_opt/test.v", mockturtle::verilog_reader( xag ) );
		//auto const read_result = lorina::read_verilog( "../experiments/crypto_tcad22/AES-expanded_untilsat.v", mockturtle::verilog_reader( xag ) );
		auto const read_result = lorina::read_verilog( "../experiments/epfl_opt/mem_ctrl.v", mockturtle::verilog_reader( xag ) );
		assert( read_result == lorina::return_code::success );
		( void )read_result;
		
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
		garble_cost_bfr = num_and_bfr * 2;

		mockturtle::topo_view<mockturtle::xag_network> xag_topo{xag};
		mockturtle::fanout_view<mockturtle::topo_view<mockturtle::xag_network>> xag_merge{xag_topo};

		xag_merge.clear_values();

		xag_merge.foreach_gate_reverse( [&]( auto const& f ) {
				//if ( ( xag_merge.is_and( f ) && ( xag_merge.fanout( f ).size() > 1u || 
				//																	xag_merge.value( f ) == 0u ) ) )
			if ( xag_merge.is_and( f ) && xag_merge.value( f ) == 0u )
			{
				// this is the root node of a potentially larger AND
				++num_and_aft;
				count_and_size_rec( xag_merge, f, f );
				garble_cost_aft += xag_merge.value( f );

				//and_size.emplace_back( xag_merge.value( f ) );
				max_fan_in = ( xag_merge.value( f ) > max_fan_in ) ? xag_merge.value( f ) : max_fan_in;
			}

		} );

		xag_merge.clear_values();
		std::cout << "Garbling cost before: " << garble_cost_bfr << ", ";
		std::cout << "after: " << garble_cost_aft << "\n";
		std::cout << num_and_aft << " cliques detected, ";
		std::cout << "the largest is with size: " << max_fan_in << "\n";
		//std::cout << "Here are each clique's size: ";
		//for (auto and_size_each: and_size)
		//{
		//	std::cout << and_size_each << ", ";
		//}
		std::cout << std::endl;
	}

	return 0;
}