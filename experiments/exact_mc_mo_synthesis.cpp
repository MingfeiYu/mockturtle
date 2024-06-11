/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2022  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// #if defined( BILL_HAS_Z3 )

#include "experiments.hpp"

#include <bill/sat/interface/abc_bsat2.hpp>
// #include <bill/sat/interface/z3.hpp>
#include <fmt/format.h>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/print.hpp>
#include <mockturtle/algorithms/detail/minmc_xags.hpp>
#include <mockturtle/algorithms/exact_mc_mo_synthesis.hpp>
// #include <mockturtle/algorithms/xag_optimization.hpp>
// #include <mockturtle/io/index_list.hpp>
// #include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/properties/mccost.hpp>
#include <mockturtle/utils/progress_bar.hpp>

#include <string>


void n_choose_r_rec( uint8_t num_to_choose, uint32_t min_to_choose, uint32_t max_to_choose, std::vector<uint32_t>& solution_tmp, std::vector<std::vector<uint32_t>>& solutions )
{
  if ( !num_to_choose )
  {
    solutions.emplace_back( solution_tmp );
    return;
  }

  for ( uint32_t next_to_choose{ min_to_choose }; next_to_choose <= max_to_choose; ++next_to_choose )
  {
    solution_tmp.emplace_back( next_to_choose );
    n_choose_r_rec( num_to_choose - 1, next_to_choose + 1, max_to_choose, solution_tmp, solutions );
    solution_tmp.pop_back();
  }
}

std::vector<std::vector<uint32_t>> n_choose_r( uint32_t n, uint32_t r )
{
  std::vector<std::vector<uint32_t>> solutions;
  std::vector<uint32_t> solution_tmp;
  n_choose_r_rec( r, 2u, n, solution_tmp, solutions );

  return solutions;
}

int main()
{
  using namespace experiments;
  using namespace mockturtle;

  uint32_t num_var{ 3u };
  uint32_t num_po{ 1u };
  for ( uint32_t i{ 2u }; i <= num_po; ++i )
  {
    // experiment<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, double> exp( "exact_mc_mo_synthesis", "variables", "outputs", "index", "AND gates", "AND gates (base)", "synthesis time" );

    const auto exact_mc_mo_syn = [&]( uint32_t num_vars, uint32_t num_pos ) {
      stopwatch<>::duration time{};
      uint32_t num_funcs{ static_cast<uint32_t>( mockturtle::detail::minmc_xags[num_vars].size() ) };
      const auto prefix = fmt::format( "exact_mc_mo_synthesis_{}_choose_{}", num_funcs, num_pos );
      std::vector<std::vector<uint32_t>> pos_selected = n_choose_r( num_funcs, num_pos );
      
      progress_bar pbar( pos_selected.size(), prefix + " |{}| time so far = {:.2f}", true );
      uint32_t cnt{ 0u };
      for ( auto const& pos_sel : pos_selected )
      {
        stopwatch<> t( time );
        std::vector<kitty::dynamic_truth_table> funcs( num_pos );
        uint32_t ctr{ 0u };
        for ( uint32_t ind : pos_sel )
        {
          kitty::dynamic_truth_table tt( num_vars );
          uint64_t word = std::get<1>( mockturtle::detail::minmc_xags[num_vars][ind - 1] );
          if ( word == 0u )
          {
            continue;
          }
          kitty::create_from_words( tt, &word, &word + 1 );
          funcs[ctr++] = tt;
        }
        pbar( cnt, to_seconds( time ) );

        exact_mc_mo_synthesis_params ps;
        ps.conflict_limit = 0u ;
        ps.verbose = false;

        auto res = exact_mc_mo_synthesis<xag_network, bill::solvers::bsat2>( funcs, ps );
        uint32_t num_ands_base{ 0u };
        for ( auto const& func : funcs )
        {
          num_ands_base += kitty::get_mc( func );
        }
        if ( res )
        {
          const uint32_t num_ands = *multiplicative_complexity( *res );
          // exp( num_vars, num_pos, cnt++, num_ands, num_ands_base, to_seconds( time ) );
        }
        else
        {
          // exp( num_vars, num_pos, cnt++, num_ands_base, num_ands_base, to_seconds( time ) );
        }
      }

      /* print out tested combinations */
      std::cout << "\n[m] " << pos_selected.size() << " combinations:\n";
      uint32_t ind{ 0u };
      for ( auto const& pos_sel : pos_selected )
      {
        std::cout << ind++ << ": { ";
        for ( auto const& po_sel : pos_sel )
        {
          std::cout << po_sel << " ";
        }
        std::cout << "}\n";
      }
    };

    exact_mc_mo_syn( num_var, i );

    // exp.save();
    // exp.table();
  }

  std::vector<uint64_t> sampled( ( 1 << ( ( 1 << num_var ) - 1 ) ) - 1 );
  for ( auto i{ 1u }; i <= sampled.size(); ++i )
  {
    sampled[i - 1] = i;
  }
  std::vector<kitty::dynamic_truth_table> funcs( sampled.size() );
  uint32_t ctr{ 0u };
  for ( auto i{ 0u }; i < sampled.size(); ++i )
  {
    kitty::dynamic_truth_table tt( num_var );
    uint64_t word = sampled[i] << 1;
    if ( word == 0u )
    {
      continue;
    }
    kitty::create_from_words( tt, &word, &word + 1 );
    funcs[ctr++] = tt;
  }
  exact_mc_mo_synthesis_params ps;
  ps.conflict_limit = 0u ;
  ps.verbose = false;

  auto res = exact_mc_mo_synthesis<xag_network, bill::solvers::bsat2>( funcs, ps );
  uint32_t num_ands_base{ 0u };
  for ( auto const& func : funcs )
  {
    num_ands_base += kitty::get_mc( func );
  }
  std::cout << "[i] Upper bound on sum of AND count: " << num_ands_base << std::endl;
  if ( res )
  {
    const uint32_t num_ands = *multiplicative_complexity( *res );
    std::cout << "[i] Actual sum of AND count: " << num_ands << std::endl;
  }

  std::cout << "[i] Number of functions: " << funcs.size() << "\n";

  return 0;
}