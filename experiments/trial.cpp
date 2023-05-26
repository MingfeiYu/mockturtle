#include <iostream>

#include <kitty/kitty.hpp>
#include "../include/mockturtle/utils/include/percy.hpp"

void look_up_mc( kitty::dynamic_truth_table const& tt, uint32_t & mc, bool & valid_mc )
{
	/* Solution 2: forget about database */
	const auto tt_lookup = tt.num_vars() < 5u ? kitty::extend_to( tt, 5u ) : tt;
	mc = kitty::get_spectral_mc( tt_lookup );
	valid_mc = true;
}

int main()
{
	std::string func = "2888a000";
	kitty::dynamic_truth_table tt( 5u );
	//kitty::create_from_binary_string( tt, func );
	//tt._bits[0] = 32768u;
	kitty::create_from_hex_string( tt, func );
	kitty::dynamic_truth_table dont_cares = tt.construct();

	percy::spec_minmc spec;
	spec.fanin_size = 3u;
	spec.verbosity = 1u;
	spec.use_contribution_clauses = true;
	spec.conflict_limit = 0u;
	spec.set_output( tt );

	if ( !kitty::is_const0( dont_cares ) )
	{
		spec.set_dont_care( dont_cares );
	}

  //uint32_t mc{ 0u };
	//bool valid_mc{ false };
	//look_up_mc( tt, mc, valid_mc );
	//assert( valid_mc );

	// XOR3 - Onehot - gragh
	kitty::dynamic_truth_table const0{ 3 };
	kitty::dynamic_truth_table a{ 3 };
	kitty::dynamic_truth_table b{ 3 };
	kitty::dynamic_truth_table c{ 3 };
	kitty::create_nth_var( a, 0 );
	kitty::create_nth_var( b, 1 );
	kitty::create_nth_var( c, 2 );
	spec.add_free_primitive( const0 );													        // 00
	spec.add_free_primitive( a );													              // aa
	spec.add_free_primitive( b );													              // cc
	spec.add_free_primitive( c );													              // f0

	spec.add_primitive(  kitty::ternary_onehot(  a,  b,  c ) );         // 16
	spec.add_primitive( ~kitty::ternary_onehot( ~a,  b,  c ) );         // d6
	spec.add_primitive( ~kitty::ternary_onehot(  a, ~b,  c ) );         // b6
	spec.add_primitive( ~kitty::ternary_onehot(  a,  b, ~c ) );         // 9e
	spec.add_primitive(  kitty::ternary_onehot( ~a, ~b,  c ) );         // 86
	spec.add_primitive(  kitty::ternary_onehot(  a, ~b, ~c ) );         // 94
	spec.add_primitive(  kitty::ternary_onehot( ~a,  b, ~c ) );         // 92
	spec.add_primitive(  kitty::ternary_onehot( ~a, ~b, ~c ) );         // 68

	//spec.add_primitive(  kitty::ternary_onehot(  const0,  b,  c ) );    // 3c
	spec.add_primitive( ~kitty::ternary_onehot( ~const0,  b,  c ) );    // fc
	spec.add_primitive(  kitty::ternary_onehot( ~const0, ~b,  c ) );    // 0c
	spec.add_primitive(  kitty::ternary_onehot( ~const0,  b, ~c ) );    // 30
	spec.add_primitive(  kitty::ternary_onehot( ~const0, ~b, ~c ) );    // c0
	//spec.add_primitive(  kitty::ternary_onehot(  a,  const0,  c ) );    // 5a
	spec.add_primitive( ~kitty::ternary_onehot(  a, ~const0,  c ) );    // fa
	spec.add_primitive(  kitty::ternary_onehot( ~a, ~const0,  c ) );    // 0a
	spec.add_primitive(  kitty::ternary_onehot(  a, ~const0, ~c ) );    // 50
	spec.add_primitive(  kitty::ternary_onehot( ~a, ~const0, ~c ) );    // a0
	//spec.add_primitive(  kitty::ternary_onehot(  a,  b,  const0 ) );    // 66
	spec.add_primitive( ~kitty::ternary_onehot(  a,  b, ~const0 ) );    // ee
	spec.add_primitive(  kitty::ternary_onehot( ~a,  b, ~const0 ) );    // 22
	spec.add_primitive(  kitty::ternary_onehot(  a, ~b, ~const0 ) );    // 44
	spec.add_primitive(  kitty::ternary_onehot( ~a, ~b, ~const0 ) );    // 88

	spec.add_free_primitive( a ^ b ^ c);                                // 96
	spec.add_free_primitive( a ^ b );                                   // 66
	spec.add_free_primitive( b ^ c );                                   // 3c
	spec.add_free_primitive( a ^ c );                                   // 5a
	//

	/* XOR3 - AND3 - gragh
	kitty::dynamic_truth_table const0{ 3 };
	kitty::dynamic_truth_table a{ 3 };
	kitty::dynamic_truth_table b{ 3 };
	kitty::dynamic_truth_table c{ 3 };
	kitty::create_nth_var( a, 0 );
	kitty::create_nth_var( b, 1 );
	kitty::create_nth_var( c, 2 );
	spec.add_free_primitive( const0 );													        // 00
	spec.add_free_primitive( a );													              // aa
	spec.add_free_primitive( b );													              // cc
	spec.add_free_primitive( c );													              // f0

	spec.add_primitive(  kitty::ternary_and(  a,  b,  c ) );            // 80
	spec.add_primitive(  kitty::ternary_and( ~a,  b,  c ) );            // 40
	spec.add_primitive(  kitty::ternary_and(  a, ~b,  c ) );            // 20
	spec.add_primitive(  kitty::ternary_and(  a,  b, ~c ) );            // 08
	spec.add_primitive(  kitty::ternary_and( ~a, ~b,  c ) );            // 10
	spec.add_primitive(  kitty::ternary_and(  a, ~b, ~c ) );            // 02
	spec.add_primitive(  kitty::ternary_and( ~a,  b, ~c ) );            // 04
	spec.add_primitive( ~kitty::ternary_and( ~a, ~b, ~c ) );            // fe

	spec.add_primitive(  kitty::ternary_and( ~const0,  b,  c ) );       // c0
	spec.add_primitive(  kitty::ternary_and( ~const0, ~b,  c ) );       // 30
	spec.add_primitive(  kitty::ternary_and( ~const0,  b, ~c ) );       // 0c
	spec.add_primitive( ~kitty::ternary_and( ~const0, ~b, ~c ) );       // fc

	spec.add_primitive(  kitty::ternary_and(  a, ~const0,  c ) );       // a0
	spec.add_primitive(  kitty::ternary_and( ~a, ~const0,  c ) );       // 50
	spec.add_primitive(  kitty::ternary_and(  a, ~const0, ~c ) );       // 0a
	spec.add_primitive( ~kitty::ternary_and( ~a, ~const0, ~c ) );       // fa

	spec.add_primitive(  kitty::ternary_and(  a,  b, ~const0 ) );       // 88
	spec.add_primitive(  kitty::ternary_and( ~a,  b, ~const0 ) );       // 44
	spec.add_primitive(  kitty::ternary_and(  a, ~b, ~const0 ) );       // 22
	spec.add_primitive( ~kitty::ternary_and( ~a, ~b, ~const0 ) );       // ee

	spec.add_free_primitive( a ^ b ^ c);                                // 96
	spec.add_free_primitive( a ^ b );                                   // 66
	spec.add_free_primitive( b ^ c );                                   // 3c
	spec.add_free_primitive( a ^ c );                                   // 5a
	*/

	/* XOR2 - AND2 - graph
	kitty::dynamic_truth_table const0{ 2 };
	kitty::dynamic_truth_table a{ 2 };
	kitty::dynamic_truth_table b{ 2 };
	kitty::create_nth_var( a, 0 );
	kitty::create_nth_var( b, 1 );
	spec.add_free_primitive( const0 );													        // 0
	spec.add_free_primitive( a );													              // a
	spec.add_free_primitive( b );													              // c

	spec.add_primitive(  kitty::binary_and(  a,  b ) );                 // 8
	spec.add_primitive(  kitty::binary_and( ~a,  b ) );                 // 4
	spec.add_primitive(  kitty::binary_and(  a, ~b ) );                 // 2
	spec.add_primitive( ~kitty::binary_and( ~a, ~b ) );                 // e

	spec.add_free_primitive( a ^ b );                                   // 6
	*/

	uint32_t mc = 2u;
	spec.set_nfree( mc );
	percy::synth_stats synth_st;
	percy::chain_minmc chain;
	const auto result = percy::std_synthesize_minmc( spec, chain, &synth_st );
	if ( result == percy::success )
	{
		std::cout << "Success.\n";
	}
	else
	{
		std::cout << "Failure.\n";
	}

	return 0;
}