#include <fmt/format.h>
#include <lorina/verilog.hpp>
#include <mockturtle/algorithms/xag2lbf.hpp>
#include <mockturtle/io/verilog_reader.hpp>

#include <experiments.hpp>


std::string bench_path( std::string const& benchmark_name )
{
  return fmt::format( "{}BEST_RESULTS/EPFL/{}.v", EXPERIMENTS_PATH, benchmark_name );
}

int main()
{
	using namespace mockturtle;
	using namespace experiments;
	bool beta{ false };

	for ( auto const& benchmark : epfl_benchmarks() )
	{
		xag_network ntk;
		if ( lorina::read_verilog( bench_path( benchmark ) , verilog_reader( ntk ) ) != lorina::return_code::success )
		{
			continue;
		}
		fmt::print( "[i] processing {}\n", benchmark );

		node_map<bool, xag_network> shall_be_negated( ntk, false );
		mvfbs_map_t mvfbs_map;
		mvfbs_map.clear();

		std::string filename = fmt::format( "results_lbf/xag/{}.lbf", benchmark );
		if ( beta )
		{
			shall_be_negated_init( ntk, shall_be_negated );
			xag2lbf_beta( ntk, shall_be_negated, filename );
		}
		else
		{
			detect_mvfbs( ntk, mvfbs_map, shall_be_negated );
			xag2lbf( ntk, mvfbs_map, shall_be_negated, filename );
		}
	}

	/* TODO: new node type in the estimator! */

	return 0;
}