#include <string>
#include <fmt/format.h>

#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/node_resynthesis/xmg_minmc.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/views/topo_view.hpp>

#include <experiments.hpp>

static const std::string EPFL_benchmarks[] = {
	"adder", "bar", "div", "log2", "max", "multiplier", "sin", "sqrt", "square", "arbiter", 
	"cavlc", "ctrl" , "dec", "i2c", "int2float" , "mem_ctrl", "priority", "router", "voter", 
	"hyp"};

static const std::string CRYPTO_benchmarks[] = {
  "adder_32bit_untilsat", "adder_64bit_untilsat", "AES-expanded_untilsat", "AES-non-expanded_unstilsat", 
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
	for ( auto i = 0u; i < 2u; ++i )
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

std::string benchmark_path( uint32_t benchmark_type, std::string const& benchmark_name, bool opt )
{
	switch( benchmark_type )
	{
	case 0u:
		return fmt::format( "../experiments/{}/{}.aig", ( opt ? "epfl_opt" : "epfl_benchmarks" ), benchmark_name );
	case 1u:
		return fmt::format( "../experiments/{}/{}.aig", ( opt ? "crypto_opt" : "crypto_benchmarks" ), benchmark_name );
	case 2u:
		return fmt::format( "../experiments/{}/{}.aig", ( opt ? "mpc_opt" : "mpc_benchmarks" ), benchmark_name );
	default:
		std::cout << "Unspecified type of benchmark. \n";
		abort();
	}

}

template<class Ntk>
bool abc_cec( Ntk const& ntk, uint32_t const& benchmark_type, std::string const& benchmark, bool opt )
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
template<class Ntk = mockturtle::xmg_network>
struct num_maj
{
	uint32_t operator()( Ntk const& ntk, mockturtle::node<Ntk> const& n ) const
	{
		return ntk.is_maj( n ) ? 1 : 0;
	}
};
}

int main()
{
	experiments::experiment<std::string, bool, uint32_t, uint32_t, float, uint32_t, float, bool> exp_res( "garble_xmg", "benchmark", "optimized", "num_maj_before", "num_maj_after", "improvement %", "iterations", "avg. runtime [s]", "equivalent" );
	uint32_t benchmark_type = 0u; /* 0u - epfl benchmark; 1u - crypto benchmark; 2u - mpc benchmark */
	bool opt = true;
	auto const benchmarks = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_benchmarks() : mpc_benchmarks() ) : epfl_benchmarks();

	for ( auto const& benchmark: benchmarks )
	{
		std::cout << "[i] processing " << ( opt ? "optimized " : "" ) << benchmark << std::endl;

		mockturtle::cut_rewriting_params ps_cut_rew;
		ps_cut_rew.cut_enumeration_ps.cut_size = 5u;
		ps_cut_rew.cut_enumeration_ps.cut_limit = 12u;
		ps_cut_rew.verbose = true;
		ps_cut_rew.progress = true;
		ps_cut_rew.min_cand_cut_size = 2u;

		mockturtle::xmg_network xmg;
		auto const read_result = lorina::read_aiger( benchmark_path( benchmark_type, benchmark, opt ), mockturtle::aiger_reader( xmg ) );
		assert( read_result == lorina::return_code::success );

		uint32_t num_maj = 0u;
		uint32_t num_maj_bfr = 0u;
		uint32_t num_maj_aft = 0u;

		xmg.foreach_gate( [&]( auto f ) {
			if ( xmg.is_maj( f ) )
			{
				++num_maj;
			}
		} );
		num_maj_bfr = num_maj;

		mockturtle::exact_xmg_resynthesis_minmc_params ps_xmg_resyn;
		ps_xmg_resyn.print_stats = true;
		ps_xmg_resyn.cache = std::make_shared<mockturtle::exact_xmg_resynthesis_minmc_params::cache_map_t>();
		ps_xmg_resyn.blacklist_cache = std::make_shared<mockturtle::exact_xmg_resynthesis_minmc_params::blacklist_cache_map_t>();

		mockturtle::exact_xmg_resynthesis_minmc_stats* pst_xmg_resyn = nullptr;
		bool use_db = ( ps_cut_rew.cut_enumeration_ps.cut_size == 5u ) ? false : true;

		mockturtle::exact_xmg_resynthesis_minmc xmg_resyn( "../experiments/db", ps_xmg_resyn, pst_xmg_resyn, use_db );

		uint32_t ite_cnt = 0u;
		const clock_t begin_time = clock();
		while ( num_maj > num_maj_aft )
		{
			++ite_cnt;
			num_maj = num_maj_aft;
			num_maj_aft = 0u;

			mockturtle::cut_rewriting_with_compatibility_graph( xmg, xmg_resyn, ps_cut_rew, nullptr, ::detail::num_maj<mockturtle::xmg_network>() );
			xmg = mockturtle::cleanup_dangling( xmg );

			xmg.foreach_gate( [&]( auto f ) {
				if ( xmg.is_maj( f ) )
				{
					++num_maj_aft;
				}
			} );
		}

		const auto cec = abc_cec( xmg, benchmark_type, benchmark, opt );

		float improve = ( ( num_maj_bfr - num_maj_aft ) / num_maj_bfr ) * 100;

		exp_res( benchmark, opt, num_maj_bfr, num_maj_aft, improve, ite_cnt, ( float( clock() - begin_time ) / CLOCKS_PER_SEC ) / ite_cnt, cec );
	}

	exp_res.save();
	exp_res.table();
	
	return 0;
}