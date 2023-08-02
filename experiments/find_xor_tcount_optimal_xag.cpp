#include <cstdint>
#include <fstream>

#include <mockturtle/networks/xag.hpp>
#include <mockturtle/algorithms/low_tcount_exact_synthesis.hpp>
#include <kitty/constructors.hpp>

typedef struct winning_AND_fence
{
	std::string func;
	std::vector<uint32_t> num_ands;
} winning_AND_fence;

static const std::vector<winning_AND_fence> winning_and_fences = {
	{ "aa2a2a80", { 1u, 1u, 2u } }, 
	{ "88080808", { 2u, 2u } }, 
	{ "bd686868", { 1u, 1u, 2u } }, 
	{ "aa808080", { 2u, 2u } }, 
	{ "7e686868", { 1u, 1u, 2u } }, 
	{ "2208a208", { 1u, 1u, 2u } }, 
	{ "08888888", { 2u, 2u } }, 
	{ "aae6da80", { 1u, 1u, 2u } }, 
	{ "58d87888", { 1u, 1u, 2u } }, 
	{ "8c88ac28", { 1u, 1u, 2u } }, 
	{ "8880f880", { 1u, 1u, 2u } }, 
	{ "9ee8e888", { 1u, 3u } }, 
	{ "4268c268", { 1u, 1u, 2u } }, 
	{ "16704c80", { 1u, 1u, 2u } }, 
	{ "372840a0", { 1u, 1u, 2u } }, 
	{ "7ca00428", { 1u, 1u, 2u } }, 
	{ "f8880888", { 1u, 2u } }, 
	{ "2ec0ae40", { 1u, 1u, 2u } }, 
	{ "f888f888", { 1u, 2u } }, 
	{ "567cea40", { 1u, 1u, 2u } }, 
	{ "6248eac0", { 1u, 1u, 2u } } };

void find_xor_tcount_optimal_xag()
{
	std::ifstream db;
	db.open( "db_tcount_5_mc", std::ios::in );
	std::string line;
	uint32_t pos{ 0u };

	while ( std::getline( db, line ) )
	{
		std::string line_dup{ line };

		pos = static_cast<uint32_t>( line.find( 'x' ) );
		const std::string repr_real_str = line.substr( ++pos, 8u );
		pos += 9u;
		line.erase( 0, pos );
		pos = line.find( 'x' );
		std::string repr_db_str = line.substr( ++pos, 8u );
		pos += 9u;
		line.erase( 0, pos );

		std::cout << "[i] processing " << repr_db_str << "\n";
		bool done{ false };

		for ( auto const& winning_and_fences_each: winning_and_fences )
		{
			if ( done )
			{
				break;
			}

			if ( repr_db_str == winning_and_fences_each.func )
			{
				done = true;
				std::cout << "[i] looking for xor-optimal XAG implementation of " << repr_db_str << "\n";

				pos = line.find( ' ' );
				const uint32_t num_var = std::stoul( line.substr( 0, pos++ ) );
				line.erase( 0, pos );
				pos = line.find( ' ' );
				const uint32_t mc = std::stoul( line.substr( 0, pos++ ) );
				line.erase( 0, pos );

				mockturtle::xag_network xag_tcount_opt;
				std::vector<mockturtle::xag_network::signal> signals_tcount_opt( num_var );
				std::generate( signals_tcount_opt.begin(), signals_tcount_opt.end(), [&]() { return xag_tcount_opt.create_pi(); } );

				while ( line.size() > 3 )
				{
					uint32_t signal_1, signal_2;
					pos = line.find( ' ' );
					signal_1 = std::stoul( line.substr( 0, pos++ ) );
					line.erase( 0, pos );
					pos = line.find( ' ' );
					signal_2 = std::stoul( line.substr( 0, pos++ ) );
					line.erase( 0, pos );

					mockturtle::xag_network::signal signal1, signal2;
		    	if ( signal_1 == 0u )
		    	{
						signal1 = xag_tcount_opt.get_constant( false );
		    	}
		    	else if ( signal_1 == 1u )
		    	{
						signal1 = xag_tcount_opt.get_constant( true );
		    	}
		    	else
		    	{
		    		signal1 = signals_tcount_opt[signal_1 / 2 - 1] ^ ( signal_1 % 2 != 0 );
		    	}
		    	if ( signal_2 == 0u )
		    	{
						signal2 = xag_tcount_opt.get_constant( false );
		    	}
		    	else if ( signal_2 == 1u )
		    	{
						signal2 = xag_tcount_opt.get_constant( true );
		    	}
		    	else
		    	{
		    		signal2 = signals_tcount_opt[signal_2 / 2 - 1] ^ ( signal_2 % 2 != 0 );
		    	}

		    	if ( signal_1 > signal_2 )
		    	{
		    		signals_tcount_opt.emplace_back( xag_tcount_opt.create_xor( signal1, signal2 ) );
		    	}
		    	else
		    	{
		    		signals_tcount_opt.emplace_back( xag_tcount_opt.create_and( signal1, signal2 ) );
		    	}
				}

				const uint32_t signal_po = std::stoul( line );
				const mockturtle::xag_network::signal po = signals_tcount_opt[signal_po / 2 - 1] ^ ( signal_po % 2 != 0 );
				xag_tcount_opt.create_po( po );

				kitty::dynamic_truth_table tt_min_base( num_var );
				kitty::create_from_hex_string( tt_min_base, winning_and_fences_each.func.substr( 0, 1 << ( num_var - 2 ) ) );

				uint32_t num_xors_base{ 0u };
				xag_tcount_opt.foreach_gate( [&]( auto const& n ) {
					if ( xag_tcount_opt.is_xor( n ) )
					{
						++num_xors_base;
					}
				} );

				mockturtle::low_tcount_exact_synthesis_params ps;
				ps.expected_num_xor = std::make_optional<uint32_t>( num_xors_base );
				ps.verbose = true;

				auto const p_xag_xor_opt = mockturtle::low_tcount_exact_synthesis( tt_min_base, winning_and_fences_each.num_ands, ps, nullptr );
				if ( p_xag_xor_opt )
				{
					line_dup  = ( "0x" + repr_real_str + " " );
			    line_dup += ( "0x" + repr_db_str + " " );
			    line_dup += ( std::to_string( num_var ) + " " );
			    line_dup += ( std::to_string( mc ) + " " );
			    ( *p_xag_xor_opt ).foreach_gate( [&]( auto const& n ) {
			    	( *p_xag_xor_opt ).foreach_fanin( n, [&]( auto const& ni ) {
			    		line_dup += ( std::to_string( static_cast<uint32_t>( ( ni.index << 1 )  + ni.complement ) ) + " " );
			    	} );
			    } );
			    mockturtle::xag_network::signal po = ( *p_xag_xor_opt ).po_at( 0 );
			    line_dup += ( std::to_string( static_cast<uint32_t>( ( po.index << 1 ) + po.complement ) ) + "\n" );
				}
				else
				{
					std::cout << "[e] didn't find better XAG implementation for " << repr_db_str << " (" << num_xors_base << ")\n";
					//abort();
				}
			}
		}

		std::ofstream db_tcount_xor_opt;
		db_tcount_xor_opt.open( "db_tcount_5_mc_xor_opt", std::ios::app );
		db_tcount_xor_opt << line_dup;
		db_tcount_xor_opt.close();
	}
}

int main()
{
	find_xor_tcount_optimal_xag();

	return 0;
}