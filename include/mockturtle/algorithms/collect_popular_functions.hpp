#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <kitty/operations.hpp>
#include <kitty/print.hpp>
#include <kitty/spectral.hpp>
#include <mockturtle/algorithms/cut_enumeration.hpp>
#include <mockturtle/io/verilog_reader.hpp>
#include <mockturtle/networks/x1g.hpp>

static const std::string EPFL_benchmarks[] = {
	"adder", "bar", "div", "log2", "max", "multiplier", "sin", "sqrt", "square", "arbiter", 
	"cavlc", "ctrl" , "dec", "i2c", "int2float" , "mem_ctrl", "priority", "router", "voter"};

static const std::string CRYPTO_benchmarks[] = {
  "AES-expanded_untilsat", "AES-non-expanded_untilsat", "DES-expanded_untilsat", "DES-non-expanded_untilsat", 
  "comparator_32bit_signed_lt_untilsat", "comparator_32bit_signed_lteq_untilsat", "comparator_32bit_unsigned_lt_untilsat", "comparator_32bit_unsigned_lteq_untilsat", 
  "md5_untilsat", "sha-1_untilsat", "sha-256_untilsat"};

static const std::string MPC_benchmarks[] = {
  "auction_N_2_W_16", "auction_N_2_W_32", "auction_N_3_W_16", "auction_N_3_W_32", "auction_N_4_W_16", "auction_N_4_W_32", 
  "knn_comb_K_1_N_8", "knn_comb_K_1_N_16", "knn_comb_K_2_N_8", "knn_comb_K_2_N_16", "knn_comb_K_3_N_8", "knn_comb_K_3_N_16", 
  "voting_N_1_M_3", "voting_N_1_M_4", "voting_N_2_M_2", "voting_N_2_M_3", "voting_N_2_M_4", "voting_N_3_M_4", 
  "stable_matching_comb_Ks_4_S_8", "stable_matching_comb_Ks_8_S_8"};

std::vector<std::string> epfl_benchmarks()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 18u; ++i )
	{
		result.emplace_back( EPFL_benchmarks[i] );
	}

	return result;
}

std::vector<std::string> crypto_benchmarks()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 11u; ++i )
	{
		result.emplace_back( CRYPTO_benchmarks[i] );
	}

	return result;
}

std::vector<std::string> mpc_benchmarks()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 20u; ++i )
	{
		result.emplace_back( MPC_benchmarks[i] );
	}

	return result;
}

std::string benchmark_path( uint32_t benchmark_type, std::string const& benchmark_name )
{
	switch( benchmark_type )
	{
	case 0u:
		return fmt::format( "../experiments/epfl_tcad22/{}.v", benchmark_name );
	case 1u:
		return fmt::format( "../experiments/crypto_tcad22/{}.v", benchmark_name );
	case 2u:
		return fmt::format( "../experiments/mpc_opt/{}.v", benchmark_name );
	default:
		std::cout << "Unspecified type of benchmark. \n";
		abort();
	}
}

namespace mockturtle
{

struct cut_enumeration_cut_rewriting_cut
{
  int32_t gain{ -1 };
};

bool sortPopularity( std::pair<std::string, uint32_t> const& func1, std::pair<std::string, uint32_t> const& func2 )
{
	return func1.second > func2.second;
}

class collect_popular_functions
{
public:
	collect_popular_functions( std::string const& db_path, uint32_t benchmark_type )
		: pdb_( std::make_shared<decltype( pdb_ )::element_type>() )
	{
			build_db( db_path );
			collect_functions( db_path, benchmark_type );
	}

private:
	void build_db( std::string const& db_path )
	{
		std::ifstream db;
		db.open( db_path, std::ios::in );
		std::string line;
		uint32_t pos{ 0u };

		while ( std::getline( db, line ) )
		{
			pos = static_cast<uint32_t>( line.find( '\t' ) );
			pos += 1u;
			const auto db_repr_str = line.substr( pos, 16u );
			pos += 17u;
			const auto real_repr_str = line.substr( pos, 16u );
			pos += 17u;

			uint32_t mc = std::stoul( line.substr( pos, 1u ) );
			if ( mc <= 4u )
			{
				continue;
			}

    	pdb_->insert( std::make_pair( real_repr_str, false ) );
		}
		std::cout << "[i] Finished building db\n";
		db.close();
	}

	void collect_functions( std::string const& db_path, uint32_t benchmark_type )
	{
		auto const benchmarks = benchmark_type ? ( ( benchmark_type == 1u ) ? crypto_benchmarks() : mpc_benchmarks() ) : epfl_benchmarks();
		std::unordered_map<std::string, uint32_t> func_counter_;
		for ( auto i{ 0u }; i < benchmarks.size(); ++i )
		//for ( auto i{ 0u }; i <= 0u; ++i )
		{
			auto const benchmark = benchmarks[i];
			mockturtle::x1g_network x1g;
			auto const read_result = lorina::read_verilog( benchmark_path( benchmark_type, benchmark ), mockturtle::verilog_reader( x1g ) );
			if ( read_result != lorina::return_code::success )
			{
				std::cout << "[i] failed to read " << benchmark << std::endl;
				abort();
			}
			cut_enumeration_params cut_enumeration_ps_{};
			cut_enumeration_ps_.cut_size = 6;
			cut_enumeration_ps_.cut_limit = 12;
			//cut_enumeration_ps_.minimize_truth_table = true;

			//uint32_t num_gates{ 0u };
			//x1g.foreach_gate( [&]( auto const& n ) 
			//{
			//	++num_gates;
			//} );
			//std::cout << "[i] " << num_gates << " gates in x1g\n";

			const auto cuts = cut_enumeration<x1g_network, true, cut_enumeration_cut_rewriting_cut>( x1g, cut_enumeration_ps_ );
			x1g.foreach_gate( [&]( auto const& n ) {
				for ( auto& cut: cuts.cuts( x1g.node_to_index( n ) ) )
				{
					const auto func = cuts.truth_table( *cut );
					const auto func_ext = kitty::extend_to<6u>( func );
					const auto spectral = kitty::exact_spectral_canonization_limit( func_ext, 100000 );
					if ( spectral.second )
					{
						const std::string cano_str = kitty::to_hex( spectral.first );
						const auto search = func_counter_.find( cano_str );
						if ( search != func_counter_.end() )
						{
							//std::cout << "[m] found 0x" << cano_str << "\n";
							++( search->second );
						}
						else
						{
							func_counter_.insert( std::make_pair( cano_str, 0u ) );
							//std::cout << "[m] insert 0x" << cano_str << "\n";
						}
					}
					//else
					//{
					//	std::cout << "[e] classification failed\n";
					//}
				}
			} );
		}
		std::cout << "[i] Finished collecting popular functions; There are " << func_counter_.size() << " functions\n";

		std::vector<std::pair<std::string, uint32_t>> func_counter_vec_;
		for ( auto const& func: func_counter_ )
		{
			func_counter_vec_.emplace_back( func );
		}
		std::sort( func_counter_vec_.begin(), func_counter_vec_.end(), sortPopularity );

		uint32_t topn{ 5500u };
		uint32_t top_count{ 0u };
		for ( auto const& func: func_counter_vec_ )
		{
			auto const search = pdb_->find( func.first );
			if ( search != pdb_->end() )
			{
				if ( search->second == false )
				{
					search->second = true;
					++top_count;
					if ( top_count == topn )
					{
						break;
					}
				}
			}
		}
		std::cout << "[i] Selected the most popular " << top_count << " functions\n";


		std::ofstream db_func;
		std::string name_postfix = ( benchmark_type ) ? ( ( benchmark_type == 1u ) ? "_crypto" : "_mpc" ) : "_epfl";
		db_func.open( ( "collected_funcs" + name_postfix ), std::ios::app );
		topn -= 500u;
		top_count = 0u;

		std::ifstream db;
		db.open( db_path, std::ios::in );
		std::string line;
		uint32_t pos{ 0u };
		while ( std::getline( db, line ) )
		{
			pos = static_cast<uint32_t>( line.find( '\t' ) );
			pos += 1u;
			const auto db_repr_str = line.substr( pos, 16u );
			pos += 17u;
			const auto real_repr_str = line.substr( pos, 16u );
			pos += 17u;
			uint32_t mc = std::stoul( line.substr( pos, 1u ) );
			if ( mc <= 4u )
			{
				continue;
			}
			auto const search = pdb_->find( real_repr_str );
			if ( search != pdb_->end() && search->second )
			{
				++top_count;
				db_func << line;
				db_func << "\n";
				if ( top_count == topn )
				{
					break;
				}
			}
		}
		db.close();
		db_func.close();
		std::cout << "[i] Stored these functions into a separated db\n";
	}

private:
	std::shared_ptr<std::unordered_map<std::string, bool>> pdb_;
};

} /* namespace mockturtle */