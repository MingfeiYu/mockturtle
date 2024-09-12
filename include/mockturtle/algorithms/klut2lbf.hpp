#pragma once

#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <parallel_hashmap/phmap.h>
#include <unordered_set>
#include <string>

#include "../networks/klut.hpp"
#include "../utils/node_map.hpp"
#include "../views/topo_view.hpp"

#include <kitty/bit_operations.hpp>
#include <kitty/operations.hpp>

namespace mockturtle
{

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

void klut2lbf( klut_network const& ntk, mergable_luts_map_t& mergable_luts_map, node_map<label_t, klut_network> const& node_to_label, std::string const& filename )
{
	std::ofstream os( filename.c_str(), std::ofstream::out );

	topo_view<klut_network> ntk_topo{ ntk };
	// std::unordered_set<std::string> names;
	std::unordered_set<label_t, ArrayHash> handled;

	/* write inputs */
	if ( ntk_topo.num_pis() > 0u )
	{
		os << ".inputs ";
		ntk_topo.foreach_pi( [&]( klut_network::node const& n ) {
			const std::string input_name = fmt::format( "pi{}", ntk_topo.node_to_index( n ) );
			os << input_name << ' ';
			// names.insert( input_name );
		} );
		os << "\n";
	}

	/* write outputs */
	bool has_zero{ false };
	bool has_one{ false };
	if ( ntk_topo.num_pos() > 0u )
	{
		os << ".outputs ";
		ntk_topo.foreach_po( [&]( klut_network::signal const& f, uint32_t index ) {
			// os << fmt::format( "po{} ", index );
			klut_network::node nf = ntk_topo.get_node( f );
			std::string output_name = {};
			if ( ntk_topo.is_pi( nf ) )
			{
				output_name = fmt::format( "pi{}", ntk_topo.node_to_index( nf ) );
			}
			else
			{
				if ( ntk_topo.is_constant( nf ) )
				{
					if ( nf == 0u )
					{
						output_name = fmt::format( "CONST0" );
						has_zero = true;
					}
					else
					{
						output_name = fmt::format( "CONST1" );
						has_one = true;
					}
				}
				else
				{
					output_name = fmt::format( "n{}", ntk_topo.node_to_index( nf ) );
				}
			}
			os << output_name << ' ';
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
	ntk_topo.foreach_node( [&]( klut_network::node const& n ) {
		if ( ntk_topo.is_constant( n ) || ntk_topo.is_pi( n ) )
		{
			return true;
		}

		/* handle inverter */
		if ( node_to_label[n][3] == MergeType::Invalid )
		{
			os << fmt::format( ".lincomb " );
			ntk_topo.foreach_fanin( n, [&]( klut_network::signal const& f ) {
				klut_network::node nf = ntk_topo.get_node( f );
				const std::string fanin_name = ntk_topo.is_pi( nf ) ? fmt::format( "pi{}", ntk_topo.node_to_index( nf ) ) : fmt::format( "n{}", ntk_topo.node_to_index( nf ) );
				os << fanin_name << ' ';
			} );
			const std::string n_name = fmt::format( "n{}", ntk_topo.node_to_index( n ) );
			os << n_name << '\n';
			// names.insert( n_name );
			os << "-1 1\n";
			return true;
		}

		/* refer to 'handled' */
		label_t current_label = node_to_label[n];
		if ( handled.find( current_label ) == handled.end() )
		{
			/* update 'handled' */
			handled.insert( current_label );

			os << fmt::format( ".lincomb " );
			switch( current_label[3] )
			{
			case MergeType::Symmetric:
			{
				/* write fan-ins of node */
				for ( auto i{ 0u }; i < 3u; ++i )
				{
					uint32_t current_label_i = current_label[i];
					const std::string fanin_name = ntk_topo.is_pi( ntk_topo.index_to_node( current_label_i ) ) ? fmt::format( "pi{}", current_label_i ) : fmt::format( "n{}", current_label_i );
					os << fanin_name << ' ';
				}

				/* write intermediate result */
				const std::string interm_name = fmt::format( "in{}", ntk_topo.node_to_index( n ) );
				os << interm_name << '\n';
				os << "1 1 1\n";

				/* write fan-outs */
				os << ".bootstrap " << interm_name << " ";
				std::vector<uint32_t> indices = mergable_luts_map[current_label];
				for ( uint32_t i{ 0u }; i < indices.size(); ++i )
				{
					const std::string fanout_name = fmt::format( "n{}", ntk_topo.index_to_node( indices[i] ) );
					os << fanout_name;
					if ( i != indices.size() - 1u )
					{
						os << ' ';
					}
					// names.insert( fanout_name );
				}
				os << '\n';

				/* write (projected) truth table */
				for ( uint32_t const& index : indices )
				{
					auto const tt = ntk_topo.node_function( ntk_topo.index_to_node( index ) );
					uint32_t i{ 0u };
					os << fmt::format( "{}", kitty::get_bit( tt, i ) );
					while ( i < tt.num_bits() )
					{
						// os << ' ';
						i += 1u;
						os << fmt::format( "{}", kitty::get_bit( tt, i ) );
						i <<= 1u;
					}
					os << '\n';
				}
				break;
			}
			case MergeType::Negacyclic:
			{
				/* write fan-ins of node */
				for ( auto i{ 0u }; i < 3u; ++i )
				{
					uint32_t current_label_i = current_label[i];
					const std::string fanin_name = ntk_topo.is_pi( ntk_topo.index_to_node( current_label_i ) ) ? fmt::format( "pi{}", current_label_i ) : fmt::format( "n{}", current_label_i );
					os << fanin_name << ' ';
				}

				/* write intermediate result */
				const std::string interm_name = fmt::format( "in{}", ntk_topo.node_to_index( n ) );
				os << interm_name << '\n';
				os << "4 1 2\n";

				/* write fan-outs */
				os << ".bootstrap " << interm_name << " ";
				std::vector<uint32_t> indices = mergable_luts_map[current_label];
				for ( uint32_t i{ 0u }; i < indices.size(); ++i )
				{
					const std::string fanout_name = fmt::format( "n{}", ntk_topo.index_to_node( indices[i] ) );
					os << fanout_name;
					if ( i != indices.size() - 1u )
					{
						os << ' ';
					}
					// names.insert( fanout_name );
				}
				os << '\n';

				/* write (projected) truth table */
				uint8_t i_target{ 0u };
				bool is_top_xor{ false };
				auto const tt_eg = ntk_topo.node_function( ntk_topo.index_to_node( indices[0] ) );
				std::make_pair( is_top_xor, i_target ) = kitty::is_top_xor_decomposible_return_support( tt_eg );
				if ( !is_top_xor )
				{
					abort();
				}
				{
					auto const cofact0 = kitty::cofactor0( tt_eg, i_target );
					auto const cofact1 = kitty::cofactor1( tt_eg, i_target );
					std::cout << "[m] tt of cofactor0: ";
					kitty::print_binary( cofact0 );
					std::cout << "\n";
					std::cout << "[m] tt of cofactor1: ";
					kitty::print_binary( cofact1 );
					std::cout << "\n";

					for ( uint32_t i{ 0u }; i < tt_eg.num_bits(); ++i )
					{
						if ( i < tt_eg.num_bits() / 2 )
						{
							os << fmt::format( "{}", kitty::get_bit( cofact0, i ) );
						}
						else
						{
							os << fmt::format( "{}", kitty::get_bit( cofact1, i ) );
						}
					}
					os << '\n';
				}

				for ( uint8_t i{ 1u }; i < indices.size(); ++i )
				{
					auto const tt = ntk_topo.node_function( ntk_topo.index_to_node( indices[i] ) );
					auto const cofact0 = kitty::cofactor0( tt, i_target );
					for ( uint32_t i{ 0u }; i < tt.num_bits(); ++i )
					{
						if ( i < tt.num_bits() / 2 )
						{
							os << fmt::format( "{}", kitty::get_bit( cofact0, i ) );
						}
						else
						{
							os << fmt::format( "{}", !kitty::get_bit( cofact0, i ) );
						}
					}
					os << '\n';
				}
				break;
			}
			case MergeType::Normal:
			{
				/* write fan-ins of node */
				for ( auto i{ 0u }; i < 2u; ++i )
				{
					uint32_t current_label_i = current_label[i];
					const std::string fanin_name = ntk_topo.is_pi( ntk_topo.index_to_node( current_label_i ) ) ? fmt::format( "pi{}", current_label_i ) : fmt::format( "n{}", current_label_i );
					os << fanin_name << ' ';
				}

				/* write intermediate result */
				const std::string interm_name = fmt::format( "in{}", ntk_topo.node_to_index( n ) );
				os << interm_name << '\n';
				os << "1 2\n";

				/* write fan-outs */
				os << ".bootstrap " << interm_name << " ";
				std::vector<uint32_t> indices = mergable_luts_map[current_label];
				for ( uint32_t i{ 0u }; i < indices.size(); ++i )
				{
					const std::string fanout_name = fmt::format( "n{}", ntk_topo.index_to_node( indices[i] ) );
					os << fanout_name;
					if ( i != indices.size() - 1u )
					{
						os << ' ';
					}
					// names.insert( fanout_name );
				}
				os << '\n';

				/* write (projected) truth table */
				for ( uint32_t const& index : indices )
				{
					auto const tt = ntk_topo.node_function( ntk_topo.index_to_node( index ) );
					for ( uint32_t i{ 0u }; i < tt.num_bits(); ++i )
					{
						os << fmt::format( "{}", kitty::get_bit( tt, i ) );
						// if ( i != tt.num_bits() - 1u )
						// {
						// 	os << ' ';
						// }
					}
					os << '\n';
				}
				break;
			}
			default:
			{
				std::cerr << "[e] Encoutered invalid gate type!\n";
				abort();
				break;
			}
			}
		}

		return true;
	} );
	
	/* connect to POs */
	// ntk_topo.foreach_po( [&]( klut_network::signal const& f, uint32_t index ) {
	// 	os << ".lincomb ";
	// 	klut_network::node nf = ntk_topo.get_node( f );
	// 	const std::string fanin_name = ntk_topo.is_pi( nf ) ? fmt::format( "pi{}", ntk_topo.node_to_index( nf ) ) : fmt::format( "n{}", ntk_topo.node_to_index( nf ) );
	// 	os << fanin_name << ' ';
	// 	const std::string fanout_name = fmt::format( "po{}", index );
	// 	os << fanout_name << '\n';
	// 	os << "1\n";
	// } );

	os << ".end\n";
}

void klut2lbf_mod( klut_network const& ntk, mergable_luts_map_t& mergable_luts_map, node_map<label_t, klut_network> const& node_to_label, std::string const& filename )
{
	std::ofstream os( filename.c_str(), std::ofstream::out );

	topo_view<klut_network> ntk_topo{ ntk };
	// std::unordered_set<std::string> names;
	std::unordered_set<label_t, ArrayHash> handled;
	phmap::flat_hash_map<uint32_t, std::string> mo_node_names;

	for ( auto it{ std::begin( mergable_luts_map ) }; it != std::end( mergable_luts_map ); ++it )
	{
		std::string mo_node_name = {};
		std::vector<uint32_t> mo_nodes = it->second;
		for ( auto j{ 0u }; j < mo_nodes.size(); ++j )
		{
			mo_node_name += fmt::format( "n{}", mo_nodes[j] );
			if ( j != mo_nodes.size() - 1u )
			{
				mo_node_name += ',';
			}
		}
		for ( uint32_t each_node : mo_nodes )
		{
			mo_node_names.emplace( each_node, mo_node_name );
		}
	}

	/* write inputs */
	if ( ntk_topo.num_pis() > 0u )
	{
		os << ".inputs ";
		ntk_topo.foreach_pi( [&]( klut_network::node const& n ) {
			const std::string input_name = fmt::format( "pi{}", ntk_topo.node_to_index( n ) );
			os << input_name << ' ';
			// names.insert( input_name );
		} );
		os << "\n";
	}

	/* write outputs */
	bool has_zero{ false };
	bool has_one{ false };
	if ( ntk_topo.num_pos() > 0u )
	{
		os << ".outputs ";
		ntk_topo.foreach_po( [&]( klut_network::signal const& f ) {
			// os << fmt::format( "po{} ", index );
			klut_network::node nf = ntk_topo.get_node( f );
			std::string output_name = {};
			if ( ntk_topo.is_pi( nf ) )
			{
				output_name = fmt::format( "pi{}", ntk_topo.node_to_index( nf ) );
			}
			else
			{
				if ( ntk_topo.is_constant( nf ) )
				{
					if ( nf == 0u )
					{
						output_name = fmt::format( "CONST0" );
						has_zero = true;
					}
					else
					{
						output_name = fmt::format( "CONST1" );
						has_one = true;
					}
				}
				else
				{
					uint32_t index = ntk_topo.node_to_index( nf );
					if ( mo_node_names.contains( index ) )
					{
						output_name = mo_node_names.at( index );
					}
					else
					{
						/* necessary due to the existence of inverters */
						output_name = fmt::format( "n{}", index );
					}
				}
			}
			os << output_name << ' ';
		} );
		os << "\n";
	}
	mo_node_names.clear();

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
	ntk_topo.foreach_node( [&]( klut_network::node const& n ) {
		if ( ntk_topo.is_constant( n ) || ntk_topo.is_pi( n ) )
		{
			return true;
		}

		/* handle inverter */
		if ( node_to_label[n][3] == MergeType::Invalid )
		{
			os << fmt::format( ".lincomb " );
			ntk_topo.foreach_fanin( n, [&]( klut_network::signal const& f ) {
				klut_network::node nf = ntk_topo.get_node( f );
				uint32_t nf_index = ntk_topo.node_to_index( nf );
				std::string fanin_name = {};
				if ( ntk_topo.is_pi( nf ) )
				{
					fanin_name = fmt::format( "pi{}", nf_index );
				}
				else if ( mo_node_names.contains( nf_index ) )
				{
					fanin_name = mo_node_names.at( nf_index );
				}
				else
				{
					fanin_name = fmt::format( "n{}", nf_index );
				}
				os << fanin_name << ' ';
			} );
			const std::string n_name = fmt::format( "n{}", ntk_topo.node_to_index( n ) );
			os << n_name << '\n';
			// names.insert( n_name );
			os << "-1 1\n";
			return true;
		}

		/* refer to 'handled' */
		label_t current_label = node_to_label[n];
		if ( handled.find( current_label ) == handled.end() )
		{
			/* update 'handled' */
			handled.insert( current_label );

			os << fmt::format( ".lincomb " );
			switch( current_label[3] )
			{
			case MergeType::Symmetric:
			{
				/* write fan-ins of node */
				for ( auto i{ 0u }; i < 3u; ++i )
				{
					uint32_t current_label_i = current_label[i];
					std::string fanin_name = {};
					if ( ntk_topo.is_constant( ntk_topo.index_to_node( current_label_i ) ) )
					{
						fanin_name = fmt::format( "CONST{}", current_label_i );
					}
					else if ( ntk_topo.is_pi( ntk_topo.index_to_node( current_label_i ) ) )
					{
						fanin_name = fmt::format( "pi{}", current_label_i );
					}
					else if ( mo_node_names.contains( current_label_i ) )
					{
						fanin_name = mo_node_names.at( current_label_i );
					}
					else
					{
						fanin_name = fmt::format( "n{}", current_label_i );
					}
					os << fanin_name << ' ';
				}

				/* write intermediate result */
				const std::string interm_name = fmt::format( "in{}", ntk_topo.node_to_index( n ) );
				os << interm_name << '\n';
				os << "1 1 1\n";

				/* write fan-outs */
				os << ".bootstrap " << interm_name << " ";
				std::vector<uint32_t> indices = mergable_luts_map[current_label];
				for ( uint32_t i{ 0u }; i < indices.size(); ++i )
				{
					const std::string fanout_name = fmt::format( "n{}", ntk_topo.index_to_node( indices[i] ) );
					os << fanout_name;
					if ( i != indices.size() - 1u )
					{
						os << ' ';
					}
					// names.insert( fanout_name );
				}
				os << '\n';

				/* write (projected) truth table */
				for ( uint32_t const& index : indices )
				{
					auto const tt = ntk_topo.node_function( ntk_topo.index_to_node( index ) );
					uint32_t i{ 0u };
					os << fmt::format( "{}", kitty::get_bit( tt, i ) );
					while ( i < tt.num_bits() )
					{
						// os << ' ';
						i += 1u;
						os << fmt::format( "{}", kitty::get_bit( tt, i ) );
						i <<= 1u;
					}
					os << '\n';
				}
				break;
			}
			case MergeType::Negacyclic:
			{
				/* write fan-ins of node */
				for ( auto i{ 0u }; i < 3u; ++i )
				{
					uint32_t current_label_i = current_label[i];
					std::string fanin_name = {};
					if ( ntk_topo.is_pi( ntk_topo.index_to_node( current_label_i ) ) )
					{
						fanin_name = fmt::format( "pi{}", current_label_i );
					}
					else if ( mo_node_names.contains( current_label_i ) )
					{
						fanin_name = mo_node_names.at( current_label_i );
					}
					else
					{
						fanin_name = fmt::format( "n{}", current_label_i );
					}
					os << fanin_name << ' ';
				}

				/* write intermediate result */
				const std::string interm_name = fmt::format( "in{}", ntk_topo.node_to_index( n ) );
				os << interm_name << '\n';
				os << "4 1 2\n";

				/* write fan-outs */
				os << ".bootstrap " << interm_name << " ";
				std::vector<uint32_t> indices = mergable_luts_map[current_label];
				for ( uint32_t i{ 0u }; i < indices.size(); ++i )
				{
					const std::string fanout_name = fmt::format( "n{}", ntk_topo.index_to_node( indices[i] ) );
					os << fanout_name;
					if ( i != indices.size() - 1u )
					{
						os << ' ';
					}
					// names.insert( fanout_name );
				}
				os << '\n';

				/* write (projected) truth table */
				/* figure out index of the top-disjointable variable */
				uint8_t i_target{ 0u };
				auto const tt_eg = ntk_topo.node_function( ntk_topo.index_to_node( indices[0] ) );
				for ( ; i_target < tt_eg.num_vars(); ++i_target )
				{
					auto const cofact0 = kitty::cofactor0( tt_eg, i_target );
					auto const cofact1 = kitty::cofactor1( tt_eg, i_target );
					if ( kitty::equal( cofact0, ~cofact1 ) )
					{
						uint32_t half = tt_eg.num_bits() / 2;

						for ( uint32_t j{ 0u }; j < tt_eg.num_bits(); ++j )
						{
							if ( j < half )
							{
								os << fmt::format( "{}", kitty::get_bit( cofact0, j ) );
							}
							else
							{
								os << fmt::format( "{}", kitty::get_bit( cofact1, j - half ) );
							}
						}
						os << '\n';
						break;
					}
				}

				for ( uint8_t i{ 1u }; i < indices.size(); ++i )
				{
					auto const tt = ntk_topo.node_function( ntk_topo.index_to_node( indices[i] ) );
					auto const cofact0 = kitty::cofactor0( tt, i_target );
					for ( uint32_t j{ 0u }; j < tt.num_bits(); ++j )
					{
						uint32_t half = tt_eg.num_bits() / 2;
						if ( j < tt.num_bits() / 2 )
						{
							os << fmt::format( "{}", kitty::get_bit( cofact0, j ) );
						}
						else
						{
							os << fmt::format( "{}", !kitty::get_bit( cofact0, j - half ) ? 1 : 0 );
						}
					}
					os << '\n';
				}
				break;
			}
			case MergeType::Normal:
			{
				/* write fan-ins of node */
				for ( auto i{ 0u }; i < 2u; ++i )
				{
					uint32_t current_label_i = current_label[i];
					std::string fanin_name = {};
					if ( ntk_topo.is_pi( ntk_topo.index_to_node( current_label_i ) ) )
					{
						fanin_name = fmt::format( "pi{}", current_label_i );
					}
					else if ( mo_node_names.contains( current_label_i ) )
					{
						fanin_name = mo_node_names.at( current_label_i );
					}
					else
					{
						fanin_name = fmt::format( "n{}", current_label_i );
					}
					os << fanin_name << ' ';
				}

				/* write intermediate result */
				const std::string interm_name = fmt::format( "in{}", ntk_topo.node_to_index( n ) );
				os << interm_name << '\n';
				os << "1 2\n";

				/* write fan-outs */
				os << ".bootstrap " << interm_name << " ";
				std::vector<uint32_t> indices = mergable_luts_map[current_label];
				for ( uint32_t i{ 0u }; i < indices.size(); ++i )
				{
					const std::string fanout_name = fmt::format( "n{}", ntk_topo.index_to_node( indices[i] ) );
					os << fanout_name;
					if ( i != indices.size() - 1u )
					{
						os << ' ';
					}
					// names.insert( fanout_name );
				}
				os << '\n';

				/* write (projected) truth table */
				for ( uint32_t const& index : indices )
				{
					auto const tt = ntk_topo.node_function( ntk_topo.index_to_node( index ) );
					for ( uint32_t i{ 0u }; i < tt.num_bits(); ++i )
					{
						os << fmt::format( "{}", kitty::get_bit( tt, i ) );
					}
					os << '\n';
				}
				break;
			}
			default:
			{
				std::cerr << "[e] Encoutered invalid gate type!\n";
				abort();
				break;
			}
			}

			/* update 'mo_node_names' */
			std::vector<uint32_t> mo_nodes = mergable_luts_map[current_label];
			std::string mo_node_name = {};
			for ( auto j{ 0u }; j < mo_nodes.size(); ++j )
			{
				mo_node_name += fmt::format( "n{}", mo_nodes[j] );
				if ( j != mo_nodes.size() - 1u )
				{
					mo_node_name += ',';
				}
			}
			for ( uint32_t each_node : mo_nodes )
			{
				mo_node_names.emplace( each_node, mo_node_name );
			}
		}

		return true;
	} );
	
	/* connect to POs */
	// ntk_topo.foreach_po( [&]( klut_network::signal const& f, uint32_t index ) {
	// 	os << ".lincomb ";
	// 	klut_network::node nf = ntk_topo.get_node( f );
	// 	const std::string fanin_name = ntk_topo.is_pi( nf ) ? fmt::format( "pi{}", ntk_topo.node_to_index( nf ) ) : fmt::format( "n{}", ntk_topo.node_to_index( nf ) );
	// 	os << fanin_name << ' ';
	// 	const std::string fanout_name = fmt::format( "po{}", index );
	// 	os << fanout_name << '\n';
	// 	os << "1\n";
	// } );

	os << ".end\n";
}

}