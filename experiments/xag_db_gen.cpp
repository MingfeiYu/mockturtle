#include <string>
#include <array>
#include <iostream>
#include <fstream>
#include <fmt/format.h>
#include <iomanip>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/utils/include/percy.hpp>
#include <mockturtle/views/topo_view.hpp>
#include <mockturtle/views/fanout_view.hpp>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/hash.hpp>
#include <kitty/spectral.hpp>
#include <kitty/constructors.hpp>
#include <kitty/properties.hpp>

#include <experiments.hpp>

using merge_view = mockturtle::fanout_view<mockturtle::topo_view<mockturtle::xag_network>>;

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

static const uint32_t mc_npn_4[] = {
	0u, 3u, 2u, 2u, 3u, 1u, 3u, 2u, 2u, 3u, 2u, 2u, 3u, 1u, 3u, 2u, 1u, 3u, 
	2u, 2u, 3u, 0u, 2u, 3u, 3u, 2u, 2u, 3u, 3u, 2u, 2u, 3u, 2u, 3u, 2u, 2u, 
	3u, 2u, 3u, 3u, 2u, 2u, 3u, 3u, 2u, 2u, 3u, 3u, 2u, 2u, 3u, 2u, 3u, 2u, 
	3u, 2u, 2u, 3u, 3u, 2u, 2u, 3u, 2u, 3u, 3u, 2u, 3u, 2u, 2u, 3u, 2u, 3u, 
	3u, 2u, 3u, 2u, 2u, 3u, 3u, 2u, 2u, 2u, 3u, 1u, 2u, 3u, 3u, 2u, 2u, 3u, 
	3u, 2u, 3u, 2u, 2u, 3u, 2u, 3u, 3u, 2u, 3u, 2u, 2u, 1u, 3u, 2u, 2u, 2u, 
	3u, 1u, 2u, 3u, 3u, 2u, 2u, 3u, 2u, 3u, 2u, 2u, 1u, 1u, 3u, 3u, 2u, 2u, 
	3u, 2u, 3u, 1u, 2u, 3u, 3u, 2u, 3u, 3u, 2u, 2u, 1u, 3u, 2u, 2u, 3u, 1u, 
	3u, 2u, 2u, 3u, 2u, 3u, 3u, 2u, 3u, 2u, 2u, 3u, 3u, 1u, 1u, 2u, 3u, 2u, 
	2u, 3u, 2u, 3u, 3u, 2u, 2u, 2u, 2u, 3u, 3u, 2u, 2u, 2u, 3u, 2u, 2u, 2u, 
	0u, 2u, 3u, 3u, 2u, 2u, 3u, 3u, 2u, 2u, 3u, 2u, 3u, 3u, 3u, 2u, 2u, 3u, 
	3u, 2u, 2u, 3u, 3u, 2u, 2u, 2u, 2u, 1u, 2u, 3u, 2u, 2u, 1u, 1u, 3u, 2u, 
	2u, 1u, 1u, 1u, 0u, 0u};

std::vector<uint32_t> npn_4_mc()
{
	std::vector<uint32_t> best_score;
	for ( auto i = 0u; i < 221u; ++i )
	{
		best_score.emplace_back( mc_npn_4[i] );
	}
	return best_score;
}

void look_up_mc( kitty::dynamic_truth_table const& tt, uint32_t & mc, bool & valid_mc )
{
	/* Solution 2: forget about database */
	const auto tt_lookup = tt.num_vars() < 5u ? kitty::extend_to( tt, 5u ) : tt;
	mc = kitty::get_spectral_mc( tt_lookup );
	valid_mc = true;
}

void count_and_size_rec( merge_view& xag, mockturtle::xag_network::node const& f, mockturtle::xag_network::node const& root )
{
	if ( xag.value( f ) == 0u ) 
	{
		if ( f != root )
		{
			//std::cout << "with fanout: "; 
			//for ( auto const fanout_each: xag.fanout( f ) )
			//{
			//	std::cout << fanout_each << ", ";
			//}
			//std::cout << "(" << xag.fanout( f ).size() << ")\n";

			if ( xag.fanout( f ).size() == 1u )
			{
				//std::cout << "AND clique larger than 2 detected! \n";
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
			//std::cout << "Root node " << f << ":\n";
			xag.set_value( f, 2u );
			/* recursively trace its fanin */
			xag.foreach_fanin( f, [&]( auto const& fi ) {
				auto const child = xag.get_node( fi );
				if ( xag.is_and( child ) && !xag.is_complemented( fi ) )
				{
					//std::cout << "child node " << child << " is an AND, ";
					count_and_size_rec( xag, child, f );
				}
			} );
		}
	}
}

void xag_db_gen()
{
	auto const benchmarks = npn_4();
	auto const best_scores = npn_4_mc();
	std::vector<uint32_t> npn_mc( 7 );
	std::vector<std::string> failed;
	float time_accum = 0.0;

	for ( auto i = 0u; i < benchmarks.size(); ++i )
	{
		auto const benchmark = benchmarks[i];
		auto const best_score = best_scores[i];

		std::cout << "[i] processing " << benchmark << std::endl;

		kitty::dynamic_truth_table function( 4u );
		kitty::create_from_hex_string( function, benchmark );

		assert( function.num_vars() == 4u );
		bool const normal = kitty::is_normal( function );

		percy::spec_minmc spec;
		spec.nr_in = function.num_vars();
		spec.fanin_size = 2u;
		spec.verbosity = 0u;
		spec.set_output( normal ? function : ~function );

		if ( int triv = spec.check_triv(); triv >= 0 )
		{
			++npn_mc[0];
			continue;
		}

		mockturtle::xag_network xag;
		xag.get_constant( false );
		xag.create_pi();
		xag.create_pi();
		xag.create_pi();
		xag.create_pi();

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

		percy::chain_minmc chain;
		uint32_t mc = best_score;
		//( void ) best_score;
		//uint32_t mc = 0u;
		//bool valid_mc{ false };
		//look_up_mc( function, mc, valid_mc );
		//assert( valid_mc );

		spec.set_nfree( mc );
		percy::synth_stats synth_st;
		const auto result = percy::std_synthesize_minmc( spec, chain, &synth_st );
		assert( result == percy::success );
		if ( result != percy::success )
		{
			failed.emplace_back( benchmark );
			continue;
		}

		std::vector<mockturtle::xag_network::signal> signals;
		signals.emplace_back( xag.make_signal( xag.pi_at( 0 ) ) );
		signals.emplace_back( xag.make_signal( xag.pi_at( 1 ) ) );
		signals.emplace_back( xag.make_signal( xag.pi_at( 2 ) ) );
		signals.emplace_back( xag.make_signal( xag.pi_at( 3 ) ) );

		for ( uint32_t i = 0u; i < static_cast<uint32_t> ( chain.get_nr_steps() ); ++i )
		{
			auto const c1 = signals[chain.get_step( i )[0]];
			auto const c2 = signals[chain.get_step( i )[1]];

			switch ( chain.get_operator( i )._bits[0] )
			{
			case 0x0:
				signals.emplace_back( xag.get_constant( false ) );
				break;
			case 0xa:
				signals.emplace_back( c1 );
				break;
			case 0xc:
				signals.emplace_back( c2 );
				break;
			case 0x8:
				signals.emplace_back(  xag.create_and(  c1,  c2 ) );
				break;
			case 0x4:
				signals.emplace_back(  xag.create_and( !c1,  c2 ) );
				break;
			case 0x2:
				signals.emplace_back(  xag.create_and(  c1, !c2 ) );
				break;
			case 0xe:
				signals.emplace_back( !xag.create_and( !c1, !c2 ) );
				break;
			case 0x6:
				signals.emplace_back(  xag.create_xor(  c1,  c2 ) );
				break;
			default:
				std::cerr << "[e] unsupported operation " << kitty::to_hex( chain.get_operator( i ) ) << std::endl;
				assert( false );
				break;
			}
		}
		auto const output_signal = signals.back();
		xag.create_po( normal ? output_signal : !output_signal );

		uint32_t num_and_aft = 0u;
		uint32_t garble_cost_aft = 0u;

		mockturtle::topo_view<mockturtle::xag_network> xag_topo{xag};
		mockturtle::fanout_view<mockturtle::topo_view<mockturtle::xag_network>> xag_merge{xag_topo};
		xag_merge.clear_values();
		const clock_t begin_time = clock();

		xag_merge.foreach_gate_reverse( [&]( auto const& f ) {
			if ( xag_merge.is_and( f ) && xag_merge.value( f ) == 0u )
			{
				// this is the root node of a potentially larger AND
				++num_and_aft;
				count_and_size_rec( xag_merge, f, f );
				garble_cost_aft += xag_merge.value( f );
			}
		} );
		time_accum += ( float( clock() - begin_time ) / CLOCKS_PER_SEC );

		assert( garble_cost_aft <= 6u );

		++npn_mc[garble_cost_aft];
	}

	for ( auto i = 0u; i < npn_mc.size(); ++i )
	{
		std::cout << "npn_mc[" << i << "] is " << npn_mc[i] << "\n";
	}
	std::cout << "Failed cases are: ";
	for ( auto failed_case: failed )
	{
		std::cout << failed_case << ", ";
	}
	std::cout << "Spent " << time_accum / benchmarks.size() << "s in average\n";
	std::cout << std::endl;
}

int main()
{	
	xag_db_gen();

	return 0;
}