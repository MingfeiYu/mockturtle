#include <cstdint>
#include <fstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <kitty/constructors.hpp>
#include <kitty/operations.hpp>
#include <mockturtle/algorithms/optimum_gc_synthesis.hpp>
#include <mockturtle/algorithms/xag2x1g.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/x1g.hpp>

namespace mockturtle
{

typedef struct AND_topo
{
	std::vector<uint32_t> num_ands;
	uint32_t mc;
	uint32_t gc;
} AND_topo;

/* X1G-oriented topos */
static std::vector<AND_topo> and_topos_lib_6_x1g_oriented = {
	{ { 1u }, 1u, 2u }, 
	{ { 2u }, 2u, 2u }, 
	{ { 1u, 1u }, 2u, 4u }, 
	{ { 3u }, 3u, 4u }, 
	{ { 1u, 2u }, 3u, 4u }, 
	{ { 2u, 1u }, 3u, 4u }, 
	{ { 4u }, 4u, 4u }, 
	{ { 2u, 2u }, 4u, 4u }, 
	{ { 1u, 1u, 1u }, 3u, 6u }, 
	{ { 1u, 3u }, 4u, 6u }, 
	{ { 3u, 1u }, 4u, 6u }, 
	{ { 1u, 1u, 2u }, 4u, 6u }, 
	{ { 1u, 2u, 1u }, 4u, 6u }, 
	{ { 2u, 1u, 1u }, 4u, 6u }, 
	{ { 2u, 2u, 1u }, 5u, 6u }, 
	{ { 5u }, 5u, 6u }, 
	{ { 1u, 4u }, 5u, 6u }, 
	{ { 2u, 3u }, 5u, 6u }, 
	{ { 3u, 2u }, 5u, 6u }, 
	{ { 4u, 1u }, 5u, 6u }, 
	{ { 2u, 1u, 2u }, 5u, 6u }, 
	{ { 1u, 2u, 2u }, 5u, 6u }, 
	{ { 6u }, 6u, 6u }, 
	{ { 2u, 4u }, 6u, 6u }, 
	{ { 4u, 2u }, 6u, 6u }, 
	{ { 2u, 2u, 2u }, 6u, 6u }, 
	{ { 1u, 1u, 1u, 1u }, 4u, 8u }, 
	{ { 1u, 1u, 2u, 1u }, 5u, 8u }, 
	{ { 1u, 1u, 3u }, 5u, 8u }, 
	{ { 2u, 1u, 1u, 1u }, 5u, 8u }, 
	{ { 1u, 1u, 1u, 2u }, 5u, 8u }, 
	{ { 1u, 3u, 1u }, 5u, 8u }, 
	{ { 3u, 1u, 1u }, 5u, 8u }, 
	{ { 1u, 2u, 1u, 1u }, 5u, 8u }, 
	{ { 3u, 3u }, 6u, 8u }, 
	{ { 1u, 5u }, 6u, 8u }, 
	{ { 5u, 1u }, 6u, 8u }, 
	{ { 1u, 1u, 4u }, 6u, 8u }, 
	{ { 1u, 2u, 3u }, 6u, 8u }, 
	{ { 1u, 3u, 2u }, 6u, 8u }, 
	{ { 1u, 4u, 1u }, 6u, 8u }, 
	{ { 2u, 1u, 3u }, 6u, 8u }, 
	{ { 2u, 1u, 1u, 2u }, 6u, 8u }, 
	{ { 2u, 3u, 1u }, 6u, 8u }, 
	{ { 3u, 1u, 2u }, 6u, 8u }, 
	{ { 3u, 2u, 1u }, 6u, 8u }, 
	{ { 4u, 1u, 1u }, 6u, 8u }, 
	{ { 2u, 2u, 1u, 1u }, 6u, 8u }, 
	{ { 1u, 1u, 2u, 2u }, 6u, 8u }, 
	{ { 2u, 1u, 2u, 1u }, 6u, 8u }, 
	{ { 1u, 2u, 1u, 2u }, 6u, 8u }, 
	{ { 1u, 2u, 2u, 1u }, 6u, 8u }, 
	{ { 1u, 1u, 1u, 1u, 1u }, 5u, 10u }, 
	{ { 1u, 3u, 1u, 1u }, 6u, 10u }, 
	{ { 1u, 1u, 3u, 1u }, 6u, 10u }, 
	{ { 1u, 1u, 1u, 3u }, 6u, 10u }, 
	{ { 3u, 1u, 1u, 1u }, 6u, 10u }, 
	{ { 1u, 1u, 1u, 1u, 2u }, 6u, 10u }, 
	{ { 1u, 1u, 1u, 2u, 1u }, 6u, 10u }, 
	{ { 1u, 1u, 2u, 1u, 1u }, 6u, 10u }, 
	{ { 1u, 2u, 1u, 1u, 1u }, 6u, 10u }, 
	{ { 2u, 1u, 1u, 1u, 1u }, 6u, 10u }, 
	{ { 1u, 1u, 1u, 1u, 1u, 1u }, 6u, 12u }
};

/* Generate db_gc for the the 5000 popular functions in crypto benchmarks */
/* Only functions with mc no more than 5 are taken into consideration     */
void create_db_x1g_crypto_thread0( std::string const& filename )
{
	std::uint32_t num_entries{ 0u };
	std::uint32_t num_cases_impr{ 0u };
	std::uint32_t num_gc_old{ 0u };
	std::uint32_t num_gc_new{ 0u };

	std::ifstream db_mc;
	db_mc.open( filename, std::ios::in );
	std::string line;
	uint32_t pos{ 0u };

	while ( std::getline( db_mc, line ) )
	{
		std::cout << "[i] working on the " << ++num_entries << "th representitive\n";

		if ( num_entries == 1000u )
		{
			std::cout << "[i] done\n";
		}

		pos = static_cast<uint32_t>( line.find( '\t' ) );
    const auto name = line.substr( 0, pos );
    pos += 1u;
    auto tt_str = line.substr( pos, 16u );
    pos += 17u;
    const auto repr_str = line.substr( pos, 16u );
    pos += 17u;
    uint32_t mc = std::stoul( line.substr( pos, 1u ) );

    pos += 2u;
    line.erase( 0, pos );
    const uint32_t num_vars = std::stoul( line.substr( 0, line.find( ' ' ) ) );
    line.erase( 0, line.find( ' ' ) + 1 );

    /* Reconstruct the mc-optimal XAG implementation */
    xag_network xag_mc_opt;
    std::vector<xag_network::signal> signals_mc_opt( num_vars );
    std::generate( signals_mc_opt.begin(), signals_mc_opt.end(), [&]() { return xag_mc_opt.create_pi(); } );

    while ( line.size() > 4 )
  	{
   		uint32_t signal_1, signal_2;
    	signal_1 = std::stoul( line.substr( 0, line.find( ' ' ) ) );
    	line.erase( 0, line.find( ' ' ) + 1 );
    	signal_2 = std::stoul( line.substr( 0, line.find( ' ' ) ) );
    	line.erase( 0, line.find( ' ' ) + 1 );

    	xag_network::signal signal1, signal2;
    	if ( signal_1 == 0u )
    	{
				signal1 = xag_mc_opt.get_constant( false );
    	}
    	else if ( signal_1 == 1u )
    	{
				signal1 = xag_mc_opt.get_constant( true );
    	}
    	else
    	{
    		signal1 = signals_mc_opt[signal_1 / 2 - 1] ^ ( signal_1 % 2 != 0 );
    	}
    	if ( signal_2 == 0u )
    	{
				signal2 = xag_mc_opt.get_constant( false );
    	}
    	else if ( signal_2 == 1u )
    	{
				signal2 = xag_mc_opt.get_constant( true );
    	}
    	else
    	{
    		signal2 = signals_mc_opt[signal_2 / 2 - 1] ^ ( signal_2 % 2 != 0 );
    	}

    	if ( signal_1 > signal_2 )
    	{
    		signals_mc_opt.emplace_back( xag_mc_opt.create_xor( signal1, signal2 ) );
    	}
    	else
    	{
    		signals_mc_opt.emplace_back( xag_mc_opt.create_and( signal1, signal2 ) );
    	}
    	line.erase( 0, line.find( ' ' ) + 1 );
  	}

  	const uint32_t signal_po = std::stoul( line );
  	xag_mc_opt.create_po( signals_mc_opt[signal_po / 2 - 1] ^ ( signal_po % 2 != 0 ) );

  	x1g_network x1g_mc_opt = mockturtle::map_xag2x1g( xag_mc_opt );
  	auto gc{ 0u };
  	x1g_mc_opt.foreach_gate( [&]( auto const& n ) {
  		if ( x1g_mc_opt.is_onehot( n ) )
  		{
  			++gc;
  		}
  	} );
  	gc *= 2u;
  	num_gc_old += gc;

    x1g_network x1g_gc_opt{ x1g_mc_opt };

  	kitty::dynamic_truth_table tt_min_base( num_vars );
  	assert( num_vars >= 2u );
    kitty::create_from_hex_string( tt_min_base, tt_str.substr( 0, 1 << ( num_vars - 2 ) ) );

    /* Look for the gc-optimal XAG implementation */
    for ( auto const& topo: and_topos_lib_6_x1g_oriented )
    {
    	if ( mc == 0u )
    	{
    		break;
    	}

    	if ( topo.mc < mc )
      {
        continue;
      }

      if ( topo.gc >= gc )
      {
        break;
      }

      optimum_gc_synthesis_params ps;
      //ps.verbose = true;
      //ps.verify_solution = true;

      auto const p_x1g_gc_opt = optimum_gc_synthesis<x1g_network, bill::solvers::glucose_41>( tt_min_base, topo.num_ands, ps, nullptr );
      if ( p_x1g_gc_opt )
      {
      	std::cout << "[i] Find " << ++num_cases_impr << " better implementations\n";
      	mc = topo.mc;
      	gc = topo.gc;
      	x1g_gc_opt = *p_x1g_gc_opt;
      	break;
      }
    }

    num_gc_new += gc;

    /* Record the gc-optimal X1G implementation */
    std::ofstream db_gc;
    db_gc.open( "db_gc_x1g_crypto_thread0", std::ios::app );
    db_gc << "0x" << repr_str << " ";
    db_gc << "0x" << tt_str << " ";
    //db_gc << mc << " ";
    db_gc << num_vars << " ";
    db_gc << gc << " ";
    x1g_gc_opt.foreach_gate( [&]( auto const& f ) {
    	x1g_gc_opt.foreach_fanin( f, [&]( auto const& fi ) {
    		db_gc << static_cast<uint32_t>( ( fi.index << 1 ) + fi.complement ) << " ";
    	} );
    } );
    x1g_network::signal po = x1g_gc_opt.po_at( 0 );
    db_gc << static_cast<uint32_t>( ( po.index << 1 ) + po.complement ) << "\n";
    db_gc.close();
	}
	db_mc.close();
	std::cout << "num_gc_old: " << num_gc_old << std::endl;
	std::cout << "num_gc_new: " << num_gc_new << std::endl;
}

} /* namespace mockturtle */

int main()
{
	mockturtle::create_db_x1g_crypto_thread0( "collected_funcs_crypto" );
	
	return 0;
}