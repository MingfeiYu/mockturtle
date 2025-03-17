#include <fmt/format.h>
#include <mockturtle/algorithms/retiming.hpp>
#include <mockturtle/networks/generic.hpp>

mockturtle::generic_network::signal create_register( mockturtle::generic_network& ntk, mockturtle::generic_network::signal f )
{
	const mockturtle::generic_network::signal in_register = ntk.create_box_input( f );
	const mockturtle::generic_network::signal node_register = ntk.create_register( in_register );
	return ntk.create_box_output( node_register );
}

int main( void )
{
	using namespace mockturtle;

	generic_network ntk;
	// {
	// 	//              -----------------------------------
	// 	//             |                                   |
	// 	// pi -> n1 -> n2 -> n3 -||-> n4 -> n5 -> n6 -||-> n7 -||-> n8 -> n9 -> n10 -> po
	// 	generic_network::signal op = ntk.create_pi();
	// 	generic_network::signal n2 = ntk.get_constant( false );
	// 	for ( auto i{ 1u }; i <= 3u; ++i )
	// 	{
	// 		op = ntk.create_buf( op );
	// 		if ( i == 2 )
	// 		{
	// 			n2 = op;
	// 		}
	// 	}
	// 	op = create_register( ntk, op );

	// 	for ( auto i{ 4u }; i <= 6u; ++i )
	// 	{
	// 		op = ntk.create_buf( op );
	// 	}
	// 	op = create_register( ntk, op );

	// 	op = ntk.create_and( op, n2 );
	// 	op = create_register( ntk, op );

	// 	for ( auto i{ 8u }; i <= 10u; ++i )
	// 	{
	// 		op = ntk.create_buf( op );
	// 	}

	// 	ntk.create_po( op );
	// }

	{
		//                                  ----- n15 -||-> n19 -> n20 ->po2 
		//                                  |     
		// pi -> n1 -> n2 -> n3 -||-> n7 -> n8 -> n9 -||-> n13 -> n14 -> po1
		generic_network::signal op = ntk.create_pi();
		for ( auto i{ 1u }; i <= 3u; ++i )
		{
			op = ntk.create_buf( op );
		}
		op = create_register( ntk, op );

		generic_network::signal n8 = ntk.get_constant( false );
		for ( auto i{ 7u }; i <= 9u; ++i )
		{
			op = ntk.create_buf( op );
			if ( i == 8 )
			{
				n8 = op;
			}
		}
		op = create_register( ntk, op );

		op = ntk.create_buf( op );
		generic_network::signal n14 = ntk.create_buf( op );

		op = n8;
		op = ntk.create_buf( op );
		op = create_register( ntk, op );

		for ( auto i{ 19u }; i <= 20u; ++i )
		{
			op = ntk.create_buf( op );
		}

		ntk.create_po( n14 );
		ntk.create_po( op );
	}

	retime_params ps{};
	ps.forward_only = false;
	ps.backward_only = true;
	ps.iterations = UINT32_MAX;
	ps.verbose = true;

	retime( ntk, ps );

	return 0;
}