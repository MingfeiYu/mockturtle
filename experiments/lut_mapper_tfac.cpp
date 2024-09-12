#include <string>
#include <vector>

#include <fmt/format.h>
#include <lorina/aiger.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/algorithms/aig_resub.hpp>
#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/algorithms/lut_map4tfhe.hpp>
#include <mockturtle/algorithms/detect_lut_merging.hpp>
#include <mockturtle/algorithms/rewrite.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/mapping_view.hpp>
#include <mockturtle/utils/tech_library.hpp>
#include <kitty/properties.hpp>

#include <experiments.hpp>

struct lut_cost_tfac
{
  // std::pair<uint32_t, uint32_t> operator()( uint32_t num_leaves ) const
  // {
  //   if ( num_leaves < 2u )
  //     return { 0u, 0u };
  //   return { 1u, 1u }; /* area, delay */
  // }
  std::pair<uint32_t, uint32_t> operator()( kitty::dynamic_truth_table const& tt ) const
  {
    uint8_t num_vars{ static_cast<uint8_t>( tt.num_vars() ) };
    
    if ( num_vars < 2u )
    {
      return { 0u, 0u };
    }
    
    if ( num_vars == 2u )
    {
      return { 1u, 1u };
    }

    if ( num_vars == 3u )
    {
      /* detect symmetry, with input negation considered */
      if ( kitty::is_symmetric( tt ) )
      {
        return { 1u, 1u };
      }
      
      /* detect top xor decomposibility */
      if ( kitty::is_top_xor_decomposible( tt ) )
      {
        return { 1u, 1u };
      }

      /* detect symmetry, with input negation considered */
      if ( std::get<0>( kitty::is_symmetric_n( tt ) ) )
      {
        return { 1u, 1u };
      }
    }

    return { 999u, 1u };
  }
};

int main()
{
  using namespace experiments;
  using namespace mockturtle;

  // experiment<std::string, uint32_t, uint32_t, uint32_t, double, bool> exp( "lut_mapper", "benchmark", "luts", "lut_depth", "edges", "runtime", "equivalent" );
  xag_npn_resynthesis<xag_network, xag_network, xag_npn_db_kind::xag_complete> resyn;
  exact_library<xag_network> exact_lib( resyn );

  for ( auto const& benchmark : epfl_benchmarks() )
  {
    // if ( benchmark != "sin" )
    // {
    //   continue;
    // }
    // if ( ( benchmark == "bar" ) || ( benchmark == "div" ) || ( benchmark == "hyp" ) || ( benchmark == "log2" ) || ( benchmark == "multiplier" ) || ( benchmark == "square" ) || ( benchmark == "arbiter" ) || ( benchmark == "mem_ctrl" ) )
    // {
    //   continue;
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

    // fmt::print( "[i] Baseline gate count: {}\n", ( xag_opt.num_gates() ) );

    /* TODO: Change resub to rewrite? */
    // for ( uint8_t i{ 0u }; i < 3u; ++i )
    // {
    //   aig_resubstitution( aig );
    // }

    lut_map_params ps;
    ps.cut_enumeration_ps.cut_size = 3u;
    ps.cut_enumeration_ps.cut_limit = 16u;
    ps.recompute_cuts = false;
    ps.area_oriented_mapping = true;
    ps.cut_expansion = false;
    ps.area_share_rounds = 0;
    ps.edge_optimization = false;
    ps.verbose = true;
    lut_map_stats st;

    fmt::print( "[i] 3-LUT mapping...\n" );
    klut_network klut_3 = lut_map<xag_network, true, lut_cost_tfac>( xag_opt, ps, &st );


    auto const cec = benchmark == "hyp" ? true : abc_cec( klut_3, benchmark );
    if ( !cec )
    {
      std::cerr << "[e] NON-EQ!\n";
      abort();
    }


    fmt::print( "[i] isolating inverters...\n" );
    // const klut_network klut_3_inv = invert_isolation( klut_3 );
    const klut_network klut_3_inv = invert_isolation_new( klut_3 );
    fmt::print( "[i] detecting merging chances...\n" );
    mergable_luts_map_t mergable_luts_map;
    mockturtle::node_map<mockturtle::label_t, mockturtle::klut_network> node_to_label = detect_lut_merging( klut_3_inv, mergable_luts_map );
    std::string filename = fmt::format( "results_lbf/area/{}.lbf", benchmark );
    mockturtle::klut2lbf_mod( klut_3_inv, mergable_luts_map, node_to_label, filename );

    // lut_map_params ps1;
    // ps1.cut_enumeration_ps.cut_size = 2u;
    // ps1.cut_enumeration_ps.cut_limit = 16u;
    // ps1.recompute_cuts = false; /* also false to test */
    // ps1.area_oriented_mapping = true;
    // ps1.cut_expansion = false;
    // ps1.area_share_rounds = 0;
    // ps1.edge_optimization = false;
    // ps1.verbose = true;
    // lut_map_stats st1;

    // fmt::print( "[i] 2-LUT mapping...\n" );
    // const klut_network klut_2 = lut_map<xag_network, true>( xag_opt, ps1, &st1 );
    // uint32_t num_not{ 0u };
    // klut_2.foreach_gate( [&klut_2, &num_not]( auto const& n ) {
    //   if ( klut_2._storage->nodes[klut_2.node_to_index( n )].children.size() == 1 )
    //   {
    //     ++num_not;
    //   }
    // } );
    // std::cout << "[i] 2-LUT mapping:\n";
    // std::cout << "[i] #BRs: " << klut_2.num_gates() - num_not << "\n\n";

    // exp( benchmark, klut.num_gates(), klut_d.depth(), st.edges, to_seconds( st.time_total ), cec );
  }

  // exp.save();
  // exp.table();

  return 0;
}
