#include <iostream>

#include <mockturtle/networks/abstract_xag.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/utils/node_map.hpp>

namespace mockturtle
{

uint32_t garbling_cost_calculator( xag_network const& xag )
{
	/* Directly map an XAG to an abstract-XAG */
	abstract_xag_network xag_abs;

	using signal = abstract_xag_network::signal;
	node_map<signal, xag_network> node2new( xag );

	/* create consts */
	node2new[xag.get_node( xag.get_constant( false ) )] = xag_abs.get_constant( false );

	/* create pis */
	xag.foreach_pi( [&]( auto n ) {
		node2new[n] = xag_abs.create_pi();
	} );

	/* create ANDs and XORs */
	xag.foreach_node( [&]( auto n ) {
		if ( xag.is_constant( n ) || xag.is_pi( n ) )
		{
			return;
		}

		std::vector<signal> children;
		xag.foreach_fanin( n, [&]( auto const& f ) {
			children.emplace_back( xag.is_complemented( f ) ? xag_abs.create_not( node2new.[f] ) : node2new.at( f ) );
		} );
		assert( children.size() == 2u );

		if ( xag.is_and( n ) )
		{
			node2new[n] = xag_abs.create_and( children[0], children[1] );
		}
		else
		{
			node2new[n] = xag_abs.create_xor( children[0], children[1] );
		}
	} );

	/* create pos */
	xag.foreach_po( [&]( auto const& f, auto index ) {
		auto const po = xag.is_complemented( f ) ? xag_abs.create_not( node2new[f] ) : node2new[f];
		xag_abs.create_po( po );
	} );

	/* calculate the mc of the synthesized abstract-XAG */
	uint32_t num_and = 0u;
	xag_abs.foreach_gate( [&]( auto f ) {
		if ( xag_abs.is_and( f ) )
		{
			++num_and;
		}
	} );

	return num_and;
}

} /* namespace mockturtle */
