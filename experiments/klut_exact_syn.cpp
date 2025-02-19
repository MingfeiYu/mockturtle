#include <kitty/bit_operations.hpp>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operators.hpp>

#include <percy/percy.hpp>

#include <bitset>

int main( void )
{
	kitty::dynamic_truth_table a{ 3u };
	kitty::dynamic_truth_table b{ 3u };
	kitty::dynamic_truth_table c{ 3u };
	kitty::dynamic_truth_table const0{ 3u };
	kitty::create_nth_var( a, 0 );
	kitty::create_nth_var( b, 1 );
	kitty::create_nth_var( c, 2 );

	percy::spec s;
	s.nr_in = 3u;
	s.fanin = 3u;
	s.verbosity = 0u;
	s.add_primitive( a );
	s.add_primitive( b );
	s.add_primitive( c );
	s.add_primitive( const0 );

	// kitty::dynamic_truth_table d{ 3u };
	// kitty::create_from_binary_string( d, "00000000" );
	// s.add_primitive( d );
	// kitty::create_from_binary_string( d, "00010001" );
	s.add_primitive( ~a & ~b );
	// kitty::create_from_binary_string( d, "00100010" );
	s.add_primitive( a & ~b );
	// kitty::create_from_binary_string( d, "00110011" );
	// s.add_primitive( d );
	// kitty::create_from_binary_string( d, "01000100" );
	s.add_primitive( ~a & b );
	// kitty::create_from_binary_string( d, "01010101" );
	// s.add_primitive( d );
	// kitty::create_from_binary_string( d, "01100110" );
	s.add_primitive( a ^ b );
	// kitty::create_from_binary_string( d, "01110111" );
	s.add_primitive( ~( a & b ) );
	// kitty::create_from_binary_string( d, "10001000" );
	s.add_primitive( a & b );
	// kitty::create_from_binary_string( d, "10011001" );
	s.add_primitive( ~( a ^ b ) );
	// kitty::create_from_binary_string( d, "10101010" );
	// s.add_primitive( d );
	// kitty::create_from_binary_string( d, "10111011" );
	s.add_primitive( a | ~b );
	// kitty::create_from_binary_string( d, "11001100" );
	// s.add_primitive( d );
	// kitty::create_from_binary_string( d, "11011101" );
	s.add_primitive( ~a | b );
	// kitty::create_from_binary_string( d, "11101110" );
	s.add_primitive( a | b );
	// kitty::create_from_binary_string( d, "11111111" );
	// s.add_primitive( d );

	auto maj = kitty::ternary_majority( a, b, c );
	for ( uint32_t i{ 3u }; i < 4u; ++i )
	{
		auto maj_tune{ maj };
		flip_bit( maj_tune, i );
		std::cout << "[i] Target function is : ";
		kitty::print_binary( maj_tune );
		std::cout << std::endl;

		percy::spec s_tune{ s };
		s_tune[0] = maj_tune;
		percy::chain c;
		const auto res = percy::synthesize( s_tune, c );
		if ( res == percy::success )
		{
			std::cout << "[i] Use " << ( c.get_nr_steps() ) << " LUTs to synthesize the target function.\n";
			for ( uint32_t j{ 0u }; j < static_cast<uint32_t>( c.get_nr_steps() ); ++j )
			{
				std::cout << "[m] LUT " << (j + 1) << ": \t";
				std::bitset<8> func_bit( c.get_operator( j )._bits[0] );
				std::cout << "Function: " << func_bit << "; \t";
				std::cout << "Ops: " << "signal " << c.get_step( j )[0] << " and signal " << c.get_step( j )[1];
				std::cout << std::endl;
			}
		}
		else
		{
			std::cout << "[e] Failed somehow...\n";
		}
		std::cout << std::endl;
	}

	return 0;
}