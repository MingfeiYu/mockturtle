#include <cstdint>
#include <fstream>
#include <unordered_map>

#include <kitty/constructors.hpp>
#include <kitty/operations.hpp>
#include <kitty/spectral.hpp>
#include <mockturtle/algorithms/optimum_gc_synthesis.hpp>
#include <mockturtle/networks/xag.hpp>

typedef struct AND_topo
{
	std::vector<uint32_t> num_ands;
	uint32_t mc;
	uint32_t gc;
} AND_topo;

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
	{ { 1u, 4u }, 5u, 7u }, 
	{ { 2u, 3u }, 5u, 7u }, 
	{ { 3u, 2u }, 5u, 7u }, 
	{ { 4u, 1u }, 5u, 7u }, 
	{ { 1u, 1u, 1u, 1u }, 4u, 8u }, 
	{ { 1u, 1u, 3u }, 5u, 8u }, 
	{ { 1u, 2u, 2u }, 5u, 8u }, 
	{ { 1u, 3u, 1u }, 5u, 8u }, 
	{ { 2u, 1u, 2u }, 5u, 8u }, 
	{ { 2u, 2u, 1u }, 5u, 8u }, 
	{ { 3u, 1u, 1u }, 5u, 8u }, 
	{ { 1u, 1u, 1u, 2u }, 5u, 9u }, 
	{ { 1u, 1u, 2u, 1u }, 5u, 9u }, 
	{ { 1u, 2u, 1u, 1u }, 5u, 9u }, 
	{ { 2u, 1u, 1u, 1u }, 5u, 9u }, 
	{ { 1u, 1u, 1u, 1u, 1u }, 5u, 10u }, 
	{ { 6u }, 6u, 7u }, 
	{ { 1u, 5u }, 6u, 8u }, 
	{ { 2u, 4u }, 6u, 8u }, 
	{ { 3u, 3u }, 6u, 8u }, 
	{ { 4u, 2u }, 6u, 8u }, 
	{ { 5u, 1u }, 6u, 8u }, 
	{ { 1u, 1u, 4u }, 6u, 9u }, 
	{ { 1u, 2u, 3u }, 6u, 9u }, 
	{ { 1u, 3u, 2u }, 6u, 9u }, 
	{ { 1u, 4u, 1u }, 6u, 9u }, 
	{ { 2u, 1u, 3u }, 6u, 9u }, 
	{ { 2u, 2u, 2u }, 6u, 9u }, 
	{ { 2u, 3u, 1u }, 6u, 9u }, 
	{ { 3u, 1u, 2u }, 6u, 9u }, 
	{ { 3u, 2u, 1u }, 6u, 9u }, 
	{ { 4u, 1u, 1u }, 6u, 9u }, 
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

std::vector<std::string> npn_4()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 222u; ++i )
	{
		result.emplace_back( npn_4_input[i] );
	}

	return result;
}

std::vector<uint32_t> npn_4_mc()
{
	std::vector<uint32_t> result;
	for ( auto i = 0u; i < 222u; ++i )
	{
		result.emplace_back( mc_npn_4[i] );
	}

	return result;
}

void build_db( std::string const& dbname, std::unordered_map<std::string, uint32_t>& db )
{
	std::ifstream db_file;
	db_file.open( dbname, std::ios::in );
	std::string line;
	uint32_t pos{ 0u };
	uint32_t cnt{ 0u };

	db.insert( std::make_pair( "0000", 0u ) );
	std::cout << "[i] " << ++cnt << " entry in db\n";

	while ( std::getline( db_file, line ) )
	{
		pos = static_cast<uint32_t>( line.find( 'x' ) );
		const std::string repr_str = line.substr( ++pos, 4u );
		pos += 9u;
		line.erase( 0, pos );
		pos = line.find( 'x' );
		pos += 10u;
		line.erase( 0, pos );
		pos = line.find( ' ' );
		const uint32_t num_vars = std::stoul( line.substr( 0, pos++ ) );
		line.erase( 0, pos );
		if ( num_vars > 4u )
		{
			continue;
		}
		pos = line.find( ' ' );
		const uint32_t gc = std::stoul( line.substr( 0, pos++ ) );
		line.erase( 0, pos );
		db.insert( std::make_pair( repr_str, gc ) );
		std::cout << "[i] " << ++cnt << " entries in db\n";
	}
}

int main()
{
	uint32_t ae_gc{ 0u };
	uint32_t min_gc{ 0u };
	uint32_t num_cases_impr{ 0u };
	std::vector<uint32_t> ae_gcs;
	std::vector<uint32_t> min_gcs;
	std::vector<uint32_t> mcs_old;
	std::vector<uint32_t> mcs_new;

	std::unordered_map<std::string, uint32_t> db;
	build_db( "db_gc_complete_5", db );

	auto const benchmarks = npn_4();
	auto const npn_4_mcs = npn_4_mc();
	for ( auto i{ 0u }; i < benchmarks.size(); ++i )
	{
		kitty::static_truth_table<4u> tt_static;
    kitty::create_from_hex_string( tt_static, benchmarks[i] );
    auto repr = kitty::exact_spectral_canonization( tt_static );
    std::string repr_str = kitty::to_hex( repr );

    /* get gc of affine equivalence-based implementation of this function */
    auto const search = db.find( repr_str );
    uint32_t gc{ 0u };
    if ( search != db.end() )
    {
    	gc = search->second;
    }
    else
    {
    	std::cerr << "[e] representative (" << repr_str << ") of function (" 
    						<< benchmarks[i] << ") does not exist in database" << std::endl;
    	abort();
    }

    if ( gc == 0u )
    {
    	ae_gcs.emplace_back( 0u );
    	min_gcs.emplace_back( 0u );
    	mcs_old.emplace_back( 0u );
    	mcs_new.emplace_back( 0u );
    	continue;
    }
    ae_gcs.emplace_back( gc );
    ae_gc += gc;

    kitty::dynamic_truth_table tt( 4u );
    kitty::create_from_hex_string( tt, benchmarks[i] );
    uint32_t mc = npn_4_mcs[i];
    mcs_old.emplace_back( mc );

    /* figure out min gc of any implementation of this function by exact synthesis */
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

    	mockturtle::optimum_gc_synthesis_params ps;
      //ps.verbose = true;
      ps.verify_solution = true;

      auto const p_xag_gc_opt = mockturtle::optimum_gc_synthesis( tt, topo.num_ands, ps, nullptr );
      if ( p_xag_gc_opt )
      {
      	++num_cases_impr;
      	mc = topo.mc;
      	gc = topo.gc;
      	break;
      }
    }

    mcs_new.emplace_back( mc );
    min_gcs.emplace_back( gc );
    min_gc += gc;
	}

	std::cout << "[i] find better implementations for " << num_cases_impr << " functions\n";
	std::cout << "[i] sum of GC of AE-based implementations: " << ae_gc << ", "
						<< " sum of GC of optimal implementations: " << min_gc << std::endl;
	std::cout << "[i] GC of AE-based implementation of each function: ";
	for ( auto ae_gcs_each: ae_gcs )
	{
		std::cout << ae_gcs_each << " ";
	}
	std::cout << "\n";
	std::cout << "[i] GC of optimal implementation of each function: ";
	for ( auto min_gcs_each: min_gcs )
	{
		std::cout << min_gcs_each << " ";
	}
	std::cout << "\n";
	std::cout << "[i] FMC of each function: ";
	for ( auto mcs_old_each: mcs_old )
	{
		std::cout << mcs_old_each << " ";
	}
	std::cout << "\n";
	std::cout << "[i] SMC of optimal implementation of each function: ";
	for ( auto mcs_new_each: mcs_new )
	{
		std::cout << mcs_new_each << " ";
	}
	std::cout << "\n";

	for ( auto i{ 0u }; i < benchmarks.size(); ++i )
	{
		if ( mcs_old[i] < mcs_new[i] )
		{
			std::cout << "For function " << benchmarks[i] << ": "
								<< "its FMC is " << mcs_old[i] << ", "
								<< "but SMC of its optimal implementation is " << mcs_new[i] << "\n";
		}
	}

	return 0;
}