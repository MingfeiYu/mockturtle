#include <string>
#include <array>
#include <iostream>
#include <fmt/format.h>
#include <mockturtle/networks/aig.hpp>

#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/io/verilog_reader.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/io/write_aiger.hpp>

static const std::string EPFL_benchmarks[] = {
	"adder", "bar", "div", "log2", "max", "multiplier", "sin", "sqrt", "square", "arbiter", 
	"cavlc", "ctrl" , "dec", "i2c", "int2float" , "mem_ctrl", "priority", "router", "voter", 
	"hyp"};

static const std::string CRYPTO_benchmarks[] = {
  "adder_32bit_untilsat", "adder_64bit_untilsat", "AES-expanded_untilsat", "AES-non-expanded_unstilsat", 
  "comparator_32bit_signed_lt_untilsat", "comparator_32bit_signed_lteq_untilsat", "comparator_32bit_unsigned_lt_untilsat", "comparator_32bit_unsigned_lteq_untilsat", 
  "DES-expanded_untilsat", "DES-non-expanded_untilsat", "md5_untilsat", "mult_32x32_untilsat", "sha-1_untilsat", 
  "sha-256_untilsat"};

static const std::string MPC_benchmarks[] = {
  "auction_N_2_W_16", "auction_N_2_W_32", "auction_N_3_W_16", "auction_N_3_W_32", "auction_N_4_W_16", "auction_N_4_W_32", 
  "knn_comb_K_1_N_8", "knn_comb_K_1_N_16", "knn_comb_K_2_N_8", "knn_comb_K_2_N_16", "knn_comb_K_3_N_8", "knn_comb_K_3_N_16", 
  "voting_N_1_M_3", "voting_N_1_M_4", "voting_N_2_M_2", "voting_N_2_M_3", "voting_N_2_M_4", "voting_N_3_M_4", 
  "stable_matching_comb_Ks_4_S_8", "stable_matching_comb_Ks_8_S_8"};

std::vector<std::string> epfl_benchmarks()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 20u; ++i )
	{
		result.emplace_back( EPFL_benchmarks[i] );
	}

	return result;
}

std::vector<std::string> crypto_benchmarks()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 14u; ++i )
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

std::string benchmark_path_verilog( uint32_t benchmark_type, std::string const& benchmark_name, bool opt )
{
	switch( benchmark_type )
	{
	case 0u:
		return fmt::format( "../experiments/{}/{}.v", opt ? "epfl_opt" : "epfl_benchmarks", benchmark_name );
	case 1u:
		return fmt::format( "../experiments/{}/{}.v", opt ? "crypto_opt" : "crypto_benchmarks", benchmark_name );
	case 2u:
		return fmt::format( "../experiments/{}/{}.v", opt ? "mpc_opt" : "mpc_benchmarks", benchmark_name );
	default:
		std::cout << "Unspecified type of benchmark. \n";
		abort();
	}
}

std::string benchmark_path_aig( uint32_t benchmark_type, std::string const& benchmark_name, bool opt )
{
	switch( benchmark_type )
	{
	case 0u:
		return fmt::format( "../experiments/{}/{}.aig", ( opt ? "epfl_opt" : "epfl_benchmarks" ), benchmark_name );
	case 1u:
		return fmt::format( "../experiments/{}/{}.aig", ( opt ? "crypto_opt" : "crypto_benchmarks" ), benchmark_name );
	case 2u:
		return fmt::format( "../experiments/{}/{}.aig", ( opt ? "mpc_opt" : "mpc_benchmarks" ), benchmark_name );
	default:
		std::cout << "Unspecified type of benchmark. \n";
		abort();
	}
}

void aig2verilog( uint32_t benchmark_type, std::string const& benchmark, bool opt )
{
	mockturtle::aig_network aig;
	auto const read_result = lorina::read_aiger( benchmark_path_aig( benchmark_type, benchmark, opt ), mockturtle::aiger_reader( aig ) );
	assert( read_result == lorina::return_code::success );

	mockturtle::write_verilog( aig, benchmark_path_verilog( benchmark_type, benchmark, opt ) );
}

void verilog2aig( uint32_t benchmark_type, std::string const& benchmark, bool opt )
{
	std::string abc_path = "/Users/myu/Documents/GitHub/abc/";
	std::string command = fmt::format( "{}abc -q \"read {}; strash; write {}\"", abc_path, benchmark_path_verilog( benchmark_type, benchmark, opt ), benchmark_path_aig( benchmark_type, benchmark, opt ) );
	std::unique_ptr<FILE, decltype( &pclose )> pipe( popen( command.c_str(), "r" ), pclose );

	if ( !pipe )
	{
		throw std::runtime_error( "popen() failed" );
	}
}

int main()
{	
	bool opt = false;
	do {
		for ( auto i = 0u; i < 3u; ++i )
		{
			auto const benchmarks = ( i > 0u ) ? ( ( i > 1u ) ? mpc_benchmarks() : crypto_benchmarks() ) : epfl_benchmarks();
			for ( auto const& benchmark: benchmarks )
			{
				std::cout << "[i] processing " << ( opt ? "optimized " : "" ) << benchmark << std::endl;
				verilog2aig( i, benchmark, opt );
			}
		}
		opt = !opt;
	} while ( opt );
	
	return 0;
}