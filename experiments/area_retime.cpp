#include <fmt/format.h>
#include <mockturtle/algorithms/area_retime.hpp>
#include <mockturtle/networks/generic.hpp>
#include <mockturtle/utils/cost_functions.hpp>
#include <mockturtle/views/fanout_view.hpp>

mockturtle::generic_network::signal create_register( mockturtle::generic_network& ntk, mockturtle::generic_network::signal f )
{
	const mockturtle::generic_network::signal in_register = ntk.create_box_input( f );
	const mockturtle::generic_network::signal node_register = ntk.create_register( in_register );
	return ntk.create_box_output( node_register );
}

template<class Ntk>
struct not_cost
{
  uint32_t operator()( Ntk const& ntk, mockturtle::node<Ntk> const& n ) const
  {
    if ( ntk.is_not( n ) )
    {
    	return 1u;
    }

    return 0u;
  }
};

template<class Ntk>
struct mult_cost
{
  uint32_t operator()( Ntk const& ntk, mockturtle::node<Ntk> const& n ) const
  {
    if ( ntk.is_and( n ) )
    {
    	return 1u;
    }

    return 0u;
  }
};

int main( void )
{
	using namespace mockturtle;

	generic_network ntk;
	{
		//              -----------------------------------
		//             |                                   |
		// pi -> n1 -> n2 -> n3 -||-> n7 -> n8 -> n9 -||-> n13 -||-> n17 -> n18 -> n19 -> po
		generic_network::signal op = ntk.create_pi();
		generic_network::signal n2 = ntk.get_constant( false );
		for ( auto i{ 1u }; i <= 3u; ++i )
		{
			op = ntk.create_not( op );
			if ( i == 2 )
			{
				n2 = op;
			}
		}
		op = create_register( ntk, op );

		for ( auto i{ 4u }; i <= 6u; ++i )
		{
			op = ntk.create_not( op );
		}
		op = create_register( ntk, op );

		op = ntk.create_and( op, n2 );
		op = create_register( ntk, op );

		for ( auto i{ 8u }; i <= 10u; ++i )
		{
			op = ntk.create_not( op );
		}

		ntk.create_po( op );
	}

	// {
	// 	//                                  ----- n15 -||-> n19 -> n20 ->po2 
	// 	//                                  |     
	// 	// pi -> n1 -> n2 -> n3 -||-> n7 -> n8 -> n9 -||-> n13 -> n14 -> po1
	// 	generic_network::signal op = ntk.create_pi();
	// 	for ( auto i{ 1u }; i <= 3u; ++i )
	// 	{
	// 		op = ntk.create_not( op );
	// 	}
	// 	op = create_register( ntk, op );

	// 	generic_network::signal n8 = ntk.get_constant( false );
	// 	for ( auto i{ 7u }; i <= 9u; ++i )
	// 	{
	// 		op = ntk.create_not( op );
	// 		if ( i == 8 )
	// 		{
	// 			n8 = op;
	// 		}
	// 	}
	// 	op = create_register( ntk, op );

	// 	op = ntk.create_not( op );
	// 	generic_network::signal n14 = ntk.create_not( op );

	// 	op = n8;
	// 	op = ntk.create_not( op );
	// 	op = create_register( ntk, op );

	// 	for ( auto i{ 19u }; i <= 20u; ++i )
	// 	{
	// 		op = ntk.create_not( op );
	// 	}

	// 	ntk.create_po( n14 );
	// 	ntk.create_po( op );
	// }

	area_retime_params ps{};
	ps.is_delay_constrained = true;
	ps.forward_only = false;
	ps.backward_only = true;
	ps.iterations = 2u;
	ps.delay_cons = 3u;
	ps.verbose = true;

	area_retime( ntk, not_cost<fanout_view<generic_network>>(), ps );

	return 0;
}