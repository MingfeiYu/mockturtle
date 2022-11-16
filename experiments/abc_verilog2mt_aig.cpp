#include <string>
#include <array>
#include <iostream>
#include <fmt/format.h>

static const std::string MPC_benchmarks[] = {
  "auction_N_2_W_16", "auction_N_2_W_32", "auction_N_3_W_16", "auction_N_3_W_32", "auction_N_4_W_16", "auction_N_4_W_32", 
  "knn_comb_K_1_N_8", "knn_comb_K_1_N_16", "knn_comb_K_2_N_8", "knn_comb_K_2_N_16", "knn_comb_K_3_N_8", "knn_comb_K_3_N_16", 
  "voting_N_1_M_3", "voting_N_1_M_4", "voting_N_2_M_2", "voting_N_2_M_3", "voting_N_2_M_4", "voting_N_3_M_4", 
  "stable_matching_comb_Ks_4_S_8", "stable_matching_comb_Ks_8_S_8"};

std::vector<std::string> mpc_benchmarks()
{
	std::vector<std::string> result;
	for ( auto i = 0u; i < 20u; ++i )
	{
		result.emplace_back( MPC_benchmarks[i] );
	}

	return result;
}

std::string benchmark_path_verilog( std::string const& benchmark_name )
{
	return fmt::format( "../experiments/mpc_benchmarks/{}.v", benchmark_name );
}

std::string benchmark_path_aig( std::string const& benchmark_name )
{
	return fmt::format( "../experiments/mpc_benchmarks/{}.aig", benchmark_name );
}

void verilog2aig( std::string const& benchmark )
{
	std::string abc_path = "/Users/myu/Documents/GitHub/abc/";
	std::string command = fmt::format( "{}abc -q \"read {}; strash; write {}\"", abc_path, benchmark_path_verilog( benchmark ), benchmark_path_aig( benchmark ) );

	//std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype( &pclose )> pipe( popen( command.c_str(), "r" ), pclose );

	if ( !pipe )
	{
		throw std::runtime_error( "popen() failed" );
	}
}

int main()
{	
	auto const benchmarks = mpc_benchmarks();
	for ( auto const& benchmark: benchmarks )
	{
		std::cout << "[i] processing " << benchmark << std::endl;

		verilog2aig( benchmark );
	}
	
	return 0;
}