#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <mockturtle/networks/x1g.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <mockturtle/views/topo_view.hpp>

namespace mockturtle
{

namespace detail
{

void detect_and_group_rec( fanout_view<topo_view<xag_network>>& xag, 
													 fanout_view<topo_view<xag_network>>::node const& n, 
													 fanout_view<topo_view<xag_network>>::node const& root, 
													 std::vector<fanout_view<topo_view<xag_network>>::signal>& leaves )
{
	if ( xag.value( n ) == 0u ) 
	{
		if ( n != root )
		{
			/* remark this node as traversed */
			xag.incr_value( n );
			/* update the size of this AND group */
			xag.incr_value( root );
		}
		else
		{
			xag.set_value( n, 2u );
		}

		/* recursively trace its fanin */
		xag.foreach_fanin( n, [&]( auto const& f ) {
			auto const child = xag.get_node( f );
			if ( xag.is_and( child ) && !xag.is_complemented( f ) && ( xag.fanout( child ).size() == 1u ) )
			{
				detect_and_group_rec( xag, child, root, leaves );
			}
			else
			{
				/* find a leave of a group */
				leaves.emplace_back( f );
			}
		} );
	}
}

void detect_and_group( fanout_view<topo_view<xag_network>>& xag, 
											 std::unordered_map<fanout_view<topo_view<xag_network>>::node, 
											 										std::vector<fanout_view<topo_view<xag_network>>::signal>>& partition )
{
	xag.clear_values();

	xag.foreach_gate_reverse( [&]( auto const& n ) {
		if ( xag.is_and( n ) && xag.value( n ) == 0u )
		{
			std::vector<fanout_view<topo_view<xag_network>>::signal> leaves;
			detect_and_group_rec( xag, n, n, leaves );
			partition.insert( std::make_pair( n, leaves ) );
		}
	} );

	xag.clear_values();
}

} /* namespace detail */

/* garbling cost-friendly convert a XAG into a X1G:       */
/* two consecutive AND2s are implemented using one OneHot */
x1g_network map_xag2x1g( xag_network const& xag )
{
	topo_view<xag_network> xag_topo{ xag };
	fanout_view<topo_view<xag_network>> xag_merge{ xag_topo };

	std::unordered_map<fanout_view<topo_view<xag_network>>::node, std::vector<fanout_view<topo_view<xag_network>>::signal>> partition;
	detail::detect_and_group( xag_merge, partition );

	node_map<x1g_network::signal, fanout_view<topo_view<xag_network>>> xag2x1g( xag_merge );
	x1g_network x1g;

	/* generate constant */
	xag2x1g[xag_merge.get_constant( false )] = x1g.get_constant( false );

	/* generate pis */
	xag_merge.foreach_pi( [&]( auto const& n ) {
		xag2x1g[n] = x1g.create_pi();
	} );

	uint32_t group_cnt{ 0u };

	/* generate gates */
	xag_merge.foreach_gate( [&]( auto const& n ) {
		std::vector<x1g_network::signal> children;
		if ( xag_merge.is_xor( n ) )
		{
			xag_merge.foreach_fanin( n, [&]( auto const& f ) {
				children.emplace_back( xag2x1g[f] ^ xag_merge.is_complemented( f ) );
			} );
			xag2x1g[n] = x1g.create_xor( children[0], children[1] );
		}
		else
		{
			auto const search = partition.find( n );
			if ( search != partition.end() )
			{
				//std::cout << "[m] root of group " << ++group_cnt << " is node " << search->first << "\n";
				//std::cout << "[m] input signals are ";
				for ( auto const& leaf: search->second )
				{
					//std::cout << ( xag_merge.is_complemented( leaf ) ? "~" : "" ) << leaf.index << " ";
					children.emplace_back( xag2x1g[leaf] ^ xag_merge.is_complemented( leaf ) );
				}
				//std::cout << "\n";
				xag2x1g[n] = x1g.create_nary_and( children );
			}
			/* skip non-root AND nodes */
		}
	} );

	/* generate pos */
	xag_merge.foreach_po( [&]( auto const& f ) {
		x1g.create_po( xag2x1g[f] ^ xag_merge.is_complemented( f ) );
	} );

	/*
	std::cout << "[m] The built X1G's pis are: ";
	x1g.foreach_pi( [&]( auto const& n ) 
	{
		std::cout << n << " ";
	} );
	std::cout << "\n";
	std::cout << "[m] The gates are: ";
	x1g.foreach_gate( [&]( auto const& n ) 
	{
		uint32_t fanin_cnt{ 0u };
		if ( x1g.is_onehot( n ) )
		{
			std::cout << "OneHot( ";
			x1g.foreach_fanin( n, [&]( auto const& f ) {
				std::cout << ( x1g.is_complemented( f ) ? "~" : "" )
									<< f.index;
				++fanin_cnt;
				if ( fanin_cnt != 3 )
				{
					std::cout << ", ";
				}
			} );
			std::cout << " ) ";
		}
		else
		{
			std::cout << "XOR3( ";
			x1g.foreach_fanin( n, [&]( auto const& f ) {
				std::cout << ( x1g.is_complemented( f ) ? "~" : "" )
									<< f.index;
				++fanin_cnt;
				if ( fanin_cnt != 3 )
				{
					std::cout << ", ";
				}
			} );
			std::cout << " ) ";
		}
	} );
	std::cout << "\n";
	std::cout << "[m] The built X1G's pos are: ";
	x1g.foreach_po( [&]( auto const& f ) 
	{
		std::cout << ( x1g.is_complemented( f ) ? "~" : "" ) << f.index << " ";
	} );
	std::cout << "\n";
	*/
	return x1g;
}

} /* namespace mockturtle */
