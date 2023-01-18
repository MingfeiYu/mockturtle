#include <iostream>
#include <string>
#include <fmt/format.h>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/io/verilog_reader.hpp>
#include <mockturtle/io/write_bench.hpp>
#include <mockturtle/io/write_dot.hpp>

std::string benchmark_path()
{
	return "../experiment/epfl_benchmark/i2c_debug.v";
}

template<class Ntk>
bool abc_cec( Ntk const& ntk )
{
	mockturtle::write_bench( ntk, "/tmp/test.bench" );
	std::cout << "Here\n";
	std::string abc_path = "/Users/myu/Documents/GitHub/abc/";
	std::string command = fmt::format( "{}abc -q \"cec -n {} /tmp/test.bench\"", abc_path, benchmark_path() );

	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype( &pclose )> pipe( popen( command.c_str(), "r" ), pclose );

	if ( !pipe )
	{
		throw std::runtime_error( "popen() failed" );
	}

	while ( fgets( buffer.data(), buffer.size(), pipe.get() ) != nullptr )
	{
		result += buffer.data();
	}

	return result.size() >= 23 && result.substr( 0u, 23u ) == "Networks are equivalent";
}

int main( void )
{
	mockturtle::xag_network xag;
	auto const read_result = lorina::read_verilog( benchmark_path(), mockturtle::verilog_reader( xag ) );
	auto const cec = abc_cec( xag );
	assert ( cec );

	return 0;
}
