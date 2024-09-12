#include <fmt/format.h>
#include <lorina/aiger.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/algorithms/aig_resub.hpp>
#include <mockturtle/algorithms/lut_mapper.hpp>
#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/algorithms/merge_neighbors.hpp>
#include <mockturtle/algorithms/detect_lut_merging.hpp>
#include <mockturtle/algorithms/rewrite.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/mapping_view.hpp>
#include <mockturtle/utils/tech_library.hpp>

#include <experiments.hpp>

int main()
{
	using namespace experiments;
	using namespace mockturtle;

	xag_npn_resynthesis<xag_network, xag_network, xag_npn_db_kind::xag_complete> resyn;
  exact_library<xag_network> exact_lib( resyn );

  for ( auto const& benchmark : epfl_benchmarks() )
  {
    // if ( ( benchmark == "bar" ) || ( benchmark == "div" ) || ( benchmark == "hyp" ) || ( benchmark == "log2" ) || ( benchmark == "multiplier" ) || ( benchmark == "square" ) || ( benchmark == "arbiter" ) || ( benchmark == "mem_ctrl" ) )
    // if ( benchmark != "adder" )
    // {
      // continue;
    // }
    fmt::print( "[i] processing {}\n", benchmark );

    xag_network aig;
    if ( lorina::read_aiger( benchmark_path( benchmark ), aiger_reader( aig ) ) != lorina::return_code::success )
    // if ( lorina::read_aiger( "fuzz.aig", aiger_reader( aig ) ) != lorina::return_code::success )
    {
      continue;
      // return 1;
    }

    // OPT
    xag_network xag_opt;
    {
      map_params ps1;
      ps1.skip_delay_round = true;
      ps1.required_time = std::numeric_limits<double>::max();
      map_stats st1;
      xag_network xag = map( aig, exact_lib, ps1, &st1 );

      rewrite( xag, exact_lib );
      xag_opt = xag;
    }

    fmt::print( "[i] area-oriented LUT mapping...\n" );
    lut_map_params ps2;
    ps2.cut_enumeration_ps.cut_size = 2u;
    ps2.cut_enumeration_ps.cut_limit = 16u;
    ps2.recompute_cuts = false;
    ps2.area_oriented_mapping = true;
    ps2.cut_expansion = false;
    ps2.area_share_rounds = 0;
    ps2.edge_optimization = false;
    ps2.verbose = false;
    lut_map_stats st2;
    klut_network klut_2 = lut_map<xag_network, true>( xag_opt, ps2, &st2 );

    merge_neighbor_params ps;
    ps.verbose = false;
    merge_neighbor_stats st;
    fmt::print( "[i] merging neighbors...\n" );
    klut_network klut_3 = merge_neighbors<klut_network>( klut_2, ps, &st );
    std::cout << fmt::format( "[i] Total runtime           = {:>5.2f} secs\n", ( mockturtle::to_seconds( st.runtime ) + mockturtle::to_seconds( st2.time_total ) ) );

    auto const cec = benchmark == "hyp" ? true : abc_cec( klut_3, benchmark );
    if ( !cec )
    {
      std::cerr << "[e] NON-EQ!\n";
      abort();
    }

    fmt::print( "[i] isolating inverters...\n" );
    const klut_network klut_3_inv = invert_isolation( klut_3 );
    fmt::print( "[i] detecting merging chances...\n" );
    mergable_luts_map_t mergable_luts_map;
    mockturtle::node_map<mockturtle::label_t, mockturtle::klut_network> node_to_label = detect_lut_merging( klut_3_inv, mergable_luts_map );
    std::string filename = fmt::format( "results_lbf/autohog/{}.lbf", benchmark );
    mockturtle::klut2lbf_mod( klut_3_inv, mergable_luts_map, node_to_label, filename );
  }

  return 0;
}