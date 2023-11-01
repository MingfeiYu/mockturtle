#include <cstdint>
#include <string>
#include <array>
#include <iostream>
#include <fstream>
#include <fmt/format.h>
#include <iomanip>

#include <mockturtle/algorithms/xag2x1g.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/x1g.hpp>
#include <mockturtle/utils/include/percy.hpp>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/hash.hpp>
#include <kitty/spectral.hpp>
#include <kitty/constructors.hpp>
#include <kitty/properties.hpp>

#include <experiments.hpp>

typedef struct represent
{
	std::string func;
	uint32_t mc;
} represent;

static const std::string affine_5_input[] = {
	"00000000", "80000000", "80008000", "00808080", "80808080", "08888000", "aa2a2a80", "88080808", 
	"2888a000", "f7788000", "a8202020", "08880888", "bd686868", "aa808080", "7e686868", "2208a208", 
	"08888888", "88888888", "ea404040", "2a802a80", "73d28c88", "ea808080", "a28280a0", "13284c88", 
	"a2220888", "aae6da80", "58d87888", "8c88ac28", "8880f880", "9ee8e888", "4268c268", "16704c80", 
	"78888888", "4966bac0", "372840a0", "5208d288", "7ca00428", "f8880888", "2ec0ae40", "f888f888", 
	"58362ec0", "0eb8f6c0", "567cea40", "f8887888", "78887888", "e72890a0", "268cea40", "6248eac0"};

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

std::vector<std::string> npn_4()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 222u; ++i )
	{
		result.emplace_back( npn_4_input[i] );
	}

	return result;
}

std::vector<std::string> affine_5()
{
	std::vector<std::string> result;
	for ( auto i{ 0u }; i < 48u; ++i )
	{
		result.emplace_back( affine_5_input[i] );
	}

	return result;
}

void look_up_mc( kitty::dynamic_truth_table const& tt, uint32_t & mc, bool & valid_mc )
{
	const auto tt_lookup = tt.num_vars() < 5u ? kitty::extend_to( tt, 5u ) : tt;
	mc = kitty::get_spectral_mc( tt_lookup );
	valid_mc = true;
}

void x1g_db_gen()
{
	experiments::experiment<std::string, uint32_t, uint32_t, float, bool> exp_res( "db_gen", "function", "num_and", "num_onehot", "improvement %", "exact synth. suc." );

	auto const benchmarks = npn_4();

	std::ofstream fout( "x1g_npn_db.txt" );
	fout << "inline static const uint16_t subgraphs[]\n{\n\t";

	std::vector<uint32_t> npn_mc( 6 );

	mockturtle::x1g_network x1g;
	x1g.get_constant( false );
  x1g.create_pi();
  x1g.create_pi();
  x1g.create_pi();
  x1g.create_pi();

  uint32_t winning_case_cnt{ 0u };
  uint32_t num_ele_cnt{ 0u };
  float time_accum = 0.0;

	for ( auto const& benchmark: benchmarks )
	{
		std::cout << "[i] processing " << benchmark << std::endl;

		kitty::dynamic_truth_table function( 4u );
		kitty::create_from_hex_string( function, benchmark );

		uint32_t mc{ 0u };
		uint32_t num_oh{ 0u };
		const clock_t begin_time = clock();

		assert( function.num_vars() == 4u );
		bool const normal = kitty::is_normal( function );

		percy::spec_minmc spec;
		spec.nr_in = function.num_vars();
		spec.fanin_size = 3u;
		spec.verbosity = 0u;
		/* based on experience, following benchmarks require 5 steps to synthesize */
		if ( benchmark == "017f" || benchmark == "0180" || benchmark == "077e" || benchmark == "167e" || benchmark == "1697" || benchmark == "177e" )
		{
			spec.set_steps_upper( 5u );
		}
		else
		{
			spec.set_steps_upper( 4u );
		}
		spec.set_output( normal ? function : ~function );

		if ( int triv = spec.check_triv(); triv >= 0 )
		{
			if ( triv == 0 )
			{
				x1g.create_po( normal ? x1g.get_constant( false ) : x1g.get_constant( true ) );
			}
			else
			{
				x1g.create_po( normal ? x1g.make_signal( x1g.pi_at( triv - 1 ) ) : !x1g.make_signal( x1g.pi_at( triv - 1 ) ) );
			}

			exp_res( benchmark, 0u, 0u, 0., true );


			++npn_mc[0];
			time_accum += ( float( clock() - begin_time ) / CLOCKS_PER_SEC );


			continue;
		}

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
		spec.add_primitive( ~kitty::ternary_onehot(  a, b,  ~c ) );         // 9e
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

		bool valid_mc{ false };
		look_up_mc( function, mc, valid_mc );
		assert( valid_mc );

		uint32_t upper_bound_oh = mc;
		uint32_t lower_bound_oh = ( mc + 1 ) / 2;

		percy::chain_minmc chain;
		for ( uint32_t bound_nfree = lower_bound_oh; bound_nfree <= upper_bound_oh; ++bound_nfree )
		{
			spec.set_nfree( bound_nfree );
			percy::synth_stats synth_st;

			if ( const auto result = percy::std_synthesize_minmc( spec, chain, &synth_st );
					 result == percy::success )
			{


				if ( mc == 0u )
				{
					assert( bound_nfree == 0u );
					++npn_mc[0];
				}
				else if ( ( mc == 1u ) && ( bound_nfree == 1 ) )
				{
					++npn_mc[1];
				}
				else if ( mc == 2u )
				{
					if ( bound_nfree == 1u )
					{
						++npn_mc[2];
					}
					else
					{
						++npn_mc[3];
					}
				}
				else if ( mc == 3u )
				{
					if ( bound_nfree == 2u )
					{
						++npn_mc[4];
					}
					else
					{
						++npn_mc[5];
					}
				}
				else
				{
					std::cout << "mc = " << mc << "\n";
					abort();
				}

				num_oh = bound_nfree;

				std::vector<mockturtle::x1g_network::signal> signals;
				signals.emplace_back( x1g.make_signal( x1g.pi_at( 0 ) ) );
				signals.emplace_back( x1g.make_signal( x1g.pi_at( 1 ) ) );
				signals.emplace_back( x1g.make_signal( x1g.pi_at( 2 ) ) );
				signals.emplace_back( x1g.make_signal( x1g.pi_at( 3 ) ) );
				/*
				for ( uint32_t i = 0u; i < static_cast<uint32_t> ( chain.get_nr_steps() ); ++i )
				{
					auto const c1 = signals[chain.get_step( i )[0]];
					auto const c2 = signals[chain.get_step( i )[1]];
					auto const c3 = signals[chain.get_step( i )[2]];

					switch ( chain.get_operator( i )._bits[0] )
					{
					case 0x00:
						signals.emplace_back( x1g.get_constant( false ) );
						break;
					case 0xaa:
						signals.emplace_back( c1 );
						break;
					case 0xcc:
						signals.emplace_back( c2 );
						break;
					case 0xf0:
						signals.emplace_back( c3 );
						break;
					case 0x16:
						signals.emplace_back(  x1g.create_onehot(  c1,  c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0xd6:
						signals.emplace_back( !x1g.create_onehot( !c1,  c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0xb6:
						signals.emplace_back( !x1g.create_onehot(  c1, !c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 1 ) << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x9e:
						signals.emplace_back( !x1g.create_onehot(  c1,  c2, !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0x86:
						signals.emplace_back(  x1g.create_onehot( !c1, !c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x94:
						signals.emplace_back(  x1g.create_onehot(  c1, !c2, !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0x92:
						signals.emplace_back(  x1g.create_onehot( !c1,  c2, !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0x68:
						signals.emplace_back(  x1g.create_onehot( !c1, !c2, !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0xfc:
						signals.emplace_back( !x1g.create_onehot(  x1g.get_constant( true ),  c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x0c:
						signals.emplace_back(  x1g.create_onehot(  x1g.get_constant( true ), !c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x30:
						signals.emplace_back(  x1g.create_onehot(  x1g.get_constant( true ),  c2, !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0xc0:
						signals.emplace_back(  x1g.create_onehot(  x1g.get_constant( true ), !c2, !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0xfa:
						signals.emplace_back( !x1g.create_onehot(  c1,  x1g.get_constant( true ),  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x0a:
						signals.emplace_back(  x1g.create_onehot( !c1,  x1g.get_constant( true ),  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x50:
						signals.emplace_back(  x1g.create_onehot(  c1,  x1g.get_constant( true ), !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0xa0:
						signals.emplace_back(  x1g.create_onehot( !c1,  x1g.get_constant( true ), !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0xee:
						signals.emplace_back( !x1g.create_onehot(  c1,  c2,  x1g.get_constant( true ) ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						++num_ele_cnt;
						break;
					case 0x22:
						signals.emplace_back(  x1g.create_onehot( !c1,  c2,  x1g.get_constant( true ) ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						++num_ele_cnt;
						break;
					case 0x44:
						signals.emplace_back(  x1g.create_onehot(  c1, !c2,  x1g.get_constant( true ) ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						++num_ele_cnt;
						break;
					case 0x88:
						signals.emplace_back(  x1g.create_onehot( !c1, !c2,  x1g.get_constant( true ) ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						++num_ele_cnt;
						break;
					case 0x3c:
						signals.emplace_back( x1g.create_xor( c2, c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( false ).data << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x5a:
						signals.emplace_back( x1g.create_xor( c1, c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( false ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x66:
						signals.emplace_back( x1g.create_xor( c1, c2 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( false ).data << ",";
						++num_ele_cnt;
						break;
					case 0x96:
						signals.emplace_back( x1g.create_xor3( c1, c2, c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( ( ( c2.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					default:
						std::cerr << "[e] unsupported operation " << kitty::to_hex( chain.get_operator( i ) ) << std::endl;
						assert( false );
						break;
					}
				}
				*/

				//
				for ( uint32_t i = 0u; i < static_cast<uint32_t> ( chain.get_nr_steps() ); ++i )
				{
					auto const c1 = signals[chain.get_step( i )[0]];
					auto const c2 = signals[chain.get_step( i )[1]];
					auto const c3 = signals[chain.get_step( i )[2]];

					switch ( chain.get_operator( i )._bits[0] )
					{
					case 0x00:
						signals.emplace_back( x1g.get_constant( false ) );
						break;
					case 0xaa:
						signals.emplace_back( c1 );
						break;
					case 0xcc:
						signals.emplace_back( c2 );
						break;
					case 0xf0:
						signals.emplace_back( c3 );
						break;
					case 0x16:
						signals.emplace_back(  x1g.create_onehot(  c1,  c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0xd6:
						signals.emplace_back( !x1g.create_onehot( !c1,  c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0xb6:
						signals.emplace_back( !x1g.create_onehot(  c1, !c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x9e:
						signals.emplace_back( !x1g.create_onehot(  c1,  c2, !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0x86:
						signals.emplace_back(  x1g.create_onehot( !c1, !c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x94:
						signals.emplace_back(  x1g.create_onehot(  c1, !c2, !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0x92:
						signals.emplace_back(  x1g.create_onehot( !c1,  c2, !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0x68:
						signals.emplace_back(  x1g.create_onehot( !c1, !c2, !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0xfc:
						signals.emplace_back( !x1g.create_onehot(  x1g.get_constant( true ),  c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x0c:
						signals.emplace_back(  x1g.create_onehot(  x1g.get_constant( true ), !c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x30:
						signals.emplace_back(  x1g.create_onehot(  x1g.get_constant( true ),  c2, !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0xc0:
						signals.emplace_back(  x1g.create_onehot(  x1g.get_constant( true ), !c2, !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0xfa:
						signals.emplace_back( !x1g.create_onehot(  c1,  x1g.get_constant( true ),  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x0a:
						signals.emplace_back(  x1g.create_onehot( !c1,  x1g.get_constant( true ),  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x50:
						signals.emplace_back(  x1g.create_onehot(  c1,  x1g.get_constant( true ), !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0xa0:
						signals.emplace_back(  x1g.create_onehot( !c1,  x1g.get_constant( true ), !c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						++num_ele_cnt;
						break;
					case 0xee:
						signals.emplace_back( !x1g.create_onehot(  c1,  c2,  x1g.get_constant( true ) ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						++num_ele_cnt;
						break;
					case 0x22:
						signals.emplace_back(  x1g.create_onehot( !c1,  c2,  x1g.get_constant( true ) ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						++num_ele_cnt;
						break;
					case 0x44:
						signals.emplace_back(  x1g.create_onehot(  c1, !c2,  x1g.get_constant( true ) ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						++num_ele_cnt;
						break;
					case 0x88:
						signals.emplace_back(  x1g.create_onehot( !c1, !c2,  x1g.get_constant( true ) ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						++num_ele_cnt;
						break;
					case 0x3c:
						signals.emplace_back( x1g.create_xor( c2, c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( false ).data << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x5a:
						signals.emplace_back( x1g.create_xor( c1, c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( false ).data << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x66:
						signals.emplace_back( x1g.create_xor( c1, c2 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << x1g.get_constant( false ).data << ",";
						++num_ele_cnt;
						break;
					case 0x96:
						signals.emplace_back( x1g.create_xor3( c1, c2, c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					default:
						std::cerr << "[e] unsupported operation " << kitty::to_hex( chain.get_operator( i ) ) << std::endl;
						assert( false );
						break;
					}
				}
				//

				auto const output_signal = signals.back();
				x1g.create_po( normal ? output_signal : !output_signal );

				float improve = ( ( static_cast<float> ( mc ) - static_cast<float> ( num_oh ) ) / static_cast<float> ( mc ) ) * 100;

				exp_res( benchmark, mc, num_oh, improve, true );

				if ( mc > num_oh )
				{
					++winning_case_cnt;
				}
				time_accum += ( float( clock() - begin_time ) / CLOCKS_PER_SEC );
				break;
			}
			else
			{
				if ( bound_nfree == upper_bound_oh )
				{
					std::cout << "Failed to synthesize exact circuit.\n";
					abort();
				}
			}
		}
	}
	fout << "\n};";
	fout.close();

	std::cout << "Get gain on " << winning_case_cnt << " functions.\n";
	std::cout << "An array with " << num_ele_cnt << " elements is created.\n";
	for ( auto i = 0u; i < npn_mc.size(); ++i )
	{
		std::cout << "npn_mc[" << i << "] is " << npn_mc[i] << "\n";
	}

	std::cout << "Spent " << time_accum / benchmarks.size() << "s in average\n";
	std::cout << std::endl;

	exp_res.save();
	exp_res.table();
}

void x1g_affine_5_exact_synthesis()
{
	experiments::experiment<std::string, uint32_t, uint32_t, uint32_t, uint32_t, float, bool> exp_res( "x1g_affine_5", "function", "num_xor", "num_onehot", "num_variables", "num_clauses", "runtime[s]", "exact synth. suc." );
	//auto const benchmarks = affine_5();
	//std::vector<uint32_t> affine_5_mc;
	//std::ofstream fout( "x1g_affine_5.txt" );
	//fout << "inline static const uint16_t subgraphs[]\n{\n\t";
	//std::cout << "std::vector<std::pair<std::string, mockturtle::x1g_network::signal>> representitives = {\n";

	mockturtle::x1g_network x1g;
	x1g.get_constant( false );
	for ( auto i{ 0u }; i < 5u; ++i )
	{
		x1g.create_pi();	
	}

	for ( auto const& benchmark: affine_represent_5 )
	{
		std::cout << "[i] processing " << benchmark.func << std::endl;
		kitty::dynamic_truth_table function( 5u );
		kitty::create_from_hex_string( function, benchmark.func );

		uint32_t mc = benchmark.mc;
		uint32_t num_oh{ 0u };
		float time_sat{ 0.0 };
		assert( function.num_vars() == 5u );
		bool const normal = kitty::is_normal( function );

		percy::spec_minmc spec;
		spec.nr_in = function.num_vars();
		spec.fanin_size = 3u;
		spec.verbosity = 0u;
		spec.conflict_limit = 1000000u;
		spec.set_output( normal ? function : ~function );

		if ( int triv = spec.check_triv(); triv >= 0 )
		{
			if ( triv == 0 )
			{
				x1g.create_po( normal ? x1g.get_constant( false ) : x1g.get_constant( true ) );
			}
			else
			{
				x1g.create_po( normal ? x1g.make_signal( x1g.pi_at( triv - 1 ) ) : !x1g.make_signal( x1g.pi_at( triv - 1 ) ) );
			}

			exp_res( benchmark.func, 0u, 0u, 0u, 0u, 0., true );
			//affine_5_mc.emplace_back( 0u );
			continue;
		}

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
		spec.add_primitive( ~kitty::ternary_onehot(  a, b,  ~c ) );         // 9e
		spec.add_primitive(  kitty::ternary_onehot( ~a, ~b,  c ) );         // 86
		spec.add_primitive(  kitty::ternary_onehot(  a, ~b, ~c ) );         // 94
		spec.add_primitive(  kitty::ternary_onehot( ~a,  b, ~c ) );         // 92
		spec.add_primitive(  kitty::ternary_onehot( ~a, ~b, ~c ) );         // 68
		spec.add_primitive( ~kitty::ternary_onehot( ~const0,  b,  c ) );    // fc
		spec.add_primitive(  kitty::ternary_onehot( ~const0, ~b,  c ) );    // 0c
		spec.add_primitive(  kitty::ternary_onehot( ~const0,  b, ~c ) );    // 30
		spec.add_primitive(  kitty::ternary_onehot( ~const0, ~b, ~c ) );    // c0
		spec.add_primitive( ~kitty::ternary_onehot(  a, ~const0,  c ) );    // fa
		spec.add_primitive(  kitty::ternary_onehot( ~a, ~const0,  c ) );    // 0a
		spec.add_primitive(  kitty::ternary_onehot(  a, ~const0, ~c ) );    // 50
		spec.add_primitive(  kitty::ternary_onehot( ~a, ~const0, ~c ) );    // a0
		spec.add_primitive( ~kitty::ternary_onehot(  a,  b, ~const0 ) );    // ee
		spec.add_primitive(  kitty::ternary_onehot( ~a,  b, ~const0 ) );    // 22
		spec.add_primitive(  kitty::ternary_onehot(  a, ~b, ~const0 ) );    // 44
		spec.add_primitive(  kitty::ternary_onehot( ~a, ~b, ~const0 ) );    // 88

		spec.add_free_primitive( a ^ b ^ c);                                // 96
		spec.add_free_primitive( a ^ b );                                   // 66
		spec.add_free_primitive( b ^ c );                                   // 3c
		spec.add_free_primitive( a ^ c );                                   // 5a

		//bool valid_mc{ false };
		//look_up_mc( function, mc, valid_mc );
		//assert( valid_mc );
		//affine_5_mc.emplace_back( mc );

		uint32_t upper_bound_oh = mc;
		uint32_t lower_bound_oh = ( mc + 1 ) / 2;

		const clock_t begin_time = clock();
		percy::chain_minmc chain;
		for ( uint32_t bound_nfree = lower_bound_oh; bound_nfree <= upper_bound_oh; ++bound_nfree )
		{
			spec.set_nfree( bound_nfree );
			percy::synth_stats synth_st;

			if ( const auto result = percy::std_synthesize_minmc( spec, chain, &synth_st );
					 result == percy::success )
			{
				num_oh = bound_nfree;
				uint32_t num_xor{ 0u };

				std::vector<mockturtle::x1g_network::signal> signals;
				signals.emplace_back( x1g.make_signal( x1g.pi_at( 0 ) ) );
				signals.emplace_back( x1g.make_signal( x1g.pi_at( 1 ) ) );
				signals.emplace_back( x1g.make_signal( x1g.pi_at( 2 ) ) );
				signals.emplace_back( x1g.make_signal( x1g.pi_at( 3 ) ) );
				signals.emplace_back( x1g.make_signal( x1g.pi_at( 4 ) ) );

				for ( uint32_t i = 0u; i < static_cast<uint32_t> ( chain.get_nr_steps() ); ++i )
				{
					auto const c1 = signals[chain.get_step( i )[0]];
					auto const c2 = signals[chain.get_step( i )[1]];
					auto const c3 = signals[chain.get_step( i )[2]];

					switch ( chain.get_operator( i )._bits[0] )
					{
					case 0x00:
						signals.emplace_back( x1g.get_constant( false ) );
						break;
					case 0xaa:
						signals.emplace_back( c1 );
						break;
					case 0xcc:
						signals.emplace_back( c2 );
						break;
					case 0xf0:
						signals.emplace_back( c3 );
						break;
					case 0x16:
						signals.emplace_back(  x1g.create_onehot(  c1,  c2,  c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						break;
					case 0xd6:
						signals.emplace_back( !x1g.create_onehot( !c1,  c2,  c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						break;
					case 0xb6:
						signals.emplace_back( !x1g.create_onehot(  c1, !c2,  c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						break;
					case 0x9e:
						signals.emplace_back( !x1g.create_onehot(  c1,  c2, !c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						break;
					case 0x86:
						signals.emplace_back(  x1g.create_onehot( !c1, !c2,  c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						break;
					case 0x94:
						signals.emplace_back(  x1g.create_onehot(  c1, !c2, !c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						break;
					case 0x92:
						signals.emplace_back(  x1g.create_onehot( !c1,  c2, !c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						break;
					case 0x68:
						signals.emplace_back(  x1g.create_onehot( !c1, !c2, !c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						break;
					case 0xfc:
						signals.emplace_back( !x1g.create_onehot(  x1g.get_constant( true ),  c2,  c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						break;
					case 0x0c:
						signals.emplace_back(  x1g.create_onehot(  x1g.get_constant( true ), !c2,  c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						break;
					case 0x30:
						signals.emplace_back(  x1g.create_onehot(  x1g.get_constant( true ),  c2, !c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						break;
					case 0xc0:
						signals.emplace_back(  x1g.create_onehot(  x1g.get_constant( true ), !c2, !c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						break;
					case 0xfa:
						signals.emplace_back( !x1g.create_onehot(  c1,  x1g.get_constant( true ),  c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						break;
					case 0x0a:
						signals.emplace_back(  x1g.create_onehot( !c1,  x1g.get_constant( true ),  c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						break;
					case 0x50:
						signals.emplace_back(  x1g.create_onehot(  c1,  x1g.get_constant( true ), !c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						break;
					case 0xa0:
						signals.emplace_back(  x1g.create_onehot( !c1,  x1g.get_constant( true ), !c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
						break;
					case 0xee:
						signals.emplace_back( !x1g.create_onehot(  c1,  c2,  x1g.get_constant( true ) ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						break;
					case 0x22:
						signals.emplace_back(  x1g.create_onehot( !c1,  c2,  x1g.get_constant( true ) ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						break;
					case 0x44:
						signals.emplace_back(  x1g.create_onehot(  c1, !c2,  x1g.get_constant( true ) ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						break;
					case 0x88:
						signals.emplace_back(  x1g.create_onehot( !c1, !c2,  x1g.get_constant( true ) ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << x1g.get_constant( true ).data << ",";
						break;
					case 0x3c:
						signals.emplace_back( x1g.create_xor( c2, c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( x1g.get_constant( false ).data << 1 ) ^ 1 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_xor;
						break;
					case 0x5a:
						signals.emplace_back( x1g.create_xor( c1, c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << x1g.get_constant( false ).data << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_xor;
						break;
					case 0x66:
						signals.emplace_back( x1g.create_xor( c1, c2 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << x1g.get_constant( false ).data << ",";
						++num_xor;
						break;
					case 0x96:
						signals.emplace_back( x1g.create_xor3( c1, c2, c3 ) );
						//fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						//fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_xor;
						break;
					default:
						std::cerr << "[e] unsupported operation " << kitty::to_hex( chain.get_operator( i ) ) << std::endl;
						assert( false );
						break;
					}
				}

				auto const output_signal = signals.back();
				x1g.create_po( normal ? output_signal : !output_signal );

				time_sat = ( float( clock() - begin_time ) / CLOCKS_PER_SEC );
				//std::cout << "{" << benchmark << ", " 
				//					<< ( ( !normal ^ x1g.is_complemented( output_signal ) ) ? "!" : "" ) << x1g.get_node( output_signal ) 
				//					<< "}, \n";

				exp_res( benchmark.func, num_xor, num_oh, static_cast<uint32_t> ( synth_st.nr_vars ), static_cast<uint32_t> ( synth_st.nr_clauses ), time_sat, true );
				break;
			}
			else
			{
				if ( bound_nfree == upper_bound_oh )
				{
					exp_res( benchmark.func, 0u, 0u, 0u, 0u, ( float( clock() - begin_time ) / CLOCKS_PER_SEC ), false );
				}
			}
		}
	}
	//fout << "\n};";
	//fout.close();
	//std::cout << "};\n\n";
	//assert( benchmarks.size() == affine_5_mc.size() );
	//std::cout << "std::vector<represent> affine_represent_5 = {\n";
	//for ( auto i{ 0u }; i < benchmarks.size(); ++i )
	//{
	//	std::cout << "{" << benchmarks[i] << ", " << affine_5_mc[i] << "}, \n";
	//}
	//std::cout << "};\n";
	exp_res.save();
	exp_res.table();
}

void x1g_affine_5_direct_mapping()
{
	experiments::experiment<std::string, uint32_t> exp_res( "x1g_affine_5_direct_mapping", "function", "num_onehot" );

	std::ifstream db_mc;
	db_mc.open( "../experiments/db_mc_5", std::ios::in );
	std::string line;
	uint32_t pos{ 0u };
	uint32_t accum_num_oh{ 0u };
	uint32_t accum_num_oh_special{ 0u };

	using xag_network = mockturtle::xag_network;
	using x1g_network = mockturtle::x1g_network;

	const clock_t begin_time = clock();
	while ( std::getline( db_mc, line ) )
	{
		pos = static_cast<uint32_t>( line.find( 'x' ) );
    auto tt_str = line.substr( ++pos, 8u );
    std::cout << "[i] Working on function " << tt_str << "\n";
    pos += 9u;
    line.erase( 0, pos );

    kitty::dynamic_truth_table tt( 5u );
    kitty::create_from_hex_string( tt, tt_str );
    auto num_vars = ( kitty::min_base_inplace( tt ) ).size();

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

  	x1g_network x1g = map_xag2x1g( xag_mc_opt );
  	uint32_t num_oh{ 0u };
  	x1g.foreach_gate( [&]( auto const& n ) {
  		if ( x1g.is_onehot( n ) )
  		{
  			++num_oh;
  		}
  	} );

  	exp_res( tt_str, num_oh );
  	accum_num_oh += num_oh;

  	if ( tt_str == "aae6da80" || tt_str == "4966bac0" || tt_str == "58362ec0" || tt_str == "0eb8f6c0" || tt_str == "567cea40" )
		{
			continue;
		}
		accum_num_oh_special += num_oh;
	}
	std::cout << "Accumulated number of OneHots is: " << accum_num_oh << ", if exclude special case: " << accum_num_oh_special << "\n";
	std::cout << "Runtime overhead is : " << ( float( clock() - begin_time ) / CLOCKS_PER_SEC ) << "\n";
	exp_res.save();
}

int main()
{	
	//x1g_db_gen();
	//x1g_affine_5_exact_synthesis();
	x1g_affine_5_direct_mapping();

	return 0;
}