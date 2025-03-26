#include <lorina/aiger.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_minmc.hpp>
#include <mockturtle/algorithms/rewrite.hpp>
#include <mockturtle/algorithms/xag_resub.hpp>
#include <mockturtle/algorithms/xag_resub_withDC.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <mockturtle/views/depth_view.hpp>

#include <experiments.hpp>

struct mc_count
{
  uint32_t operator()( mockturtle::xag_network const& ntk, mockturtle::xag_network::node const& n ) const
  {
    return ntk.is_and( n ) ? 1u : 0u;
  }
};

int main()
{
  using namespace experiments;
  using namespace mockturtle;

  bool resub{ false };

  experiment<std::string, uint32_t, uint32_t, uint32_t> exp( ( resub ? "xag_mc_resub" : "xag_mc_rewrite" ), "benchmark", "#initial_mc", "#mc_old", "#mc_new" );

  xag_npn_resynthesis<xag_network, xag_network, xag_npn_db_kind::xag_complete> xag_area_resyn;
  exact_library<xag_network> xag_area_lib( xag_area_resyn );

  uint32_t mc_init{};
  uint32_t mc_old_tech{};
  uint32_t mc_new_tech{};

  resubstitution_params ps_resub;
  ps_resub.max_divisors = 100u;
  ps_resub.max_inserts = 4u;
  ps_resub.max_pis = 8u;
  ps_resub.progress = false;
  ps_resub.verbose = false;
  ps_resub.use_dont_cares = true;

  xag_minmc_resynthesis xag_mc_resyn( "db.txt" );

  for ( auto const& benchmark : epfl_benchmarks() )
  {
  	fmt::print( "[i] processing {}\n", benchmark );

  	xag_network ntk_init;
  	if ( lorina::read_aiger( benchmark_path( benchmark ), aiger_reader( ntk_init ) ) != lorina::return_code::success )
    {
      continue;
    }

    /* preprocessing benchmarks */
    map_params ps_map;
    ps_map.skip_delay_round = true;
    ps_map.required_time = std::numeric_limits<double>::max();
    map_stats st_map;
    xag_network ntk1 = map( ntk_init, xag_area_lib, ps_map, &st_map );
    rewrite( ntk1, xag_area_lib );
    ntk1 = cleanup_dangling( ntk1 );

    mc_init = 0u;
    mc_old_tech = 0u;
    mc_new_tech = 0u;

    ntk1.foreach_node( [&ntk1, &mc_init]( auto const& n ) {
    	if ( ntk1.is_and( n ) )
    	{
    		++mc_init;
    	}
    } );

    if ( resub )
    {
    	/* Old technique : Resubstitution in DATE20 */
    	fanout_view<xag_network> ntk1_fanout_view{ ntk1 };
    	depth_view<fanout_view<xag_network>> ntk1_resub{ ntk1_fanout_view };
    	resubstitution_minmc_withDC( ntk1_resub, ps_resub );
    	ntk1 = cleanup_dangling( ntk1 );
    }
    else
    {
    	/* Old technique : Rewriting in DATE20 */
    	cut_rewriting_params ps_old_rewrite;
    	ps_old_rewrite.cut_enumeration_ps.cut_size = 6;
    	ps_old_rewrite.cut_enumeration_ps.cut_limit = 12;
    	ps_old_rewrite.verbose = false;
    	ps_old_rewrite.progress = false;
    	ps_old_rewrite.min_cand_cut_size = 2u;
    	cut_rewriting<xag_network, xag_minmc_resynthesis, mc_count>( ntk1, xag_mc_resyn, ps_old_rewrite, nullptr );
    	ntk1 = cleanup_dangling( ntk1 );
    }

    ntk1.foreach_node( [&ntk1, &mc_old_tech]( auto const& n ) {
    	if ( ntk1.is_and( n ) )
    	{
    		++mc_old_tech;
    	}
    } );

    xag_network ntk2 = map( ntk_init, xag_area_lib, ps_map, &st_map );
    rewrite( ntk2, xag_area_lib );

    if ( resub )
    {
    	/* New technique : Resubstitution in DAC23 */
    	fanout_view<xag_network> ntk2_fanout_view{ ntk2 };
    	depth_view<fanout_view<xag_network>> ntk2_resub{ ntk2_fanout_view };
    	xag_resubstitution( ntk2_resub, ps_resub );
    	ntk2 = cleanup_dangling( ntk2 );
    }
    else
    {
    	/* New technique : Rewriting in DAC23 */
    	exact_library<xag_network> xag_mc_lib( xag_mc_resyn );
    	rewrite( ntk2, xag_mc_lib );
    	ntk2 = cleanup_dangling( ntk2 );
    }

    ntk2.foreach_node( [&ntk2, &mc_new_tech]( auto const& n ) {
    	if ( ntk2.is_and( n ) )
    	{
    		++mc_new_tech;
    	}
    } );


    exp( benchmark, mc_init, mc_old_tech, mc_new_tech );
  }

  exp.save();
  exp.table();

  return 0;
}