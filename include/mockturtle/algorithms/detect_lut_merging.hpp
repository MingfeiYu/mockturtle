#pragma once

#include <algorithm>
#include <parallel_hashmap/phmap.h>

#include <kitty/print.hpp>
#include <kitty/properties.hpp>

#include "cleanup.hpp"
#include "../networks/klut.hpp"

struct ArrayHash
{
  size_t operator()( std::array<uint32_t, 4u> const& arr ) const
  {
    size_t seed = 0;
    for ( uint32_t value : arr ) 
    {
      seed ^= value + 0x9e3779b97f4a7c55 + ( seed << 6 ) + ( seed >> 2 );
    }
    return seed;
  }
};

typedef std::array<uint32_t, 4u> label_t;
typedef phmap::flat_hash_map<label_t, std::vector<uint32_t>, ArrayHash> mergable_luts_map_t;

enum MergeType: uint8_t 
{
    Trivial = 0u,
    Symmetric = 1u,
    Negacyclic = 2u,
    Normal = 3u,
    Invalid = 4u
};

namespace mockturtle
{

klut_network invert_isolation( klut_network& ntk )
{
	ntk.foreach_gate( [&]( klut_network::node const& n ) {
		auto const& node = ntk._storage->nodes[ntk.node_to_index( n )];
		auto const tt = ntk.node_function( n );

		if ( tt.num_vars() == 3u )
		{
			if ( kitty::is_symmetric( tt ) || kitty::is_top_xor_decomposible( tt ) )
			{
				return true;
			}

			uint8_t num_vars = tt.num_vars();
			const auto sym_check = kitty::is_symmetric_n( tt );

			if ( !( std::get<0>( sym_check ) ) )
			{
				std::cerr << "[e] a 3-LUT whose function is neither symmetric nor negacyclic is detected.\n";
				std::cerr << "[e] TT: ";
				kitty::print_hex( tt );
				std::cerr << std::endl;
				abort();
			}

			std::vector<klut_network::signal> children;
			for ( auto const& child : node.children )
			{
				children.emplace_back( child.index );
			}
			std::vector<klut_network::signal> children_new( children.size() );

			const uint8_t phase =std::get<2>( sym_check );
			for ( uint8_t i{ 0u }; i < num_vars; ++i )
			{
				if ( ( phase >> i ) & 1 )
				{
					/* If child node also functions an inverter, the two inverters cancel each other */
					/* disjoint in NEG */
					klut_network::node const& child_i = ntk.index_to_node( children[i] );
					if ( ntk.is_not( child_i ) && ( ntk.fanout_size( child_i ) == 1u ) )
					{
						children_new[i] = ntk.make_signal( ntk.index_to_node( ntk._storage->nodes[children[i]].children[0].index ) );
					}
					else
					{
						children_new[i] = ntk.create_not( children[i] );
					}
				}
				else
				{
					children_new[i] = children[i];
				}
			}

			ntk.substitute_node( n, ntk.create_node( children_new, std::get<1>( sym_check ) ) );
		}

		return true;
	} );

	return cleanup_dangling( ntk );
}

void detect_lut_merging( klut_network const& ntk )
{ 
	uint32_t num_3_luts_sym_nm{ 0u };
	uint32_t num_3_luts_neg_nm{ 0u };
	uint32_t num_2_luts_nm{ 0u };
	uint32_t num_3_luts_sym{ 0u };
	uint32_t num_3_luts_neg{ 0u };
	uint32_t num_2_luts{ 0u };
	uint32_t num_not{ 0u };

	mergable_luts_map_t mergable_luts_map;
	ntk.foreach_node( [&]( klut_network::node const& n ) {

		/* skip constants and PIs */
		if ( ntk.is_pi( n ) || ntk.is_constant( n ) )
		{
			return true;
		}

		uint32_t idx = ntk.node_to_index( n );
		auto const node = ntk._storage->nodes[idx];

		/* get children */
		std::array<uint32_t, 3u> leaves;
		auto cnt{ 0u };
		for ( auto const& child : node.children )
		{
			leaves[cnt++] = child.index;
		}
		// std::sort( std::begin( leaves ), std::end( leaves ) );

		label_t label;
		for ( auto i{ 0u }; i < 3u; ++i )
		{
			label[i] = leaves[i];
		}

		/* figure out function property */
		auto const tt = ntk.node_function( n );
		if ( tt.num_vars() == 3u )
		{
			// either symmetric or negacyclic, otherwise illegal
			if ( kitty::is_symmetric( tt ) )
			{
				std::sort( std::begin( label ), ( std::end( label ) - 1 ) );
				label[3] = static_cast<uint32_t>( MergeType::Symmetric );
				++num_3_luts_sym_nm;
			}
			else if ( std::tuple<bool, uint8_t> neg_check = kitty::is_top_xor_decomposible_return_support( tt ); std::get<0>( neg_check ) )
			{
				/* store node index of the disjoint support in label[0] */
				/* reorder the remaining two supports */
				uint32_t disjoint_support_idx = leaves[std::get<1>( neg_check )];
				if ( disjoint_support_idx != label[0] )
				{
					for ( uint8_t i{ 1u }; i < tt.num_vars(); ++i )
					{
						if ( disjoint_support_idx == label[i] )
						{
							std::swap( label[0], label[i] );
							break;
						}
					}
				}
				std::sort( ( std::begin( label ) + 1 ), ( std::end( label ) - 1 ) );
				label[3] = static_cast<uint32_t>( MergeType::Negacyclic );
				++num_3_luts_neg_nm;
			}
			else
			{
				std::cerr << "[e] a 3-LUT whose function is neither symmetric nor negacyclic is detected.\n";
				std::cerr << "[e] TT: ";
				kitty::print_hex( tt );
				std::cerr << std::endl;
				abort();
			}

			if ( mergable_luts_map.contains( label ) )
			{
				mergable_luts_map.at( label ).emplace_back( idx );
			}
			else
			{
				std::vector<uint32_t> indices( 1u );
				indices[0]= idx;
				mergable_luts_map.emplace( label, indices );
				if ( label[3] == Symmetric )
				{
					++num_3_luts_sym;
				}
				else
				{
					++num_3_luts_neg;
				}
			}
		}
		else if ( tt.num_vars() == 2u )
		{
			label[2] = 0u;
			std::sort( std::begin( label ), ( std::end( label ) - 2 ) );
			label[3] = MergeType::Normal;
			
			if ( mergable_luts_map.contains( label ) )
			{
				mergable_luts_map.at( label ).emplace_back( idx );
			}
			else
			{
				std::vector<uint32_t> indices( 1u );
				indices[0]= idx;
				mergable_luts_map.emplace( label, indices );
				++num_2_luts;
			}

			++num_2_luts_nm;
		}
		else
		{
			++num_not;
		}

		return true;
	} );

	std::cout << "[i] Without merging:\n";
	std::cout << "[i] #BRs: " << ( num_3_luts_sym_nm + num_3_luts_neg_nm + num_2_luts_nm ) << "\n";
	std::cout << "[i] #3-LUTs: " << ( num_3_luts_sym_nm + num_3_luts_neg_nm ) << "( SYM: " << num_3_luts_sym_nm << ", NEG: " << num_3_luts_neg_nm << " )\n";
	std::cout << "[i] #2-LUTs: " << num_2_luts_nm << "\n";
	std::cout << "[i] #invertors: " << num_not << "\n\n";

	std::cout << "[i] With merging:\n";
	std::cout << "[i] #BRs: " << ( num_3_luts_sym + num_3_luts_neg+ num_2_luts ) << "\n";
	std::cout << "[i] #3-LUTs: " << ( num_3_luts_sym + num_3_luts_neg ) << "( SYM: " << num_3_luts_sym << ", NEG: " << num_3_luts_neg << " )\n";
	std::cout << "[i] #2-LUTs: " << num_2_luts << "\n";
	std::cout << "[i] #invertors: " << num_not << "\n\n";
}

void detect_lut_merging_ad( klut_network const& ntk )
{ 
	uint32_t num_3_luts_sym_nm{ 0u };
	uint32_t num_3_luts_neg_nm{ 0u };
	uint32_t num_2_luts_nm{ 0u };
	uint32_t num_3_luts_sym{ 0u };
	uint32_t num_3_luts_neg{ 0u };
	uint32_t num_2_luts{ 0u };
	uint32_t num_not{ 0u };

	mergable_luts_map_t mergable_luts_map;
	std::vector<label_t> neg_cut_labels;
	ntk.foreach_gate( [&]( klut_network::node const& n ) {

		/* figure out function property */
		auto const tt = ntk.node_function( n );
		if ( tt.num_vars() == 3u )
		{
			uint32_t idx = ntk.node_to_index( n );
			auto const node = ntk._storage->nodes[idx];

			/* get children */
			std::array<uint32_t, 3u> leaves;
			auto cnt{ 0u };
			for ( auto const& child : node.children )
			{
				leaves[cnt++] = child.index;
			}
			std::sort( std::begin( leaves ), std::end( leaves ) );

			label_t label;
			for ( auto i{ 0u }; i < 3u; ++i )
			{
				label[i] = leaves[i];
			}

			// either symmetric or negacyclic, otherwise illegal
			if ( kitty::is_symmetric( tt ) )
			{
				std::sort( std::begin( label ), ( std::end( label ) - 1 ) );
				label[3] = static_cast<uint32_t>( MergeType::Symmetric );
				++num_3_luts_sym_nm;
			}
			else if ( std::tuple<bool, uint8_t> neg_check = kitty::is_top_xor_decomposible_return_support( tt ); std::get<0>( neg_check ) )
			{
				/* store node index of the disjoint support in label[0] */
				/* reorder the remaining two supports */
				uint32_t disjoint_support_idx = leaves[std::get<1>( neg_check )];
				if ( disjoint_support_idx != label[0] )
				{
					for ( uint8_t i{ 1u }; i < tt.num_vars(); ++i )
					{
						if ( disjoint_support_idx == label[i] )
						{
							std::swap( label[0], label[i] );
							break;
						}
					}
				}
				std::sort( ( std::begin( label ) + 1 ), ( std::end( label ) - 1 ) );
				label[3] = static_cast<uint32_t>( MergeType::Negacyclic );
				++num_3_luts_neg_nm;
			}
			else
			{
				std::cerr << "[e] a 3-LUT whose function is neither symmetric nor negacyclic is detected.\n";
				std::cerr << "[e] TT: ";
				kitty::print_hex( tt );
				std::cerr << std::endl;
				abort();
			}

			if ( mergable_luts_map.contains( label ) )
			{
				mergable_luts_map.at( label ).emplace_back( idx );
			}
			else
			{
				std::vector<uint32_t> indices( 1u );
				indices[0]= idx;
				mergable_luts_map.emplace( label, indices );
				if ( label[3] == MergeType::Symmetric )
				{
					++num_3_luts_sym;
				}
				else
				{
					++num_3_luts_neg;
					neg_cut_labels.emplace_back( label );
				}
			}
		}
		else if ( tt.num_vars() == 1u )
		{
			++num_not;
		}

		return true;
	} );

	ntk.foreach_gate( [&]( klut_network::node const& n ) {

		auto const tt = ntk.node_function( n );
		if ( tt.num_vars() == 2u )
		{
			uint32_t idx = ntk.node_to_index( n );
			auto const node = ntk._storage->nodes[idx];

			/* get children */
			std::array<uint32_t, 3u> leaves;
			auto cnt{ 0u };
			for ( auto const& child : node.children )
			{
				leaves[cnt++] = child.index;
			}

			label_t label;
			for ( auto i{ 0u }; i < 3u; ++i )
			{
				label[i] = leaves[i];
			}
			label[2] = 0u;
			std::sort( std::begin( label ), ( std::end( label ) - 2 ) );
			label[3] = static_cast<uint32_t>( MergeType::Normal );
			++num_2_luts_nm;

			for ( label_t const& target : neg_cut_labels )
			{
				if ( std::make_pair( label[0], label[1] ) == std::make_pair( target[1], target[2] ) )
				{
					if ( !mergable_luts_map.contains( target ) )
					{
						std::cerr << "[e] merging a 2-LUT with a not committed negacyclic 3-LUT!\n";
						abort();
					}
					mergable_luts_map.at( target ).emplace_back( idx );
					return true;
				}
			}

			if ( mergable_luts_map.contains( label ) )
			{
				mergable_luts_map.at( label ).emplace_back( idx );
			}
			else
			{
				std::vector<uint32_t> indices( 1u );
				indices[0] = idx;
				mergable_luts_map.emplace( label, indices );
				++num_2_luts;
			}
		}

		return true;
	} );

	std::cout << "[i] Without merging:\n";
	std::cout << "[i] #BRs: " << ( num_3_luts_sym_nm + num_3_luts_neg_nm + num_2_luts_nm ) << "\n";
	std::cout << "[i] #3-LUTs: " << ( num_3_luts_sym_nm + num_3_luts_neg_nm ) << "( SYM: " << num_3_luts_sym_nm << ", NEG: " << num_3_luts_neg_nm << " )\n";
	std::cout << "[i] #2-LUTs: " << num_2_luts_nm << "\n";
	std::cout << "[i] #invertors: " << num_not << "\n\n";

	std::cout << "[i] With merging:\n";
	std::cout << "[i] #BRs: " << ( num_3_luts_sym + num_3_luts_neg+ num_2_luts ) << "\n";
	std::cout << "[i] #3-LUTs: " << ( num_3_luts_sym + num_3_luts_neg ) << "( SYM: " << num_3_luts_sym << ", NEG: " << num_3_luts_neg << " )\n";
	std::cout << "[i] #2-LUTs: " << num_2_luts << "\n";
	std::cout << "[i] #invertors: " << num_not << "\n\n";

}

} /* namespace mockturtle */
