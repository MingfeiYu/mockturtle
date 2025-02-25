#pragma once

#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <parallel_hashmap/phmap.h>
#include <unordered_set>

#include <mockturtle/networks/xag.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/views/topo_view.hpp>

namespace mockturtle
{

typedef std::array<xag_network::signal, 2u> xag_label_t;

struct LabelHash
{
	size_t operator()( xag_label_t const& label ) const
	{
		size_t res = 0;
		const uint64_t c1 = 0x87c37b91114253d5;
		const uint64_t c2 = 0x4cf5ad432745937f;
		uint64_t a1 = label[0].data;
		uint64_t a2 = label[1].data;

		a1 ^= ( a1 >> 33 );
    a1 *= c1;
    a1 ^= ( a1 >> 33 );
    a1 *= c2;
    a1 ^= ( a1 >> 33 );
    a2 ^= ( a2 >> 33 );
    a2 *= c1;
    a2 ^= ( a2 >> 33 );
    a2 *= c2;
    a2 ^= ( a2 >> 33 );
    res = a1 ^ a2;
    res *= c1;
    res ^= ( res >> 33 );
    res *= c2;
    res ^= ( res >> 33 );

    return res;
	}
};

typedef phmap::flat_hash_map<xag_label_t, std::vector<uint32_t>, LabelHash> mvfbs_map_t;

void detect_mvfbs( xag_network const& ntk, mvfbs_map_t& mvfbs_map, node_map<bool, xag_network>& shall_be_negated )
{
	topo_view<xag_network> ntk_topo{ ntk };
	ntk_topo.foreach_node( [&]( xag_network::node const& n ) {
		if ( ntk_topo.is_pi( n ) || ntk_topo.is_constant( n ) )
		{
			return true;
		}

		uint8_t i{ 0u };
		xag_label_t label = { ntk_topo.make_signal( 0u ), ntk_topo.make_signal( 0u ) };
		ntk_topo.foreach_fanin( n, [&]( xag_network::signal const& fi ) {
			label[i++] = fi;
			if ( ntk_topo.is_complemented( fi ) )
			{
				shall_be_negated[ntk_topo.get_node( fi )] = true;
			}
		} );
		if ( ntk_topo.get_node( label[0] ) > ntk_topo.get_node( label[1] ) )
		{
			std::swap( label[0], label[1] );
		}

		if ( mvfbs_map.contains( label ) )
		{
			std::vector<uint32_t>& shared_nodes = mvfbs_map.at( label );
			assert( shared_nodes.size() < 2 );
			shared_nodes.resize( shared_nodes.size() + 1 );
			shared_nodes[shared_nodes.size() - 1] = ntk_topo.node_to_index( n );
			// shared_nodes.push_back( ntk_topo.node_to_index( n ) );
		}
		else
		{
			std::vector<uint32_t> shared_nodes;
			shared_nodes.reserve( 2 );
			shared_nodes.push_back( ntk_topo.node_to_index( n ) );
			mvfbs_map.emplace( label, shared_nodes );
		}

		return true;
	} );

	ntk_topo.foreach_po( [&]( xag_network::signal const& po ) {
		if ( ntk_topo.is_complemented( po ) )
		{
			shall_be_negated[ntk_topo.get_node( po )] = true;
		}
	} );
}

void xag2lbf( xag_network const& ntk, mvfbs_map_t const& mvfbs_map, node_map<bool, xag_network> const& shall_be_negated, std::string const& filename )
{
	std::ofstream os( filename.c_str(), std::ofstream::out );
	topo_view<xag_network> ntk_topo{ ntk };
	std::unordered_set<xag_label_t, LabelHash> handled;
	phmap::flat_hash_map<uint32_t, std::string> shared_nodes_names;

	for ( auto it{ std::begin( mvfbs_map ) }; it != std::end( mvfbs_map ); ++it )
	{
		std::string shared_nodes_name = {};
		std::vector<uint32_t> shared_nodes = it->second;
		for ( uint8_t i{ 0u }; i < shared_nodes.size(); ++i )
		{
			shared_nodes_name += fmt::format( "n{}", shared_nodes[i] );
			if ( i != shared_nodes.size() - 1 )
			{
				shared_nodes_name += ',';
			}
		}
		for ( uint32_t each_node : shared_nodes )
		{
			shared_nodes_names.emplace( each_node, shared_nodes_name );
		}
	}

	/* write inputs */
	if ( ntk_topo.num_pis() > 0u )
	{
		os << ".inputs ";
		ntk_topo.foreach_pi( [&]( xag_network::node const& n ) {
			const std::string input_name = fmt::format( "pi{}", ntk_topo.node_to_index( n ) );
			os << input_name << ' ';
		} );
		os << "\n";
	}

	/* write outputs */
	bool has_zero{ false };
	bool has_one{ false };
	if ( ntk_topo.num_pos() > 0u )
	{
		os << ".outputs ";
		ntk_topo.foreach_po( [&]( xag_network::signal const& f ) {
			xag_network::node nf = ntk_topo.get_node( f );
			std::string postfix = ntk_topo.is_complemented( f ) ? "_neg" : "";

			if ( ntk_topo.is_constant( nf ) )
			{
				if ( ntk_topo.is_complemented( f ) )
				{
					os << fmt::format( "CONST1" ) << ' ';
					has_one = true;
				}
				else
				{
					os << fmt::format( "CONST0" ) << ' ';
					has_zero = true;
				}
				return true;
			}

			if ( ntk_topo.is_pi( nf ) )
			{
				os << fmt::format( "pi{}{}", ntk_topo.node_to_index( nf ), postfix ) << ' ';
				return true;
			}

			if ( postfix != "" )
			{
				os << fmt::format( "n{}_neg", ntk_topo.node_to_index( nf ) ) << ' ';
				return true;
			}

			os << shared_nodes_names.at( ntk_topo.node_to_index( nf ) ) << ' ';
			return true;
		} );
		os << "\n";
	}

	/* write constants */
	if ( has_zero )
	{
		os << ".lincomb CONST0\n";
		os << "0\n";
	}
	if ( has_one )
	{
		os << ".lincomb CONST1\n";
		os << "1\n";
	}

	/* write LUTs */
	ntk_topo.foreach_node( [&]( xag_network::node const& n ) {
		if ( ntk_topo.is_constant( n ) )
		{
			return true;
		}

		if ( ntk_topo.is_pi( n ) )
		{
			if ( shall_be_negated[n] )
			{
				os << fmt::format( ".lincomb " );
				uint32_t ind{ ntk_topo.node_to_index( n ) };
				os << fmt::format( "pi{}", ind ) << ' ';
				os << fmt::format( "pi{}_neg", ind ) << '\n';
				os << "-1 1\n";
			}
			return true;
		}

		uint8_t i{ 0u };
		xag_label_t current_label = { ntk_topo.make_signal( 0u ), ntk_topo.make_signal( 0u ) };
		ntk_topo.foreach_fanin( n, [&]( xag_network::signal const& fi ) {
			current_label[i++] = fi;
		} );
		if ( ntk_topo.get_node( current_label[0] ) > ntk_topo.get_node( current_label[1] ) )
		{
			std::swap( current_label[0], current_label[1] );
		}
		if ( handled.find( current_label ) == handled.end() )
		{
			handled.insert( current_label );

			os << fmt::format( ".lincomb " );
			ntk_topo.foreach_fanin( n, [&]( xag_network::signal const& fi ) {
				uint32_t fi_ind = ntk_topo.node_to_index( ntk_topo.get_node( fi ) );
				if ( ntk_topo.is_pi( ntk_topo.get_node( fi ) ) )
				{
					os << ( ntk_topo.is_complemented( fi ) ? fmt::format( "pi{}_neg", fi_ind ) : fmt::format( "pi{}", fi_ind ) ) << ' ';
					return true;
				}
				if ( ntk_topo.is_complemented( fi ) )
				{
					os << fmt::format( "n{}_neg", fi_ind );
				}
				else
				{
					os << shared_nodes_names.at( fi_ind );
				}
				os << ' ';
				return true;
			} );

			const std::string interm_name = fmt::format( "n{}_int", ntk_topo.node_to_index( n ) );
			os << interm_name << '\n';
			os << "2 1\n";

			os << ".bootstrap " << interm_name << ' ';
			std::vector<uint32_t> indices = mvfbs_map.at( current_label );
			for ( auto j{ 0u }; j < indices.size(); ++j )
			{
				os << fmt::format( "n{}", indices[j] );
				if ( j != indices.size() - 1 )
				{
					os << ' ';
				}
			}
			os << '\n';

			for ( auto j{ 0u }; j < indices.size(); ++j )
			{
				if ( ntk_topo.is_and( ntk_topo.index_to_node( indices[j] ) ) )
				{
					os << "0001\n";
				}
				else
				{
					os << "0110\n";
				}
			}

			std::string shared_nodes_name = shared_nodes_names.at( ntk_topo.node_to_index( n ) );
			for ( auto j{ 0u }; j < indices.size(); ++j )
			{
				if ( shall_be_negated[ntk_topo.index_to_node( indices[j] )] )
				{
					os << fmt::format( ".lincomb " );
					os << shared_nodes_name << ' ';
					os << fmt::format( "n{}_neg", indices[j] ) << '\n';
					os << "-1 1\n";
				}
			}
		}
		return true;
	} );

	os << ".end\n";
}

void shall_be_negated_init( xag_network const& ntk, node_map<bool, xag_network>& shall_be_negated )
{
	topo_view<xag_network> ntk_topo{ ntk };
	ntk_topo.foreach_node( [&]( xag_network::node const& n ) {
		if ( ntk_topo.is_pi( n ) || ntk_topo.is_constant( n ) )
		{
			return true;
		}

		uint8_t i{ 0u };
		ntk_topo.foreach_fanin( n, [&]( xag_network::signal const& fi ) {
			if ( ntk_topo.is_complemented( fi ) )
			{
				shall_be_negated[ntk_topo.get_node( fi )] = true;
			}
		} );

		return true;
	} );

	ntk_topo.foreach_po( [&]( xag_network::signal const& po ) {
		if ( ntk_topo.is_complemented( po ) )
		{
			shall_be_negated[ntk_topo.get_node( po )] = true;
		}
	} );
}

void xag2lbf_beta( xag_network const& ntk, node_map<bool, xag_network> const& shall_be_negated, std::string const& filename )
{
	/* Since the computation of XOR does not rely on FBS, */
	/* MVFBS is not applicable in the beta version. */
	std::ofstream os( filename.c_str(), std::ofstream::out );
	topo_view<xag_network> ntk_topo{ ntk };

	/* write inputs */
	if ( ntk_topo.num_pis() > 0u )
	{
		os << ".inputs ";
		ntk_topo.foreach_pi( [&]( xag_network::node const& n ) {
			const std::string input_name = fmt::format( "pi{}", ntk_topo.node_to_index( n ) );
			os << input_name << ' ';
		} );
		os << "\n";
	}

	/* write outputs */
	bool has_zero{ false };
	bool has_one{ false };
	if ( ntk_topo.num_pos() > 0u )
	{
		os << ".outputs ";
		ntk_topo.foreach_po( [&]( xag_network::signal const& f ) {
			xag_network::node nf = ntk_topo.get_node( f );
			std::string postfix = ntk_topo.is_complemented( f ) ? "_neg" : "";

			if ( ntk_topo.is_constant( nf ) )
			{
				if ( ntk_topo.is_complemented( f ) )
				{
					os << fmt::format( "CONST1" ) << ' ';
					has_one = true;
				}
				else
				{
					os << fmt::format( "CONST0" ) << ' ';
					has_zero = true;
				}
				return true;
			}

			if ( ntk_topo.is_pi( nf ) )
			{
				os << fmt::format( "pi{}{}", ntk_topo.node_to_index( nf ), postfix ) << ' ';
				return true;
			}

			if ( postfix != "" )
			{
				os << fmt::format( "n{}_neg", ntk_topo.node_to_index( nf ) ) << ' ';
				return true;
			}

			os << fmt::format( "n{}", ntk_topo.node_to_index( nf ) ) << ' ';
			return true;
		} );
		os << "\n";
	}

	/* write constants */
	if ( has_zero )
	{
		os << ".lincomb CONST0\n";
		os << "0\n";
	}
	if ( has_one )
	{
		os << ".lincomb CONST1\n";
		os << "1\n";
	}

	/* write LUTs */
	ntk_topo.foreach_node( [&]( xag_network::node const& n ) {
		if ( ntk_topo.is_constant( n ) )
		{
			return true;
		}

		uint32_t ind{ ntk_topo.node_to_index( n ) };
		if ( ntk_topo.is_pi( n ) )
		{
			if ( shall_be_negated[n] )
			{
				os << fmt::format( ".lincomb " );
				os << fmt::format( "pi{}", ind ) << ' ';
				os << fmt::format( "pi{}_neg", ind ) << '\n';
				os << "-1 1\n";
			}
			return true;
		}

		bool is_and = ntk_topo.is_and( n );
		os << fmt::format( ".lincomb " );
		ntk_topo.foreach_fanin( n, [&]( xag_network::signal const& fi ) {
			uint32_t fi_ind = ntk_topo.node_to_index( ntk_topo.get_node( fi ) );
			if ( ntk_topo.is_pi( ntk_topo.get_node( fi ) ) )
			{
				os << ( ntk_topo.is_complemented( fi ) ? fmt::format( "pi{}_neg", fi_ind ) : fmt::format( "pi{}", fi_ind ) ) << ' ';
				return true;
			}
			if ( ntk_topo.is_complemented( fi ) )
			{
				os << fmt::format( "n{}_neg", fi_ind );
			}
			else
			{
				os << fmt::format( "n{}", fi_ind );
			}
			os << ' ';
			return true;
		} );

		const std::string interm_name = fmt::format( "n{}_int", ind );
		os << interm_name << '\n';
		os << ( is_and ? "1 1\n" : "2 1\n" );

		os << ".bootstrap " << interm_name << ' ';
		os << fmt::format( "n{}\n", ind );
		os << ( is_and ? "001\n" : "0110\n" );

		if ( shall_be_negated[n] )
		{
			os << fmt::format( ".lincomb " );
			os << fmt::format( "n{}", ind ) << ' ';
			os << fmt::format( "n{}_neg", ind ) << '\n';
			os << "-1 1\n";
		}

		return true;
	} );

	os << ".end\n";
}

} /* namespace mockturtle */