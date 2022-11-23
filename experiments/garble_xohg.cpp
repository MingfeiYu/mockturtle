#include <string>
#include <fmt/format.h>

#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/networks/x1g.hpp>
#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/algorithms/node_resynthesis/xohg_minmc.hpp>
#include <mockturtle/algorithms/node_resynthesis/x1g_npn.hpp>
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
	for ( auto i = 0u; i < 1u; ++i )
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
template<class Ntk = mockturtle::xag_network>
struct num_and
{
	uint32_t operator()( Ntk const& ntk, mockturtle::node<Ntk> const& n ) const
	{
		return ntk.is_and( n ) ? 1 : 0;
	}
};

template<class Ntk = mockturtle::x1g_network>
struct num_oh
{
	uint32_t operator()( Ntk const& ntk, mockturtle::node<Ntk> const& n ) const
	{
		return ntk.is_onehot( n ) ? 1 : 0;
	}
};

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
	experiments::experiment<std::string, bool, uint32_t, uint32_t, float, uint32_t, float, bool> exp_res( "garble_xohg", "benchmark", "optimized", "num_oh_before", "num_oh_after", "improvement %", "iterations", "avg. runtime [s]", "equivalent" );
	uint32_t benchmark_type = 0u; /* 0u - epfl benchmark; 1u - crypto benchmark; 2u - mpc benchmark */
	bool opt = true;
	auto const benchmarks = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_benchmarks() : mpc_benchmarks() ) : epfl_benchmarks();

	for ( auto const& benchmark: benchmarks )
	{
		std::cout << "[i] processing " << ( opt ? "optimized " : "" ) << benchmark << std::endl;

		mockturtle::cut_rewriting_params ps_cut_rew;
		ps_cut_rew.cut_enumeration_ps.cut_size = 4u;
		ps_cut_rew.cut_enumeration_ps.cut_limit = 12u;
		ps_cut_rew.verbose = true;
		ps_cut_rew.progress = true;
		ps_cut_rew.min_cand_cut_size = 2u;

		mockturtle::x1g_network x1g;
		/* Obtain initial X1G from optimized XAGs                                  */
		/* ( if there is no optimized XAGs available, but only optimization flow ) */
    /*
    using sig_x1g = typename mockturtle::x1g_network::signal;

    // derive signals in the targeted X1G from nodes in the original XAG
    auto const read_result = lorina::read_verilog( benchmark_path( benchmark_type, benchmark ), mockturtle::verilog_reader( xag ) );
		assert( read_result == lorina::return_code::success );
    node_map<sig_x1g, mockturtle::xag_network> xag_node2x1g_sig( xag );

    // create const
    xag_node2x1g_sig[xag.get_node( xag.get_constant( false ) )] = x1g.get_constant( false );

    // create pis
    xag.foreach_pi( [&]( auto n ) {
      xag_node2x1g_sig[n] = x1g.create_pi();
    } );
    
    // create OneHots and XOR-3s
    mockturtle::topo_view xag_topo{xag};
    xag_topo.foreach_node( [&]( auto n ) {
      if ( xag.is_constant( n ) || xag.is_pi( n ) )
        return;
      std::vector<sig_x1g> children;
      xag.foreach_fanin( n, [&]( auto const& f ) {
        children.push_back( xag.is_complemented( f ) ? x1g.create_not( xag_node2x1g_sig[f] ) : xag_node2x1g_sig[f] );
      } );

      assert( children.size() == 2u );
      if ( xag.is_and( n ) )
      {
        xag_node2x1g_sig[n] = x1g.create_and( children[0], children[1] );
      }
      else if ( xag.is_xor( n ) )
      {
        xag_node2x1g_sig[n] = x1g.create_xor( children[0], children[1] );
      }
      else
      {
        std::cerr << "Unknown gate type detected! \n";
        abort();
      }
    } );
    
    // create pos
    xag.foreach_po( [&]( auto const& f ) {
      auto const o = xag.is_complemented( f ) ? x1g.create_not( xag_node2x1g_sig[f] ) : xag_node2x1g_sig[f];
      x1g.create_po( o );
    } );
    */

		/* xohg optimization */
		

		/* Obtain initial X1G by reading in benchmarks      */
		/* ( if optimized XAGs are provided as benchmarks ) */
		auto const read_result = lorina::read_aiger( benchmark_path( benchmark_type, benchmark, opt ), mockturtle::aiger_reader( x1g ) );
		assert( read_result == lorina::return_code::success );

		uint32_t num_oh = 0u;
		uint32_t num_oh_bfr = 0u;
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
		num_oh_bfr = num_oh;
		num_oh_aft = num_oh - 1u;

		clock_t begin_time;
		uint32_t ite_cnt = 0u;
		if ( ps_cut_rew.cut_enumeration_ps.cut_size > 4u )
		{
			mockturtle::exact_xohg_resynthesis_minmc_params ps_xohg_resyn;
			ps_xohg_resyn.print_stats = true;
			ps_xohg_resyn.conflict_limit = 1000000u;
			ps_xohg_resyn.cache = std::make_shared<mockturtle::exact_xohg_resynthesis_minmc_params::cache_map_t>();
			ps_xohg_resyn.blacklist_cache = std::make_shared<mockturtle::exact_xohg_resynthesis_minmc_params::blacklist_cache_map_t>();

			mockturtle::exact_xohg_resynthesis_minmc_stats* pst_xohg_resyn = nullptr;
			bool use_db = ( ps_cut_rew.cut_enumeration_ps.cut_size == 5u ) ? false : true;

			mockturtle::exact_xohg_resynthesis_minmc xohg_resyn( "../experiments/db", ps_xohg_resyn, pst_xohg_resyn, use_db );

			begin_time = clock();
			while ( num_oh > num_oh_aft )
			{
				if ( ite_cnt > 0u )
				{
					num_oh = num_oh_aft;
				}
				++ite_cnt;
				num_oh_aft = 0u;

				mockturtle::cut_rewriting_with_compatibility_graph( x1g, xohg_resyn, ps_cut_rew, nullptr, ::detail::num_oh<mockturtle::x1g_network>() );

				x1g = mockturtle::cleanup_dangling( x1g );

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


				//mockturtle::cut_rewriting( x1g, x1g_resyn, ps_cut_rew, nullptr );
				
				//mockturtle::cut_rewriting_with_compatibility_graph( x1g, x1g_resyn, ps_cut_rew, nullptr, ::detail::num_oh<mockturtle::x1g_network>() );
    		
    		mockturtle::exact_library_params _exact_lib_params;
    		_exact_lib_params.verbose = true;
    		_exact_lib_params.np_classification = false;
    		mockturtle::exact_library<mockturtle::x1g_network, decltype( x1g_resyn )> lib( x1g_resyn, _exact_lib_params );
    		x1g = mockturtle::map( x1g, lib );

				//x1g = mockturtle::cleanup_dangling( x1g );

				x1g.foreach_gate( [&]( auto f ) {
					if ( x1g.is_onehot( f ) )
					{
						++num_oh_aft;
					}
				} );
			}
		}

		const auto cec = abc_cec( x1g, benchmark_type, benchmark, opt );
		//assert( cec );

		float improve = ( ( static_cast<float> ( num_oh_bfr ) - static_cast<float> ( num_oh_aft ) ) / static_cast<float> ( num_oh_bfr ) ) * 100;

		exp_res( benchmark, opt, num_oh_bfr, num_oh_aft, improve, ite_cnt, ( float( clock() - begin_time ) / CLOCKS_PER_SEC ) / ite_cnt, cec );
	}

	exp_res.save();
	exp_res.table();
	
	return 0;
}
