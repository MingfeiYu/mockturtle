#include <cstdint>
#include <fstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <kitty/constructors.hpp>
#include <kitty/operations.hpp>
#include <kitty/spectral.hpp>
#include <mockturtle/algorithms/optimum_gc_synthesis.hpp>
#include <mockturtle/algorithms/xag2x1g.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/x1g.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <mockturtle/views/topo_view.hpp>

#include <experiments.hpp>

namespace mockturtle
{

using merge_view = fanout_view<topo_view<xag_network>>;

typedef struct AND_topo
{
	std::vector<uint32_t> num_ands;
	uint32_t mc;
	uint32_t gc;
} AND_topo;


/* smc-up-to-6 topos */
static std::vector<AND_topo> and_topos_lib = {
	{ { 1u }, 1u, 2u }, 
	{ { 2u }, 2u, 3u }, 
	{ { 1u, 1u }, 2u, 4u }, 
	{ { 3u }, 3u, 4u }, 
	{ { 1u, 2u }, 3u, 5u }, 
	{ { 2u, 1u }, 3u, 5u }, 
	{ { 4u }, 4u, 5u }, 
	{ { 1u, 1u, 1u }, 3u, 6u }, 
	{ { 1u, 3u }, 4u, 6u }, 
	{ { 2u, 2u }, 4u, 6u }, 
	{ { 3u, 1u }, 4u, 6u }, 
	{ { 5u }, 5u, 6u }, 
	{ { 1u, 1u, 2u }, 4u, 7u }, 
	{ { 1u, 2u, 1u }, 4u, 7u }, 
	{ { 2u, 1u, 1u }, 4u, 7u }, 
	{ { 4u, 1u }, 5u, 7u }, 
	{ { 1u, 4u }, 5u, 7u }, 
	{ { 2u, 3u }, 5u, 7u }, 
	{ { 3u, 2u }, 5u, 7u }, 
	{ { 6u }, 6u, 7u }, 
	{ { 1u, 1u, 1u, 1u }, 4u, 8u }, 
	{ { 1u, 2u, 2u }, 5u, 8u }, 
	{ { 1u, 3u, 1u }, 5u, 8u }, 
	{ { 1u, 1u, 3u }, 5u, 8u }, 
	{ { 2u, 2u, 1u }, 5u, 8u }, 
	{ { 3u, 1u, 1u }, 5u, 8u }, 
	{ { 2u, 1u, 2u }, 5u, 8u }, 
	{ { 4u, 2u }, 6u, 8u }, 
	{ { 1u, 5u }, 6u, 8u }, 
	{ { 3u, 3u }, 6u, 8u }, 
	{ { 5u, 1u }, 6u, 8u }, 
	{ { 2u, 4u }, 6u, 8u }, 
	{ { 1u, 1u, 2u, 1u }, 5u, 9u }, 
	{ { 1u, 1u, 1u, 2u }, 5u, 9u }, 
	{ { 2u, 1u, 1u, 1u }, 5u, 9u }, 
	{ { 1u, 2u, 1u, 1u }, 5u, 9u }, 
	{ { 2u, 1u, 3u }, 6u, 9u }, 
	{ { 1u, 1u, 4u }, 6u, 9u }, 
	{ { 1u, 2u, 3u }, 6u, 9u }, 
	{ { 1u, 3u, 2u }, 6u, 9u }, 
	{ { 1u, 4u, 1u }, 6u, 9u }, 
	{ { 2u, 2u, 2u }, 6u, 9u }, 
	{ { 2u, 3u, 1u }, 6u, 9u }, 
	{ { 3u, 1u, 2u }, 6u, 9u }, 
	{ { 3u, 2u, 1u }, 6u, 9u }, 
	{ { 4u, 1u, 1u }, 6u, 9u }, 
	{ { 1u, 1u, 1u, 1u, 1u }, 5u, 10u }, 
	{ { 1u, 1u, 1u, 3u }, 6u, 10u }, 
	{ { 1u, 1u, 2u, 2u }, 6u, 10u }, 
	{ { 1u, 1u, 3u, 1u }, 6u, 10u }, 
	{ { 1u, 2u, 1u, 2u }, 6u, 10u }, 
	{ { 1u, 2u, 2u, 1u }, 6u, 10u }, 
	{ { 1u, 3u, 1u, 1u }, 6u, 10u }, 
	{ { 2u, 1u, 1u, 2u }, 6u, 10u }, 
	{ { 2u, 1u, 2u, 1u }, 6u, 10u }, 
	{ { 2u, 2u, 1u, 1u }, 6u, 10u }, 
	{ { 3u, 1u, 1u, 1u }, 6u, 10u }, 
	{ { 1u, 1u, 1u, 1u, 2u }, 6u, 11u }, 
	{ { 1u, 1u, 1u, 2u, 1u }, 6u, 11u }, 
	{ { 1u, 1u, 2u, 1u, 1u }, 6u, 11u }, 
	{ { 1u, 2u, 1u, 1u, 1u }, 6u, 11u }, 
	{ { 2u, 1u, 1u, 1u, 1u }, 6u, 11u }, 
	{ { 1u, 1u, 1u, 1u, 1u, 1u }, 6u, 12u }
};


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

typedef struct represent
{
	std::string func;
	uint32_t mc;
} represent;

static std::vector<represent> affine_represent_5 = {
	{"00000000", 0}, 
	{"80000000", 4}, 
	{"80008000", 3}, 
	{"00808080", 4}, 
	{"80808080", 2}, 
	{"08888000", 3}, 
	{"aa2a2a80", 4}, 
	{"88080808", 4}, 
	{"2888a000", 3}, 
	{"f7788000", 3}, 
	{"a8202020", 3}, 
	{"08880888", 3}, 
	{"bd686868", 4}, 
	{"aa808080", 4}, 
	{"7e686868", 4}, 
	{"2208a208", 4}, 
	{"08888888", 4}, 
	{"88888888", 1}, 
	{"ea404040", 3}, 
	{"2a802a80", 2}, 
	{"73d28c88", 3}, 
	{"ea808080", 3}, 
	{"a28280a0", 3}, 
	{"13284c88", 3}, 
	{"a2220888", 3}, 
	{"aae6da80", 4}, 
	{"58d87888", 4}, 
	{"8c88ac28", 4}, 
	{"8880f880", 4}, 
	{"9ee8e888", 4}, 
	{"4268c268", 4}, 
	{"16704c80", 4}, 
	{"78888888", 3}, 
	{"4966bac0", 4}, 
	{"372840a0", 4}, 
	{"5208d288", 3}, 
	{"7ca00428", 4}, 
	{"f8880888", 3}, 
	{"2ec0ae40", 4}, 
	{"f888f888", 3}, 
	{"58362ec0", 4}, 
	{"0eb8f6c0", 4}, 
	{"567cea40", 4}, 
	{"f8887888", 4}, 
	{"78887888", 2}, 
	{"e72890a0", 4}, 
	{"268cea40", 3}, 
	{"6248eac0", 4}
};

void count_and_size_rec( merge_view& xag, merge_view::node const& f, merge_view::node const& root )
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

void count_and_size_rec_verbose( merge_view& xag, merge_view::node const& n, merge_view::node const& root, std::vector<merge_view::node>& leaves )
{
	if ( xag.value( n ) == 0u ) 
	{
		if ( n != root )
		{
			if ( xag.fanout( n ).size() == 1u )
			{
				/* remark this node as traversed */
				xag.incr_value( n );
				/* update the size of this AND group */
				xag.incr_value( root );
				/* recursively trace its fanin */
				xag.foreach_fanin( n, [&]( auto const& f ) {
					auto const child = xag.get_node( f );
					if ( xag.is_and( child ) && !xag.is_complemented( f ) )
					{
						count_and_size_rec_verbose( xag, child, root, leaves );
					}
					else
					{
						/* find a leave of a group */
						leaves.emplace_back( n );
					}
				} );
			}
		}
		else
		{
			xag.set_value( n, 2u );
			/* recursively trace its fanin */
			xag.foreach_fanin( n, [&]( auto const& f ) {
				auto const child = xag.get_node( f );
				if ( xag.is_and( child ) && !xag.is_complemented( f ) )
				{
					count_and_size_rec_verbose( xag, child, n, leaves );
				}
			} );
		}
	}
}

uint32_t count_gc_verbose( merge_view& xag, std::unordered_map<merge_view::node, std::vector<merge_view::node>>& partition )
{	
	uint32_t gc{ 0u };
	xag.clear_values();

	xag.foreach_gate_reverse( [&]( auto const& f ) {
		if ( xag.is_and( f ) && xag.value( f ) == 0u )
		{
			std::vector<xag_network::node> leaves;
			count_and_size_rec_verbose( xag, f, f, leaves );
			partition.insert( std::make_pair( f, leaves ) );
			gc += xag.value( f );
		}
	} );

	xag.clear_values();
	return gc;
}

uint32_t count_gc_report( merge_view& xag, std::vector<uint32_t>& topo )
{
	uint32_t gc{ 0u };
	xag.clear_values();

	xag.foreach_gate_reverse( [&]( auto const& f ) {
		if ( xag.is_and( f ) && xag.value( f ) == 0u )
		{
			count_and_size_rec( xag, f, f );
			topo.emplace_back( xag.value( f ) - 1 );
			gc += xag.value( f );
		}
	} );
	xag.clear_values();

	return gc;
}

/* For debugging */
void test()
{
	kitty::dynamic_truth_table tt( 4u );
  kitty::create_from_hex_string( tt, "f888" );
  kitty::dynamic_truth_table tt_spec( 5u );
  kitty::create_from_hex_string( tt_spec, "f888f888" );
  uint32_t mc{ 3u };
  uint32_t num_var{ 4u };
  std::string xag_mc_opt_str = "2 4 6 8 10 12 14 10 16 12 18";
  uint32_t mc_spectral = kitty::get_spectral_mc( tt_spec );
  std::cout << "[i] figured out mc is: " << mc_spectral << ", while the real mc is: " << mc << "\n";

  xag_network xag_mc_opt;
  std::vector<xag_network::signal> signals_mc_opt( num_var );
  std::generate( signals_mc_opt.begin(), signals_mc_opt.end(), [&]() { return xag_mc_opt.create_pi(); } );

  while ( xag_mc_opt_str.size() > 3 )
	{
 		uint32_t signal_1, signal_2;
  	signal_1 = std::stoul( xag_mc_opt_str.substr( 0, xag_mc_opt_str.find( ' ' ) ) );
  	xag_mc_opt_str.erase( 0, xag_mc_opt_str.find( ' ' ) + 1 );
  	signal_2 = std::stoul( xag_mc_opt_str.substr( 0, xag_mc_opt_str.find( ',' ) ) );
  	xag_mc_opt_str.erase( 0, xag_mc_opt_str.find( ' ' ) + 1 );

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
	}

	const uint32_t signal_po = std::stoul( xag_mc_opt_str );
	xag_mc_opt.create_po( signals_mc_opt[signal_po / 2 - 1] ^ ( signal_po % 2 != 0 ) );

	/* reconstruct the mc-optimal X1G implementation */
	x1g_network x1g_mc_opt = mockturtle::map_xag2x1g( xag_mc_opt );
	auto gc{ 0u };
	x1g_mc_opt.foreach_gate( [&]( auto const& n ) {
		if ( x1g_mc_opt.is_onehot( n ) )
		{
			++gc;
		}
	} );
	gc *= 2u;
  std::cout << "GC of the X1G obtained from mc-optimal XAG is: " << gc << std::endl;

  uint32_t topo_cnt{ 1u };

  for ( auto const& topo: and_topos_lib_6_x1g_oriented )
  {
    if ( topo.mc < mc )
    {
      continue;
    }

    if ( topo.gc >= gc )
    {
      break;
    }

    std::cout << "The " << topo_cnt++ << "th trial: mc = " << topo.mc << ", gc = " << topo.gc << std::endl;

    optimum_gc_synthesis_params ps;
    ps.verbose = true;
    ps.verify_solution = true;

    auto const p_xag_gc_opt = optimum_gc_synthesis( tt, topo.num_ands, ps, nullptr );
    if ( p_xag_gc_opt )
    {
      mc = topo.mc;
     	gc = topo.gc;
     	std::cout << "[i] find a better solution!\n";
     	std::cout << "[i] minimum GC is: " << gc << "\n";
      break;
    }
  }
}

/* to find a toy example illustrating that for the same function, its GC-optimal */
/* X1G and compact XAG implementations can be structurally different             */
void experiment( std::string const& filename )
{
	bool found{ false };
	std::map<std::vector<uint32_t>, uint32_t> and_topos_xag;
	for ( auto topo_each: and_topos_lib )
	{
		and_topos_xag.insert( std::make_pair( topo_each.num_ands, topo_each.gc ) );
	}
	uint32_t num_entries{ 0u };
	std::ifstream db_mc;
	db_mc.open( filename, std::ios::in );
	std::string line;
	uint32_t pos{ 0u };

	while ( std::getline( db_mc, line ) )
	{
		std::cout << "[i] Working on the " << ++num_entries << "th representitive\n";

		pos = static_cast<uint32_t>( line.find( 'x' ) );
    auto tt_str = line.substr( ++pos, 8u );
    //if ( tt_str != "2888a000" )
    //{
    //	continue;
    //}
    pos += 9u;
    line.erase( 0, pos );
    kitty::static_truth_table<5u> tt_static;
    kitty::create_from_hex_string( tt_static, tt_str );
		auto repr = kitty::exact_spectral_canonization( tt_static );
    std::string repr_str = kitty::to_hex( repr );

    kitty::dynamic_truth_table tt( 5u );
    kitty::create_from_hex_string( tt, tt_str );
    auto num_vars = ( kitty::min_base_inplace( tt ) ).size();
    uint32_t mc = kitty::get_spectral_mc( tt );
    std::cout << "[i] its FMC is " << mc << "\n";

    /* Reconstruct the mc-optimal XAG implementation */
    xag_network xag_mc_opt;
    std::vector<xag_network::signal> signals_mc_opt( num_vars );
    std::generate( signals_mc_opt.begin(), signals_mc_opt.end(), [&]() { return xag_mc_opt.create_pi(); } );

    while ( line.size() > 3 )
  	{
   		uint32_t signal_1, signal_2;
    	signal_1 = std::stoul( line.substr( 0, line.find( ',' ) ) );
    	line.erase( 0, line.find( ' ' ) + 1 );
    	signal_2 = std::stoul( line.substr( 0, line.find( ',' ) ) );
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
  	}

  	const uint32_t signal_po = std::stoul( line );
  	xag_mc_opt.create_po( signals_mc_opt[signal_po / 2 - 1] ^ ( signal_po % 2 != 0 ) );

    mockturtle::topo_view<mockturtle::xag_network> xag_mc_opt_topo{ xag_mc_opt };
    merge_view xag_mc_opt_merge{ xag_mc_opt_topo };
    std::vector<uint32_t> topo1;
    uint32_t gc1;
  	auto gc = count_gc_report( xag_mc_opt_merge, topo1 );
  	gc1 = gc;
  	std::cout << "[i] its gc is " << gc << "\n";

  	kitty::dynamic_truth_table tt_min_base( num_vars );
  	assert( num_vars >= 2u );
    kitty::create_from_hex_string( tt_min_base, tt_str.substr( 0, 1 << ( num_vars - 2 ) ) );

    /* Look for the gc-optimal XAG implementation */
    for ( auto const& topo: and_topos_lib )
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
      //ps.verify_solution = true;

      auto const p_xag_gc_opt = optimum_gc_synthesis( tt_min_base, topo.num_ands, ps, nullptr );
      if ( p_xag_gc_opt )
      {
      	topo1 = topo.num_ands;
      	gc1 = topo.gc;
      	//std::cout << "[i] print topo on the scece: ";
      	//for ( auto topo_element: topo.num_ands )
      	//{
      	//	std::cout << topo_element << " ";
      	//}
      	//std::cout << "\n";
      	break;
      }
    }

    x1g_network x1g_mc_opt = mockturtle::map_xag2x1g( xag_mc_opt );
  	gc = 0u;
  	x1g_mc_opt.foreach_gate( [&]( auto const& n ) {
  		if ( x1g_mc_opt.is_onehot( n ) )
  		{
  			++gc;
  		}
  	} );
  	gc *= 2u;
  	std::vector<uint32_t> topo2;
  	std::sort( topo1.begin(), topo1.end() );

    for ( auto const& topo: and_topos_lib_6_x1g_oriented )
    {
    	if ( topo.mc < mc )
      {
        continue;
      }

      if ( topo.gc >= gc )
      {
        break;
      }
      optimum_gc_synthesis_params ps;
      auto const p_xag_gc_opt = optimum_gc_synthesis( tt_min_base, topo.num_ands, ps, nullptr );
      if ( p_xag_gc_opt )
      {
      	topo2 = topo.num_ands;
      	std::sort( topo2.begin(), topo2.end() );
      	if ( topo1 != topo2 )
      	{
      		//std::cout << "[i] print topo on the scece: ";
      		//for ( auto topo_element: topo.num_ands )
      		//{
      		//	std::cout << topo_element << " ";
      		//}
      		//std::cout << "\n";
      		auto const search = and_topos_xag.find( topo2 );
      		if ( search != and_topos_xag.end() && search->second > gc1 )
      		{	
      			found = true;
      			std::cout << "[i] 0x" << tt_str << " is the function\n";

      			std::cout << "[i] the AND distribution of its GC-optimal compact XAG: ";
      			for ( auto topo_element: topo1 )
      			{
      				std::cout << topo_element << " ";
      			}
      			std::cout << "\n";
      			std::cout << "[i] the AND distribution of its GC-optimal X1G: ";
      			for ( auto topo_element: topo2 )
      			{
      				std::cout << topo_element << " ";
      			}
      			std::cout << "\n";
      		}
      	}
      	break;
      }
    }
    if ( found )
    {
    	break;
    }

	}
	db_mc.close();  
}

/* to find a GC-optimal compact XAG implementation for #2888a000                 */
void experiment2( std::string const& filename )
{
	std::ifstream db_mc;
	db_mc.open( filename, std::ios::in );
	std::string line;
	uint32_t pos{ 0u };

	while ( std::getline( db_mc, line ) )
	{
		pos = static_cast<uint32_t>( line.find( 'x' ) );
    auto tt_str = line.substr( ++pos, 8u );
    if ( tt_str != "2888a000" )
    {
    	continue;
    }
    else
    {
    	std::cout << "[i] working on 2888a000\n";
    }
    pos += 9u;
    line.erase( 0, pos );
    kitty::static_truth_table<5u> tt_static;
    kitty::create_from_hex_string( tt_static, tt_str );
		auto repr = kitty::exact_spectral_canonization( tt_static );
    std::string repr_str = kitty::to_hex( repr );

    kitty::dynamic_truth_table tt( 5u );
    kitty::create_from_hex_string( tt, tt_str );
    auto num_vars = ( kitty::min_base_inplace( tt ) ).size();
    uint32_t mc = kitty::get_spectral_mc( tt );
    std::cout << "[i] its FMC is " << mc << "\n";

    /* Reconstruct the mc-optimal XAG implementation */
    xag_network xag_mc_opt;
    std::vector<xag_network::signal> signals_mc_opt( num_vars );
    std::generate( signals_mc_opt.begin(), signals_mc_opt.end(), [&]() { return xag_mc_opt.create_pi(); } );

    while ( line.size() > 3 )
  	{
   		uint32_t signal_1, signal_2;
    	signal_1 = std::stoul( line.substr( 0, line.find( ',' ) ) );
    	line.erase( 0, line.find( ' ' ) + 1 );
    	signal_2 = std::stoul( line.substr( 0, line.find( ',' ) ) );
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
  	}

  	const uint32_t signal_po = std::stoul( line );
  	xag_mc_opt.create_po( signals_mc_opt[signal_po / 2 - 1] ^ ( signal_po % 2 != 0 ) );

    mockturtle::topo_view<mockturtle::xag_network> xag_mc_opt_topo{ xag_mc_opt };
    merge_view xag_mc_opt_merge{ xag_mc_opt_topo };
  	auto gc = count_gc( xag_mc_opt_merge );

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
      ps.verify_solution = true;

      auto const p_xag_gc_opt = optimum_gc_synthesis( tt_min_base, topo.num_ands, ps, nullptr );
      if ( p_xag_gc_opt )
      {
      	std::cout << "[i] find a implementation whose gc is " << topo.gc << "\n";
      	break;
      }
    }
	}
	db_mc.close();  
}

/* Generate db_gc for the 38 5-input representatives in the 147,998-entry db_mc */
void create_db_fast( std::string const& filename )
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
		pos = static_cast<uint32_t>( line.find( '\t' ) );
    const auto name = line.substr( 0, pos );
    pos += 1u;
    auto tt_str = line.substr( pos, 8u );
    pos += 17u;
    //const auto repr_str = line.substr( pos, 8u );
    pos += 17u;
    uint32_t mc = std::stoul( line.substr( pos, 1u ) );
    pos += 2u;
    line.erase( 0, pos );
    const uint32_t num_vars = std::stoul( line.substr( 0, line.find( ' ' ) ) );
    if ( num_vars > 5u ) 
    {
    	continue;
    }
    std::cout << "[i] Working on the " << num_entries << "th representitive\n";
    ++num_entries;

    line.erase( 0, line.find( ' ' ) + 1 );

    kitty::static_truth_table<5u> tt_static;
    kitty::create_from_hex_string( tt_static, tt_str );
		auto repr = kitty::exact_spectral_canonization( tt_static );
    std::string repr_str = kitty::to_hex( repr );

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

    mockturtle::topo_view<mockturtle::xag_network> xag_mc_opt_topo{ xag_mc_opt };
    merge_view xag_mc_opt_merge{ xag_mc_opt_topo };
  	auto gc = count_gc( xag_mc_opt_merge );
  	num_gc_old += gc;
    kitty::dynamic_truth_table tt( 5u );
    kitty::create_from_hex_string( tt, tt_str );

    xag_network xag_gc_opt{ xag_mc_opt };

    /* Look for the gc-optimal XAG implementation */
    for ( auto const& topo: and_topos_lib )
    {
    	if ( topo.mc < mc )
      {
        continue;
      }

      if ( topo.gc >= gc )
      {
        break;
      }

      optimum_gc_synthesis_params ps;
      ps.verify_solution = true;

      auto const p_xag_gc_opt = optimum_gc_synthesis( tt, topo.num_ands, ps, nullptr );
      if ( p_xag_gc_opt )
      {
      	++num_cases_impr;
      	std::cout << "[i] Find " << num_cases_impr << " better implementations\n";
      	mc = topo.mc;
      	gc = topo.gc;
      	xag_gc_opt = *p_xag_gc_opt;
      	break;
      }
    }

    num_gc_new += gc;

    /* Record the gc-optimal XAG implementation */
    std::ofstream db_gc;
    db_gc.open( "db_gc_fast_5", std::ios::app );
    //db_gc << name << " ";
    db_gc << "0x" << repr_str << " ";
    db_gc << "0x" << tt_str << " ";
    //db_gc << "0x" << repr_str << " ";
    //db_gc << mc << " ";
    db_gc << num_vars << " ";
    db_gc << gc << " ";
    xag_gc_opt.foreach_gate( [&]( auto const& f ) {
    	xag_gc_opt.foreach_fanin( f, [&]( auto const& fi ) {
    		db_gc << static_cast<uint32_t>( ( fi.index << 1 ) + fi.complement ) << " ";
    	} );
    	//db_gc << static_cast<uint32_t>( ( xag_gc_opt.node_to_index( f ) ) << 1 ) << " ";
    } );
    xag_network::signal po = xag_gc_opt.po_at( 0 );
    db_gc << static_cast<uint32_t>( ( po.index << 1 ) + po.complement ) << "\n";
    db_gc.close();
	}
	db_mc.close();
	std::cout << "num_gc_old: " << num_gc_old << std::endl;
	std::cout << "num_gc_new: " << num_gc_new << std::endl;
}

/* Generate db_gc for the 822 6-input-up-to-4-mc representatives in the 147,998-entry db_mc */
void create_db_practical( std::string const& filename )
{
	std::uint32_t num_entries{ 0u };
	std::uint32_t num_cases_impr{ 0u };
	std::uint32_t num_gc_old{ 0u };
	std::uint32_t num_gc_new{ 0u };

	//std::make_shared<xag_network> p_db_gc;
	//std::vector<xag_network::signal> db_gc_pis( 5u );
	//std::generate( db_gc_pis.begin(), db_gc_pis.end(), [&]() { return p_db_gc->create_pi(); } );

	std::ifstream db_mc;
	db_mc.open( filename, std::ios::in );
	std::string line;
	uint32_t pos{ 0u };

	while ( std::getline( db_mc, line ) )
	{
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
    if ( num_vars > 5u && mc > 4u ) 
    {
    	continue;
    }
    std::cout << "[i] Working on the " << num_entries << "th representitive\n";
    ++num_entries;

    line.erase( 0, line.find( ' ' ) + 1 );

    //std::vector<xag_network::signal> nodes_db_gc( db_gc_pis.begin(), db_gc_pis.begin() + num_vars );

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

    merge_view xag_mc_opt_merge{ xag_mc_opt };
  	auto gc = count_gc( xag_mc_opt_merge );
  	num_gc_old += gc;
    kitty::dynamic_truth_table tt( 6u );
    kitty::create_from_hex_string( tt, tt_str );

    xag_network xag_gc_opt{ xag_mc_opt };

    /* Look for the gc-optimal XAG implementation */
    for ( auto const& topo: and_topos_lib )
    {
    	if ( topo.mc < mc )
      {
        continue;
      }

      if ( topo.gc >= gc )
      {
        break;
      }

      optimum_gc_synthesis_params ps;

      auto const p_xag_gc_opt = optimum_gc_synthesis( tt, topo.num_ands, ps, nullptr );
      if ( p_xag_gc_opt )
      {
      	++num_cases_impr;
      	std::cout << "[i] Find " << num_cases_impr << " better implementations\n";
      	mc = topo.mc;
      	gc = topo.gc;
      	xag_gc_opt = *p_xag_gc_opt;
      	break;
      }
    }

    num_gc_new += gc;

    /* Record the gc-optimal XAG implementation */
    std::ofstream db_gc;
    db_gc.open( "db_gc_practical", std::ios::app );
    //db_gc << name << " ";
    db_gc << "0x" << tt_str << " ";
    //db_gc << "0x" << repr_str << " ";
    //db_gc << mc << " ";
    db_gc << num_vars << " ";
    db_gc << gc << " ";
    xag_gc_opt.foreach_gate( [&]( auto const& f ) {
    	xag_gc_opt.foreach_fanin( f, [&]( auto const& fi ) {
    		db_gc << static_cast<uint32_t>( ( fi.index << 1 ) + fi.complement ) << " ";
    	} );
    	//db_gc << static_cast<uint32_t>( ( xag_gc_opt.node_to_index( f ) ) << 1 ) << " ";
    } );
    xag_network::signal po = xag_gc_opt.po_at( 0 );
    db_gc << static_cast<uint32_t>( ( po.index << 1 ) + po.complement ) << " \n";
    db_gc.close();
	}
	db_mc.close();
	std::cout << "num_gc_old: " << num_gc_old << std::endl;
	std::cout << "num_gc_new: " << num_gc_new << std::endl;
}

/* Generate db_gc for all the 48 5-input representatives */
void create_db_complete( std::string const& filename )
{
	//std::uint32_t num_entries{ 0u };
	std::uint32_t num_cases_impr{ 0u };
	std::uint32_t num_gc_old{ 0u };
	std::uint32_t num_gc_new{ 0u };

	std::ifstream db_mc;
	db_mc.open( filename, std::ios::in );
	std::string line;
	uint32_t pos{ 0u };
	uint32_t num_entries{ 0u };

	while ( std::getline( db_mc, line ) )
	{
		std::cout << "[i] Working on the " << ++num_entries << "th representitive\n";

		pos = static_cast<uint32_t>( line.find( 'x' ) );
    auto tt_str = line.substr( ++pos, 8u );
    pos += 9u;
    line.erase( 0, pos );
    kitty::static_truth_table<5u> tt_static;
    kitty::create_from_hex_string( tt_static, tt_str );
    //std::vector<kitty::detail::spectral_operation> trans;

    //auto repr = kitty::exact_spectral_canonization( tt_static, [&trans]( auto const& ops ) {
		//																															std::copy( ops.begin(), ops.end(), std::back_inserter( trans ) );
		//																														} );
		auto repr = kitty::exact_spectral_canonization( tt_static );
    std::string repr_str = kitty::to_hex( repr );
    //if ( tt_str != repr_str )
    //{
    //	std::cout << "The fake representitive: " << tt_str << std::endl;
    //	std::cout << "The real representitive: " << repr_str << std::endl;
    //}
    //assert( tt_str == repr_str );

    kitty::dynamic_truth_table tt( 5u );
    kitty::create_from_hex_string( tt, tt_str );
    auto num_vars = ( kitty::min_base_inplace( tt ) ).size();
    uint32_t mc = kitty::get_spectral_mc( tt );

    /* Reconstruct the mc-optimal XAG implementation */
    xag_network xag_mc_opt;
    std::vector<xag_network::signal> signals_mc_opt( num_vars );
    std::generate( signals_mc_opt.begin(), signals_mc_opt.end(), [&]() { return xag_mc_opt.create_pi(); } );

    while ( line.size() > 3 )
  	{
   		uint32_t signal_1, signal_2;
    	signal_1 = std::stoul( line.substr( 0, line.find( ',' ) ) );
    	line.erase( 0, line.find( ' ' ) + 1 );
    	signal_2 = std::stoul( line.substr( 0, line.find( ',' ) ) );
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
  	}

  	const uint32_t signal_po = std::stoul( line );
  	xag_mc_opt.create_po( signals_mc_opt[signal_po / 2 - 1] ^ ( signal_po % 2 != 0 ) );

    mockturtle::topo_view<mockturtle::xag_network> xag_mc_opt_topo{ xag_mc_opt };
    merge_view xag_mc_opt_merge{ xag_mc_opt_topo };
  	auto gc = count_gc( xag_mc_opt_merge );
  	num_gc_old += gc;

  	kitty::dynamic_truth_table tt_min_base( num_vars );
  	assert( num_vars >= 2u );
    kitty::create_from_hex_string( tt_min_base, tt_str.substr( 0, 1 << ( num_vars - 2 ) ) );

    xag_network xag_gc_opt{ xag_mc_opt };

    /* Look for the gc-optimal XAG implementation */
    for ( auto const& topo: and_topos_lib )
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
      ps.verbose = true;
      ps.verify_solution = true;

      auto const p_xag_gc_opt = optimum_gc_synthesis( tt_min_base, topo.num_ands, ps, nullptr );
      if ( p_xag_gc_opt )
      {
      	++num_cases_impr;
      	//std::cout << "[i] Find " << num_cases_impr << " better implementations\n";
      	if ( topo.mc > mc )
      	{
      		std::cout << "[i] For function " << tt_str << ": "
										<< "its FMC is " << mc << ", "
										<< "but SMC of its optimal implementation is " << topo.mc << "\n";
      	}
      	mc = topo.mc;
      	gc = topo.gc;
      	xag_gc_opt = *p_xag_gc_opt;
      	break;
      }
    }

    num_gc_new += gc;

    /* Record the gc-optimal XAG implementation */
    std::ofstream db_gc;
    db_gc.open( "experiments_db_gc_complete_5", std::ios::app );
    db_gc << "0x" << repr_str << " ";
    db_gc << "0x" << tt_str << " ";
    //db_gc << mc << " ";
    db_gc << num_vars << " ";
    db_gc << gc << " ";
    xag_gc_opt.foreach_gate( [&]( auto const& f ) {
    	xag_gc_opt.foreach_fanin( f, [&]( auto const& fi ) {
    		db_gc << static_cast<uint32_t>( ( fi.index << 1 ) + fi.complement ) << " ";
    	} );
    } );
    xag_network::signal po = xag_gc_opt.po_at( 0 );
    db_gc << static_cast<uint32_t>( ( po.index << 1 ) + po.complement ) << "\n";
    db_gc.close();
	}
	db_mc.close();
	std::cout << "num_gc_old: " << num_gc_old << std::endl;
	std::cout << "num_gc_new: " << num_gc_new << std::endl;
}

/* Generate X1G implementations for all the 48 5-input representatives */
void create_db_complete_x1g( std::string const& filename )
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
		std::cout << "[i] Working on the " << ++num_entries << "th representitive\n";

		pos = static_cast<uint32_t>( line.find( 'x' ) );
    auto tt_str = line.substr( ++pos, 8u );
    pos += 9u;
    line.erase( 0, pos );
    kitty::static_truth_table<5u> tt_static;
    kitty::create_from_hex_string( tt_static, tt_str );
		auto repr = kitty::exact_spectral_canonization( tt_static );
    std::string repr_str = kitty::to_hex( repr );

    kitty::dynamic_truth_table tt( 5u );
    kitty::create_from_hex_string( tt, tt_str );
    auto num_vars = ( kitty::min_base_inplace( tt ) ).size();
    uint32_t mc = kitty::get_spectral_mc( tt );

    //bool display{ false };
    //if ( tt_str == "f888f888" )
    //{
    //	std::cout << "[m] here comes function 0xf888, whose FMC is " << mc << "\n";
    //	display = true;
    //}

    /* Reconstruct the mc-optimal XAG implementation */
    xag_network xag_mc_opt;
    std::vector<xag_network::signal> signals_mc_opt( num_vars );
    std::generate( signals_mc_opt.begin(), signals_mc_opt.end(), [&]() { return xag_mc_opt.create_pi(); } );

    while ( line.size() > 3 )
  	{
   		uint32_t signal_1, signal_2;
    	signal_1 = std::stoul( line.substr( 0, line.find( ',' ) ) );
    	line.erase( 0, line.find( ' ' ) + 1 );
    	signal_2 = std::stoul( line.substr( 0, line.find( ',' ) ) );
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
  	}

  	const uint32_t signal_po = std::stoul( line );
  	xag_mc_opt.create_po( signals_mc_opt[signal_po / 2 - 1] ^ ( signal_po % 2 != 0 ) );

    //topo_view<xag_network> xag_mc_opt_topo{ xag_mc_opt };
    //merge_view xag_mc_opt_merge{ xag_mc_opt_topo };
    //std::unordered_map<merge_view::node, std::vector<merge_view::node>> partition;
  	//auto gc = count_gc_verbose( xag_mc_opt_merge, partition );

  	/* reconstruct the mc-optimal X1G implementation */
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

  	//if ( display )
  	//{
  	//	std::cout << "[m] the initial GC is " << gc << "\n";
  	//}
  	/*node_map<x1g_network::signal, merge_view> xag2x1g( xag_mc_opt_merge );

  	// generate constants //
  	xag2x1g[xag_mc_opt.get_constant( false )] = x1g_mc_opt.get_constant( false );

  	// generate pis //
  	xag_mc_opt_merge.foreach_pi( [&]( auto const& f ) {
  		xag2x1g[f] = x1g_mc_opt.create_pi();
  	});

  	// generate gates //
  	xag_mc_opt_merge.foreach_gate( [&]( auto const& n ) { 
  		if ( xag_mc_opt_merge.is_xor( n ) )
  		{
  			std::vector<x1g_network::signal> children;
	  		xag_mc_opt_merge.foreach_fanin( n, [&]( auto const& f ) {
	        children.emplace_back( xag_mc_opt_merge.is_complemented( f ) ? x1g_mc_opt.create_not( xag2x1g[f] ) : xag2x1g[f] );
	      } );
	      assert( children.size() == 2 );
	      xag2x1g[n] = x1g_mc_opt.create_xor( children[0], children[1] );
  		}
  		else
  		{
  			auto search = partition.find( n );
  			if ( search != partition.end() )
  			{
  				// the root of an AND group //
  				if ( search->second.empty() )
  				{
  					// the group consists of only the root //
  					std::vector<x1g_network::signal> children;
  					xag_mc_opt_merge.foreach_fanin( n, [&]( auto const& f ) {
  						children.emplace_back( xag_mc_opt_merge.is_complemented( f ) ? x1g_mc_opt.create_not( xag2x1g[f] ) : xag2x1g[f] );
  					} );
  					assert( children.size() == 2 );
  					xag2x1g[n] = x1g_mc_opt.create_and( children[0], children[1] );
  				}
  				else
  				{
  					// the group consists of root, leaves, and (possibly) internal nodes //
  					std::vector<x1g_network::signal> children;
  					for ( auto const& leaf: search->second )
  					{
  						xag_mc_opt_merge.foreach_fanin( leaf, [&]( auto const& f ) {
  							children.emplace_back( xag_mc_opt_merge.is_complemented( f ) ? x1g_mc_opt.create_not( xag2x1g[f] ) : xag2x1g[f] );
  						} );
  					}
  					xag2x1g[n] = x1g_mc_opt.create_nary_and( children );
  				}
  			}
  			// skip non-root AND nodes //
  		}
  	});

  	// generate po //
  	xag_mc_opt_merge.foreach_po( [&]( auto const& f ) {
  		x1g_mc_opt.create_po( xag_mc_opt_merge.is_complemented( f ) ? x1g_mc_opt.create_not( xag2x1g[f] ) : xag2x1g[f] );
  	} );
  	*/

  	xag_network xag_gc_opt{ xag_mc_opt };
    x1g_network x1g_gc_opt{ x1g_mc_opt };

    //
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
      ps.verify_solution = true;

      auto const p_xag_gc_opt = optimum_gc_synthesis( tt_min_base, topo.num_ands, ps, nullptr );
      if ( p_xag_gc_opt )
      {
      	std::cout << "[i] Find " << ++num_cases_impr << " better implementations\n";
      	mc = topo.mc;
      	gc = topo.gc;
      	xag_gc_opt = *p_xag_gc_opt;
      	auto const p_x1g_gc_opt = optimum_gc_synthesis<x1g_network, bill::solvers::glucose_41>( tt_min_base, topo.num_ands, ps, nullptr );
      	x1g_gc_opt = *p_x1g_gc_opt;
      	//x1g_gc_opt = map_xag2x1g( xag_gc_opt );

      	//if ( display )
      	//{
      	//	std::cout << "[m] minimum GC is " << gc << "\n";
      	//}
      	break;
      }
    }
    //

    num_gc_new += gc;

    /* Record the gc-optimal X1G implementation */
    std::ofstream db_gc;
    db_gc.open( "experiments_db_gc_complete_x1g_5", std::ios::app );
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

/* Generate X1G implementations for all the 48 5-input representatives */
void x1g_affine_5_exact_synthesis()
{
	experiments::experiment<std::string, uint32_t, uint32_t, uint32_t, uint32_t, float, bool> exp_res( "novel_x1g_affine_5", "function", "num_xor", "num_onehot", "num_variables", "num_clauses", "runtime[s]", "exact synth. suc." );

	for ( auto const& benchmark: affine_represent_5 )
	{
		if ( benchmark.func == "00000000" )
		{
			exp_res( benchmark.func, 0u, 0u, 0u, 0u, 0., true );
			continue;
		}
		std::cout << "[i] processing " << benchmark.func << "\n";
		kitty::dynamic_truth_table tt( 5u );
    kitty::create_from_hex_string( tt, benchmark.func );
    auto num_vars = ( kitty::min_base_inplace( tt ) ).size();
    uint32_t mc = benchmark.mc;
  	kitty::dynamic_truth_table tt_min_base( num_vars );
    kitty::create_from_hex_string( tt_min_base, benchmark.func.substr( 0, 1 << ( num_vars - 2 ) ) );

    /* Look for the gc-optimal XAG implementation */
    const clock_t begin_time = clock();
    for ( auto i{ 0u }; i < and_topos_lib_6_x1g_oriented.size(); ++i )
    {
    	auto topo = and_topos_lib_6_x1g_oriented[i];

    	if ( mc == 0u )
    	{
    		break;
    	}

    	if ( topo.mc < mc )
      {
        continue;
      }

      optimum_gc_synthesis_params ps;
      //ps.verbose = true;
      //ps.verify_solution = true;
      optimum_gc_synthesis_stats st;

      auto const p_x1g_gc_opt = optimum_gc_synthesis<x1g_network, bill::solvers::glucose_41>( tt_min_base, topo.num_ands, ps, &st );
      if ( p_x1g_gc_opt )
      {
      	float time_sat = float( clock() - begin_time ) / CLOCKS_PER_SEC;
      	x1g_network x1g = *p_x1g_gc_opt;
      	uint32_t num_oh{ 0u };
      	uint32_t num_xor{ 0u };
      	x1g.foreach_gate( [&]( auto const& n ) {
      		if ( x1g.is_onehot( n ) )
      		{
      			++num_oh;
      		}
      		else
      		{
      			++num_xor;
      		}
      	} );
      	exp_res( benchmark.func, num_xor, num_oh, st.num_vars, st.num_clauses, time_sat, true );
      	break;
      }
      else
      {
      	if ( i == and_topos_lib_6_x1g_oriented.size() - 1 )
      	{
      		exp_res( benchmark.func, 0u, 0u, 0u, 0u, ( float( clock() - begin_time ) / CLOCKS_PER_SEC ), false );
      	}
      }
    }
	}
	exp_res.save();
	exp_res.table();
}

/* Generate db_gc for the the 147,998-entry 6-input db_mc             */
/* Only functions with mc no more than 5 are taken into consideration */
void create_db_practical_x1g( std::string const& filename )
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
		++num_entries;
		if ( num_entries < 146384u )
		{
			continue;
		}
		if ( num_entries == 147000u )
		{
			break;
		}
		std::cout << "[i] working on the " << num_entries << "th representitive\n";
		//if ( num_entries == 10000u )
		//{
		//	break;
		//}
		//if ( num_entries <= 75u )
		//{
		//	std::cout << "[i] skip, as it is already in the database\n";
		//	continue;
		//}
		pos = static_cast<uint32_t>( line.find( '\t' ) );
    const auto name = line.substr( 0, pos );
    pos += 1u;
    auto tt_str = line.substr( pos, 16u );
    pos += 17u;
    const auto repr_str = line.substr( pos, 16u );
    pos += 17u;
    uint32_t mc = std::stoul( line.substr( pos, 1u ) );
    if ( mc <= 4u )
    {
    	std::cout << "[i] already in the database\n";
    	continue;
    }
    if ( mc > 5u )
    {
    	std::cout << "[i] encounter a challenge...\n";
    	//continue;
    }
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
    db_gc.open( "db_gc_practical_x1g_6_145000", std::ios::app );
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

/* Generate db_gc for the the 147,998-entry 6-input db_mc             */
/* Only functions with mc no more than 5 are taken into consideration */
void create_db_complete_x1g_6( std::string const& filename )
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

		pos = static_cast<uint32_t>( line.find( '\t' ) );
    const auto name = line.substr( 0, pos );
    pos += 1u;
    auto tt_str = line.substr( pos, 16u );
    pos += 17u;
    const auto repr_str = line.substr( pos, 16u );
    pos += 17u;
    uint32_t mc = std::stoul( line.substr( pos, 1u ) );
    if ( mc <= 4u )
    {
    	std::cout << "[i] skip, as it is already in the database ( db_gc_practical_x1g_6 )\n";
    	continue;
    }

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

    xag_network xag_gc_opt{ xag_mc_opt };
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
      ps.verbose = true;
      ps.verify_solution = true;

      auto const p_xag_gc_opt = optimum_gc_synthesis( tt_min_base, topo.num_ands, ps, nullptr );
      if ( p_xag_gc_opt )
      {
      	std::cout << "[i] Find " << ++num_cases_impr << " better implementations\n";
      	mc = topo.mc;
      	gc = topo.gc;
      	xag_gc_opt = *p_xag_gc_opt;
      	x1g_gc_opt = map_xag2x1g( xag_gc_opt );
      	break;
      }
    }

    num_gc_new += gc;

    /* Record the gc-optimal X1G implementation */
    std::ofstream db_gc;
    db_gc.open( "db_gc_complete_x1g_6", std::ios::app );
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

void create_db_complete_x1g_6_epfl()
{
	std::uint32_t num_entries{ 0u };
	std::uint32_t num_cases_impr{ 0u };
	std::uint32_t num_gc_old{ 0u };
	std::uint32_t num_gc_new{ 0u };

	std::ifstream db_mc;
	db_mc.open( "collected_funcs_epfl", std::ios::in );
	std::string line;
	uint32_t pos{ 0u };

	while ( std::getline( db_mc, line ) )
	{
		std::cout << "[i] working on the " << ++num_entries << "th representitive\n";

		pos = static_cast<uint32_t>( line.find( '\t' ) );
    const auto name = line.substr( 0, pos );
    pos += 1u;
    auto tt_str = line.substr( pos, 16u );
    pos += 17u;
    const auto repr_str = line.substr( pos, 16u );
    pos += 17u;
    uint32_t mc = std::stoul( line.substr( pos, 1u ) );
    if ( mc <= 4u )
    {
    	std::cout << "[i] skip, as it is already in the database ( db_gc_practical_x1g_6 )\n";
    	continue;
    }

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

    xag_network xag_gc_opt{ xag_mc_opt };
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
      ps.verbose = true;
      ps.verify_solution = true;

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
    db_gc.open( "db_gc_complete_x1g_6_epfl", std::ios::app );
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
	//mockturtle::test();
	//mockturtle::create_db_fast( "../experiments/db_mc" );
	//mockturtle::create_db_practical( "../experiments/db_mc" );
	//mockturtle::create_db_complete( "../experiments/db_mc_5" );
	//mockturtle::create_db_complete_x1g( "../experiments/db_mc_5" );
	//mockturtle::create_db_practical_x1g( "../experiments/db_mc" );
	//mockturtle::create_db_complete_x1g_6( "../experiments/db_mc" );
	mockturtle::x1g_affine_5_exact_synthesis();
	//mockturtle::create_db_complete_x1g_6_epfl();
	//mockturtle::experiment( "../experiments/db_mc_5" );
	//mockturtle::experiment2( "../experiments/db_mc_5" );

	return 0;
}
