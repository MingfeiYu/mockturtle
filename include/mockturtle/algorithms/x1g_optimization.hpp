#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <mockturtle/algorithms/dont_cares.hpp>
#include <mockturtle/networks/x1g.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <mockturtle/views/topo_view.hpp>

namespace mockturtle
{
/* if there are two adjacent OneHot gates, x1 and x2, meeting the conditions that */
/* (1) x1 = OneHot( 1, !a, !b ) and x2 = OneHot( 1, !c, !x1 );                    */
/* (2) x1's fanout size is 1,                                                     */
/* then, they can be replaced by x3 = AND( a, b, c )                              */
x1g_network x1g_merge_optimization( x1g_network const& x1g )
{
	topo_view<x1g_network> x1g_topo{ x1g };
	fanout_view<topo_view<x1g_network>> x1g_merge{ x1g_topo };
	node_map<x1g_network::signal, x1g_network> old2new( x1g_merge );
	x1g_network dest;

	/* generate constant */
	old2new[x1g_merge.get_constant( false )] = dest.get_constant( false );

	/* generate pis */
	x1g_merge.foreach_pi( [&]( auto const& n )
	{
		old2new[n] = dest.create_pi();
	} );

	/* generate gates */
	x1g_merge.clear_values();

	x1g_merge.foreach_gate( [&]( auto const& n ) {
		if ( x1g_merge.is_onehot( n ) && ( x1g_merge.value( n ) == 0u ) )
		{
			bool cond1{ false };
			bool cond2{ false };

			if ( x1g_merge.fanout( n ).size() == 1u )
			{
				cond2 = true;

				if ( cond2 )
				{
					x1g_merge.foreach_fanin( n, [&]( auto const& f ) {
						if ( f == x1g_merge.get_constant( true ) )
						{
							cond1 = true;
						}
					} );

					if ( cond1 )
					{
						auto const next = x1g_merge.fanout( n )[0];
						if ( x1g_merge.is_onehot( next ) )
						{
							bool has_const1_input{ false };
							bool x1_is_inverted{ false };

							x1g_merge.foreach_fanin( next, [&]( auto const& f ) {
								if ( f == x1g_merge.get_constant( true ) )
								{
									has_const1_input = true;
								}
								if ( ( x1g_merge.get_node( f ) == n ) && ( x1g_merge.is_complemented( f ) ) )
								{
									x1_is_inverted = true;
								}
							} );

							if ( has_const1_input && x1_is_inverted )
							{
								/* found a pair of OneHots that meets the conditions */
								x1g_merge.set_value( n, 1u );
								x1g_merge.set_value( next, 2u );
								//std::cout << "[m] node " << n << " and node " << next << " can be replaced by an AND3\n";
								//std::cout << "[m] node " << n << "'s input signals are: ";
								//x1g_merge.foreach_fanin( n, [&]( auto const& ni ) {
								//	std::cout << ( x1g_merge.is_complemented( ni ) ? "~" : "" ) << ni.index << " ";
								//} );
								//std::cout << "\n";
								//std::cout << "[m] node " << n << "'s pos are: ";
								//x1g_merge.foreach_fanout( n, [&]( auto const& no ) {
								//	std::cout << "node " << no << " ";
								//} );
								//std::cout << "\n";
								//std::cout << "[m] node " << next << "'s input signals are: ";
								//x1g_merge.foreach_fanin( next, [&]( auto const& ni ) {
								//	std::cout << ( x1g_merge.is_complemented( ni ) ? "~" : "" ) << ni.index << " ";
								//} );
								//std::cout << "\n";
								//std::cout << "[m] node " << next << "'s pos are: ";
								//x1g_merge.foreach_fanout( next, [&]( auto const& no ) {
								//	std::cout << "node " << no << " ";
								//} );
								//std::cout << "\n";
							}
						}
					}
				}
			}
		}
	} );

	x1g_merge.foreach_gate( [&]( auto const& n ) {
		std::vector<x1g_network::signal> children;
		if ( x1g_merge.is_onehot( n ) )
		{
			if ( x1g_merge.value( n ) == 0u )
			{
				/* untouched OneHots */
				x1g_topo.foreach_fanin( n, [&]( auto const& f ) {
					children.emplace_back( old2new[f] ^ x1g_merge.is_complemented( f ) );
				} );
				old2new[n] = dest.create_onehot( children[0], children[1], children[2] );
			}
			else if ( x1g_merge.value( n ) == 2u )
			{
				x1g_network::node previous;
				x1g_merge.foreach_fanin( n, [&]( auto const& f ) {
					if ( x1g_merge.value( x1g_merge.get_node( f ) ) == 1u )
					{
						previous = f.index;
					}
				} );
				//std::cout << "[m] input signals to the AND3 replace node " << previous << " and " << n << " are: ";
				x1g_merge.foreach_fanin( n, [&]( auto const& f ) {
					if ( ( f == x1g_merge.get_constant( true ) ) || ( x1g_merge.get_node( f ) == previous ) )
					{
						return;
					}
					//std::cout << ( !x1g_merge.is_complemented( f ) ? "~" : "" ) << f.index << " ";
					children.emplace_back( old2new[f] ^ ( !x1g_merge.is_complemented( f ) ) );
				} );

				x1g_merge.foreach_fanin( previous, [&]( auto const& f ) {
					if ( f == x1g_merge.get_constant( true ) )
					{
						return;
					}
					//std::cout << ( !x1g_merge.is_complemented( f ) ? "~" : "" ) << f.index << " ";
					children.emplace_back( old2new[f] ^ ( !x1g_merge.is_complemented( f ) ) );
				} );
				//std::cout << "\n";
				//std::cout << "[m] They are ";
				//for ( auto const& child: children )
				//{
				//	std::cout << ( dest.is_complemented( child ) ? "~" : "" ) << child.index << " ";
				//}
				//std::cout << "in the new X1G\n";

				assert( children.size() == 3 );
				old2new[n] = dest.create_and3( children[0], children[1], children[2] );
			}
		}
		else
		{
			x1g_merge.foreach_fanin( n, [&]( auto const& f ) {
				children.emplace_back( old2new[f] ^ x1g_merge.is_complemented( f ) );
			} );
			old2new[n] = dest.create_xor3( children[0], children[1], children[2] );
		}
	} );
	x1g_merge.clear_values();

	/* generate output */
	x1g_merge.foreach_po( [&]( auto const& f ) {
		dest.create_po( old2new[f] ^ x1g_merge.is_complemented( f ) );
	} );

	return dest;
}

/* if a OneHot is satisfiability don't care for assignment 111, it can be         */
/* replaced by a XOR3 */

x1g_network x1g_dont_cares_optimization( x1g_network const& x1g )
{

	node_map<x1g_network::signal, x1g_network> old2new( x1g );
	x1g_network dest;

	/* generate constant */
	old2new[x1g.get_constant( false )] = dest.get_constant( false );

	/* generate pi */
	x1g.foreach_pi( [&]( auto const& n ) {
		old2new[n] = dest.create_pi();
	} );

	/* generate gates */
	satisfiability_dont_cares_checker<x1g_network> checker( x1g );
	
	topo_view<x1g_network>{ x1g }.foreach_node( [&]( auto const& n ) {
		if ( x1g.is_constant( n ) || x1g.is_pi( n ) )
		{
			return;
		}

		std::vector<x1g_network::signal> children;
		x1g.foreach_fanin( n, [&]( auto const& f ) 
		{
			children.emplace_back( old2new[f] ^ x1g.is_complemented( f ) );
		} );

		if ( x1g.is_onehot( n ) )
		{
			if ( checker.is_dont_care( n, { true, true, true } ) )
			{
				old2new[n] = dest.create_xor3( children[0], children[1], children[2] );
			}
			else
			{
				old2new[n] = dest.create_onehot( children[0], children[1], children[2] );
			}
		}
		else
		{
			old2new[n] = dest.create_xor3( children[0], children[1], children[2] );
		}
	} );

	x1g.foreach_po( [&]( auto const& f ) {
		dest.create_po( old2new[f] ^ x1g.is_complemented( f ) );
	} );

	return dest;
}

} /* namespace mockturtle */