#include <mockturtle/algorithms/collect_popular_functions.hpp>

int main()
{
	for ( auto i{ 2u }; i <= 2u; ++i )
	{
		std::cout << "[i] Working on " << ( i ? ( ( i == 1u ) ? "crypto" : "mpc" ) : "epfl" ) << " benchmarks\n";
		mockturtle::collect_popular_functions run{ "../experiments/db_mc", i };
	}
	return 0;
}