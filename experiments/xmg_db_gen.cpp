#include <string>
#include <array>
#include <iostream>
#include <fstream>
#include <fmt/format.h>
#include <iomanip>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/utils/include/percy.hpp>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/hash.hpp>
#include <kitty/spectral.hpp>
#include <kitty/constructors.hpp>

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

std::vector<std::string> npn_4()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 222u; ++i )
	{
		result.emplace_back( npn_4_input[i] );
	}

	return result;
}

void look_up_mc( kitty::dynamic_truth_table const& tt, uint32_t & mc, bool & valid_mc )
{
	const auto tt_lookup = tt.num_vars() < 5u ? kitty::extend_to( tt, 5u ) : tt;
	mc = kitty::get_spectral_mc( tt_lookup );
	valid_mc = true;
}

void xmg_db_gen()
{
	auto const benchmarks = npn_4();

	std::ofstream fout( "xmg_npn_db.txt" );
	fout << "inline static const uint16_t subgraphs[]\n{\n\t";

	mockturtle::xmg_network xmg;
	xmg.get_constant( false );
  xmg.create_pi();
  xmg.create_pi();
  xmg.create_pi();
  xmg.create_pi();

  uint32_t winning_case_cnt{ 0u };
  uint32_t num_ele_cnt{ 0u };
	for ( auto const& benchmark: benchmarks )
	{
		std::cout << "[i] processing " << benchmark << std::endl;

		kitty::dynamic_truth_table function( 4u );
		kitty::create_from_hex_string( function, benchmark );

		uint32_t mc{ 0u };

		assert( function.num_vars() == 4u );
		bool const normal = kitty::is_normal( function );

		percy::spec_minmc spec;
		spec.nr_in = function.num_vars();
		spec.fanin_size = 3u;
		spec.verbosity = 0u;
		spec.set_steps_upper( 5u );
		spec.set_output( normal ? function : ~function );

		if ( int triv = spec.check_triv(); triv >= 0 )
		{
			if ( triv == 0 )
			{
				xmg.create_po( normal ? xmg.get_constant( false ) : xmg.get_constant( true ) );
			}
			else
			{
				xmg.create_po( normal ? xmg.make_signal( xmg.pi_at( triv - 1 ) ) : !xmg.make_signal( xmg.pi_at( triv - 1 ) ) );
			}
			continue;
		}

		kitty::dynamic_truth_table const0{ 3 };
		kitty::dynamic_truth_table a{ 3 };
		kitty::dynamic_truth_table b{ 3 };
		kitty::dynamic_truth_table c{ 3 };
		kitty::create_nth_var( a, 0 );
		kitty::create_nth_var( b, 1 );
		kitty::create_nth_var( c, 2 );
		spec.add_free_primitive( const0 );													          // 00
		spec.add_free_primitive( a );													                // aa
		spec.add_free_primitive( b );													                // cc
		spec.add_free_primitive( c );													                // f0

		spec.add_primitive( kitty::ternary_majority(  a,  b,  c ) );          // e8
	  spec.add_primitive( kitty::ternary_majority( ~a,  b,  c ) );          // d4
	  spec.add_primitive( kitty::ternary_majority(  a, ~b,  c ) );          // b2
	  spec.add_primitive( kitty::ternary_majority(  a,  b, ~c ) );          // 8e

		spec.add_primitive( kitty::ternary_majority(  const0,  b,  c ) );     // c0
	  spec.add_primitive( kitty::ternary_majority( ~const0,  b,  c ) );     // fc
	  spec.add_primitive( kitty::ternary_majority(  const0, ~b,  c ) );     // 30
	  spec.add_primitive( kitty::ternary_majority(  const0,  b, ~c ) );     // 0c
	  spec.add_primitive( kitty::ternary_majority(  a,  const0,  c ) );     // a0
	  spec.add_primitive( kitty::ternary_majority( ~a,  const0,  c ) );     // 50
	  spec.add_primitive( kitty::ternary_majority(  a, ~const0,  c ) );     // fa
	  spec.add_primitive( kitty::ternary_majority(  a,  const0, ~c ) );     // 0a
	  spec.add_primitive( kitty::ternary_majority(  a,  b,  const0 ) );     // 88
	  spec.add_primitive( kitty::ternary_majority(  a,  b, ~const0 ) );     // ee
	  spec.add_primitive( kitty::ternary_majority( ~a,  b,  const0 ) );     // 44
	  spec.add_primitive( kitty::ternary_majority(  a, ~b,  const0 ) );     // 22

		spec.add_free_primitive( a ^ b ^ c);                                  // 96
		spec.add_free_primitive( a ^ b );                                     // 66
		spec.add_free_primitive( b ^ c );                                     // 3c
		spec.add_free_primitive( a ^ c );                                     // 5a

		bool valid_mc{ false };
		look_up_mc( function, mc, valid_mc );
		assert( valid_mc );

		uint32_t upper_bound_oh = mc;
		uint32_t lower_bound_oh = mc;

		percy::chain_minmc chain;
		for ( uint32_t bound_nfree = lower_bound_oh; bound_nfree <= upper_bound_oh; ++bound_nfree )
		{
			spec.set_nfree( bound_nfree );
			percy::synth_stats synth_st;

			if ( const auto result = percy::std_synthesize_minmc( spec, chain, &synth_st );
					 result == percy::success )
			{
				num_oh = bound_nfree;

				std::vector<mockturtle::xmg_network::signal> signals;
				signals.emplace_back( xmg.make_signal( xmg.pi_at( 0 ) ) );
				signals.emplace_back( xmg.make_signal( xmg.pi_at( 1 ) ) );
				signals.emplace_back( xmg.make_signal( xmg.pi_at( 2 ) ) );
				signals.emplace_back( xmg.make_signal( xmg.pi_at( 3 ) ) );
				
				for ( uint32_t i = 0u; i < static_cast<uint32_t> ( chain.get_nr_steps() ); ++i )
				{
					auto const c1 = signals[chain.get_step( i )[0]];
					auto const c2 = signals[chain.get_step( i )[1]];
					auto const c3 = signals[chain.get_step( i )[2]];

					switch ( chain.get_operator( i )._bits[0] )
					{
					case 0x00:
						signals.emplace_back( xmg.get_constant( false ) );
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
					case 0xe8:
	          signals.emplace_back(  xmg.create_maj(  c1,  c2,  c3 ) );
	          fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
	          break;
	        case 0xd4:
	          signals.emplace_back(  xmg.create_maj( !c1,  c2,  c3 ) );
	          fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
	          break;
	        case 0xb2:
	          signals.emplace_back(  xmg.create_maj(  c1, !c2,  c3 ) );
	          fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
	          break;
	        case 0x8e:
	          signals.emplace_back(  xmg.create_maj(  c1,  c2, !c3 ) );
	          fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
	          break;
	        case 0xc0:
	          signals.emplace_back(  xmg.create_maj(  xmg.get_constant( false ),  c2,  c3 ) );
	          fout << "0x" << std::setbase( 16 ) << ( ( xmg.get_constant( false ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
	          break;
	        case 0xfc:
	          signals.emplace_back(  xmg.create_maj( !xmg.get_constant( false ),  c2,  c3 ) );
	          fout << "0x" << std::setbase( 16 ) << ( ( xmg.get_constant( true ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
	          break;
	        case 0x30:
	          signals.emplace_back(  xmg.create_maj(  xmg.get_constant( false ), !c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( xmg.get_constant( false ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
	          break;
	        case 0x0c:
	          signals.emplace_back(  xmg.create_maj(  xmg.get_constant( false ),  c2, !c3 ) );
	          fout << "0x" << std::setbase( 16 ) << ( ( xmg.get_constant( false ).data << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
	          break;
	        case 0xa0:
	          signals.emplace_back(  xmg.create_maj(  c1,  xmg.get_constant( false ),  c3 ) );
	          fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << xmg.get_constant( false ).data << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
	          break;
	        case 0x50:
	          signals.emplace_back(  xmg.create_maj( !c1,  xmg.get_constant( false ),  c3 ) );
	          fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << xmg.get_constant( false ).data << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
	          break;
	        case 0xfa:
	          signals.emplace_back(  xmg.create_maj(  c1, !xmg.get_constant( false ),  c3 ) );
	          fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << xmg.get_constant( true ).data << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
	          break;
	        case 0x0a:
	          signals.emplace_back(  xmg.create_maj(  c1,  xmg.get_constant( false ), !c3 ) );
	          fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << xmg.get_constant( false ).data << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 1 ) << ",";
	          break;
	        case 0x88:
	          signals.emplace_back(  xmg.create_maj(  c1,  c2,  xmg.get_constant( false ) ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << xmg.get_constant( false ).data << ",";	          
	          break;
	        case 0xee:
	          signals.emplace_back(  xmg.create_maj(  c1,  c2, !xmg.get_constant( false ) ) );
	          fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << xmg.get_constant( true ).data << ",";
	          break;
	        case 0x44:
	          signals.emplace_back(  xmg.create_maj( !c1,  c2,  xmg.get_constant( false ) ) );
	          fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 1 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << xmg.get_constant( false ).data << ",";
	          break;
	        case 0x22:
	          signals.emplace_back(  xmg.create_maj(  c1, !c2,  xmg.get_constant( false ) ) );
	          fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << xmg.get_constant( false ).data << ",";
	          break;
					case 0x3c:
						signals.emplace_back(  xmg.create_xor(  c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( xmg.get_constant( false ).data << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						++num_ele_cnt;
						break;
					case 0x5a:
						signals.emplace_back(  xmg.create_xor(  c1,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << xmg.get_constant( false ).data << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						break;
					case 0x66:
						signals.emplace_back(  xmg.create_xor(  c1,  c2 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << xmg.get_constant( false ).data << ",";
						break;
					case 0x96:
						signals.emplace_back(  xmg.create_xor3(  c1,  c2,  c3 ) );
						fout << "0x" << std::setbase( 16 ) << ( ( ( c1.data ^ 0 ) << 1 ) ^ 1 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c2.data ^ 0 ) << ",";
						fout << "0x" << std::setbase( 16 ) << ( c3.data ^ 0 ) << ",";
						break;
					default:
						std::cerr << "[e] unsupported operation " << kitty::to_hex( chain.get_operator( i ) ) << std::endl;
						assert( false );
						break;
					}
				}

				auto const output_signal = signals.back();
				xmg.create_po( normal ? output_signal : !output_signal );
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
}

int main()
{	
	xmg_db_gen();

	return 0;
}