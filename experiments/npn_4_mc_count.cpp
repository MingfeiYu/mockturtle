#include <string>
#include <array>
#include <iostream>
#include <fstream>
#include <fmt/format.h>
#include <iomanip>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/hash.hpp>
#include <kitty/spectral.hpp>
#include <kitty/constructors.hpp>

#include <experiments.hpp>

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
	/* Solution 2: forget about database */
	const auto tt_lookup = tt.num_vars() < 5u ? kitty::extend_to( tt, 5u ) : tt;
	mc = kitty::get_spectral_mc( tt_lookup );
	valid_mc = true;
}

void npn_4_mc_count()
{
	auto const benchmarks = npn_4();

	std::ofstream fout( "npn_4_mc_count.txt" );

	for ( auto const& benchmark: benchmarks )
	{
		std::cout << "[i] processing " << benchmark << std::endl;

		kitty::dynamic_truth_table function( 4u );
		kitty::create_from_hex_string( function, benchmark );

		uint32_t mc{ 0u };

		bool valid_mc{ false };
		look_up_mc( function, mc, valid_mc );
		assert( valid_mc );

		fout << mc << "u, ";
	}
	fout << "\n};";
	fout.close();
}

int main()
{	
	//npn_4_mc_count();
	uint32_t mc{ 0u };
	bool validity{ false };
	kitty::dynamic_truth_table function( 3u );
	std::string function_str = "68";
	kitty::create_from_hex_string( function, function_str );
	look_up_mc( function, mc, validity );
	std::cout << "Functional MC of function 0x" << function_str << " is : "; 
	std::cout << ( validity ? std::to_string( mc ) : "N/A" ) << std::endl;

	return 0;
}