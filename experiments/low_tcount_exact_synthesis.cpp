#include <cstdint>
#include <unordered_map>
#include <fstream>

#include <kitty/constructors.hpp>
#include <kitty/operators.hpp>
#include <mockturtle/algorithms/low_tcount_exact_synthesis.hpp>
#include <mockturtle/algorithms/merge_split_xag.hpp>

#include <experiments.hpp>

namespace mockturtle
{

typedef struct AND_fence
{
	std::vector<uint32_t> fanin_size;
	uint32_t mc;
	uint32_t t_count;
} AND_fence;

static const std::vector<AND_fence> and_fences_lib = {
	{ { 1u }, 1u, 4u }, 
	{ { 2u }, 2u, 6u }, 
	{ { 1u, 1u }, 2u, 8u }, 
	{ { 3u }, 3u, 10u }, 
	{ { 1u, 2u }, 3u, 10u }, 
	{ { 2u, 1u }, 3u, 10u }, 
	{ { 1u, 1u, 1u }, 3u, 12u }, 
	{ { 4u }, 4u, 12u }, 
	{ { 2u, 2u }, 4u, 12u }, 
	{ { 1u, 3u }, 4u, 14u }, 
	{ { 3u, 1u }, 4u, 14u }, 
	{ { 1u, 1u, 2u }, 4u, 14u }, 
	{ { 1u, 2u, 1u }, 4u, 14u }, 
	{ { 2u, 1u, 1u }, 4u, 14u }, 
	{ { 1u, 1u, 1u, 1u }, 4u, 16u }, 
};

typedef struct represent
{
	std::string func;
	uint32_t mc;
	uint32_t t_count_old;
} represent;

static std::vector<represent> affine_represent_5 = {
	{"00000000", 0,  0}, 
	{"80000000", 4, 12}, 
	{"80008000", 3, 10}, 
	{"00808080", 4, 12}, 
	{"80808080", 2,  6}, 
	{"08888000", 3, 10}, 
	{"aa2a2a80", 4, 16}, 
	{"88080808", 4, 14}, 
	{"2888a000", 3, 12}, 
	{"f7788000", 3, 12}, 
	{"a8202020", 3, 12}, 
	{"08880888", 3, 10}, 
	{"bd686868", 4, 16}, 
	{"aa808080", 4, 16}, 
	{"7e686868", 4, 16}, 
	{"2208a208", 4, 16}, 
	{"08888888", 4, 14}, 
	{"88888888", 1,  4}, 
	{"ea404040", 3, 12}, 
	{"2a802a80", 2,  8}, 
	{"73d28c88", 3, 12}, 
	{"ea808080", 3, 12}, 
	{"a28280a0", 3, 12}, 
	{"13284c88", 3, 12}, 
	{"a2220888", 3, 10}, 
	{"aae6da80", 4, 16}, 
	{"58d87888", 4, 16}, 
	{"8c88ac28", 4, 16}, 
	{"8880f880", 4, 16}, 
	{"9ee8e888", 4, 16}, 
	{"4268c268", 4, 16}, 
	{"16704c80", 4, 16}, 
	{"78888888", 3, 10}, 
	{"4966bac0", 4, 16}, 
	{"372840a0", 4, 16}, 
	{"5208d288", 3, 12}, 
	{"7ca00428", 4, 16}, 
	{"f8880888", 3, 16}, 
	{"2ec0ae40", 4, 16}, 
	{"f888f888", 3, 12}, 
	{"58362ec0", 4, 16}, 
	{"0eb8f6c0", 4, 16}, 
	{"567cea40", 4, 16}, 
	{"f8887888", 4, 14}, 
	{"78887888", 2,  8}, 
	{"e72890a0", 4, 16}, 
	{"268cea40", 3, 12}, 
	{"6248eac0", 4, 16}
};

void print_xag( xag_network const& ntk )
{
	ntk.foreach_gate( [&]( auto const& n ) {
		std::cout << "Node " << n << " = " 
							<< ( ntk.is_and( n ) ? "AND( " : "XOR( " );
		ntk.foreach_fanin( n, [&]( auto const& ni ) {
			std::cout << ( ntk.is_complemented( ni ) ? "!" : "" ) 
								<< ntk.get_node( ni ) << " ";
		} );
		std::cout << ")\n";
	} );
	std::cout << "POs are: ";
	ntk.foreach_po( [&]( auto const& f ) {
		std::cout << ( ntk.is_complemented( f ) ? "!" : "" ) << "node " << ntk.get_node( f ) << " ";
	} );
	std::cout << "\n";
}

void learn_tcount( std::string const& filename )
{
	std::ifstream db;
	db.open( filename, std::ios::in );
	std::string line;
	uint32_t pos{ 0u };

	while ( std::getline( db, line ) )
	{
		pos = static_cast<uint32_t>( line.find( 'x' ) );
    auto tt_str = line.substr( ++pos, 8u );
    pos += 9u;
    line.erase( 0, pos );

    kitty::dynamic_truth_table tt( 5u );
    kitty::create_from_hex_string( tt, tt_str );
    auto num_vars = ( kitty::min_base_inplace( tt ) ).size();

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

    uint32_t t_count = merge_split_xag( xag_mc_opt );
    std::cout << tt_str << ": " << t_count << "\n";
  }
}

void low_tcount_exact_synthesis_affine_5()
{
	experiments::experiment<std::string, uint32_t, uint32_t, uint32_t, uint32_t, float, bool> exp_res( "experiment_low_tcount_xag", "function", "num_xor", "num_and", "T-count-old", "T-count", "runtime[s]", "suc." );

	uint32_t cases_impr{ 0u };
	uint32_t cases_impr_more_ands{ 0u };

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

    /* Look for the tcount-optimal XAG implementation */
    const clock_t begin_time = clock();
    for ( auto i{ 0u }; i < and_fences_lib.size(); ++i )
    {
    	auto topo = and_fences_lib[i];

    	if ( mc == 0u )
    	{
    		break;
    	}

    	if ( topo.mc < mc )
      {
        continue;
      }

      if ( topo.t_count >= benchmark.t_count_old )
      {
      	exp_res( benchmark.func, 0u, 0u, benchmark.t_count_old, topo.t_count, ( float( clock() - begin_time ) / CLOCKS_PER_SEC ), true );
      	//abort();
      	break;
      }

      low_tcount_exact_synthesis_params ps;
      //ps.verbose = true;
      //ps.verify_solution = true;
      low_tcount_exact_synthesis_stats st;

      auto const p_xag = low_tcount_exact_synthesis( tt_min_base, topo.fanin_size, ps, &st );
      if ( p_xag )
      {
      	float time_sat = float( clock() - begin_time ) / CLOCKS_PER_SEC;
      	xag_network xag = *p_xag;
      	uint32_t num_and{ 0u };
      	uint32_t num_xor{ 0u };
      	xag.foreach_gate( [&]( auto const& n ) {
      		if ( xag.is_and( n ) )
      		{
      			++num_and;
      		}
      		else
      		{
      			++num_xor;
      		}
      	} );
      	uint32_t t_count = merge_split_xag( xag );
      	exp_res( benchmark.func, num_xor, num_and, benchmark.t_count_old, t_count, time_sat, true );
      	if ( topo.mc > mc )
      	{
      		std::cout << "[i] The T-count-minimal XAG implementation of " << benchmark.func << ", ";
      		std::cout << "#AND2s previously is " << mc << ", now is " << topo.mc << "\n: ";
      		print_xag( xag );
      		++cases_impr_more_ands;
      	}
      	++cases_impr;
      	//if ( benchmark.func == "aa808080" )
      	//{
      	//	std::cout << "[i] The T-count-minimal XAG implementation of aa808080: \n";
      	//	print_xag( xag );
      	//	return;
      	//}
      	break;
      }
      else
      {
      	if ( i == and_fences_lib.size() - 1 )
      	{
      		exp_res( benchmark.func, 0u, 0u, benchmark.t_count_old, 0u, ( float( clock() - begin_time ) / CLOCKS_PER_SEC ), false );
      	}
      }
    }
	}
	exp_res.save();
	exp_res.table();
	std::cout << "Among 48 functions, better implementations are found for " << cases_impr << " of them.\n";
	std::cout << "For " << cases_impr_more_ands << " functions, the T-count-minimal XAG implementations needs more ANDs than required.\n";
}

void low_tcount_exact_synthesis_affine_5_gendb()
{
	std::ifstream db_mc;
	db_mc.open( "../experiments/db_mc_5", std::ios::in );
	std::string line;
	uint32_t pos{ 0u };

	while ( std::getline( db_mc, line ) )
	{
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
  	uint32_t t_count = merge_split_xag( xag_mc_opt );

  	kitty::dynamic_truth_table tt_min_base( num_vars );
    kitty::create_from_hex_string( tt_min_base, tt_str.substr( 0, 1 << ( num_vars - 2 ) ) );

    xag_network xag_tcount_opt{ xag_mc_opt };

    /* Look for the tcount-optimal XAG implementation */
    for ( auto const& fence: and_fences_lib )
    {
    	if ( mc == 0u )
    	{
    		break;
    	}

    	if ( fence.mc < mc )
      {
        continue;
      }

      if ( fence.t_count >= t_count )
      {
        break;
      }

      low_tcount_exact_synthesis_params ps;
      auto const p_xag_tcount_opt = low_tcount_exact_synthesis( tt_min_base, fence.fanin_size, ps, nullptr );
      if ( p_xag_tcount_opt )
      {
      	mc = fence.mc;
      	t_count = fence.t_count;
      	xag_tcount_opt = *p_xag_tcount_opt;
      	if ( false ) {

		    std::ofstream db_tcount;
		    db_tcount.open( "db_tcount_5_only_better", std::ios::app );
		    db_tcount << "0x" << repr_str << " ";
		    db_tcount << "0x" << tt_str << " ";
		    db_tcount << num_vars << " ";
		    db_tcount << t_count << " ";
		    xag_tcount_opt.foreach_gate( [&]( auto const& f ) {
		    	xag_tcount_opt.foreach_fanin( f, [&]( auto const& fi ) {
		    		db_tcount << static_cast<uint32_t>( ( fi.index << 1 ) + fi.complement ) << " ";
		    	} );
		    } );
		    xag_network::signal po = xag_tcount_opt.po_at( 0 );
		    db_tcount << static_cast<uint32_t>( ( po.index << 1 ) + po.complement ) << "\n";
		    db_tcount.close();}
      	break;
      }
    }

    
    if ( true ) {
    std::ofstream db_tcount;
    //db_tcount.open( "db_tcount_5", std::ios::app );
    db_tcount.open( "db_tcount_5_mc", std::ios::app );
    db_tcount << "0x" << repr_str << " ";
    db_tcount << "0x" << tt_str << " ";
    db_tcount << num_vars << " ";
    //db_tcount << t_count << " ";
    db_tcount << mc << " ";
    xag_tcount_opt.foreach_gate( [&]( auto const& f ) {
    	xag_tcount_opt.foreach_fanin( f, [&]( auto const& fi ) {
    		db_tcount << static_cast<uint32_t>( ( fi.index << 1 ) + fi.complement ) << " ";
    	} );
    } );
    xag_network::signal po = xag_tcount_opt.po_at( 0 );
    db_tcount << static_cast<uint32_t>( ( po.index << 1 ) + po.complement ) << "\n";
    db_tcount.close();}
	}
	db_mc.close();
}

} /* namespace mockturtle */

int main( void )
{
	//mockturtle::learn_tcount( "../experiments/db_mc_5" );
	//mockturtle::low_tcount_exact_synthesis_affine_5();
	mockturtle::low_tcount_exact_synthesis_affine_5_gendb();

	return 0;
}