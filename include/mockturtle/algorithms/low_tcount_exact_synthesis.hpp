#pragma once

#include <cstdint>
#include <fmt/format.h>
#include <optional>
#include <vector>
#include <iostream>

#include <bill/sat/interface/common.hpp>
#include <bill/sat/interface/glucose.hpp>
#include <kitty/bit_operations.hpp>
#include <kitty/constructors.hpp>
#include <kitty/operations.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/generators/sorting.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/utils/progress_bar.hpp>
#include <mockturtle/utils/stopwatch.hpp>
#include <mockturtle/views/cnf_view.hpp>

namespace mockturtle
{

struct low_tcount_exact_synthesis_params
{
  bool no_constant_linear_fanin{ false };
  bool break_commutativity{ false };
  bool break_symmetric_variables{ false };
  bool use_all_gates_variables{ true };
  bool no_subset_linear_fanin{ false };
  bool multilevel_subset_relation{ false };
  bool forbid_po_xor{ false };
  std::optional<uint32_t> expected_num_xor{};
  uint32_t conflict_limit{ 500000u };
  std::optional<std::string> write_dimacs{};
  bool verbose{ false };
  bool verify_solution{ false };
};

struct low_tcount_exact_synthesis_stats
{
  stopwatch<>::duration time_total{};
  stopwatch<>::duration time_solving{}; 
  uint32_t num_vars{};
  uint32_t num_clauses{};

  void report() const
  {
    fmt::print( "[i] total time    = {:>5.2f} secs\n", to_seconds( time_total ) );
    fmt::print( "[i] solving time  = {:>5.2f} secs\n", to_seconds( time_solving ) );
    fmt::print( "[i] total vars    = {}\n", num_vars );
    fmt::print( "[i] total clauses = {}\n", num_clauses );
  }
};

namespace detail
{

template<class Ntk, bill::solvers Solver>
struct low_tcount_exact_synthesis_impl
{
  using problem_network_t = cnf_view<xag_network, false, Solver>;

public:
  low_tcount_exact_synthesis_impl( kitty::dynamic_truth_table const& func, std::vector<uint32_t> num_ands, 
                                   low_tcount_exact_synthesis_params const& ps, low_tcount_exact_synthesis_stats& st )
    : num_vars_( func.num_vars() ), 
      func_( kitty::get_bit( func, 0 ) ? ~func : func ), 
      invert_( kitty::get_bit( func, 0 ) ), 
      num_ands_( num_ands ), 
      expected_num_xor_( ps.expected_num_xor ), 
      ps_( ps ), 
      st_( st )
  {
  }

  std::optional<Ntk> run()
  {
    stopwatch<> time_total( st_.time_total );

    if ( expected_num_xor_ )
    {
      Ntk sol;
      bool first_trial{ true }; 
      
      while ( true )
      {
        cnf_view_params cvps;
        cvps.write_dimacs = ps_.write_dimacs;
        problem_network_t pntk( cvps );
        reset( pntk );

        encode( pntk );
        add_xor_counter( pntk );

        if ( const auto sat = solve_xor_opt( pntk ); sat )
        {
          sol = extract_network( pntk );
          first_trial = false;

          if ( ps_.verbose )
          {
            std::cout << "[i] managed to find an implementation using " << count_xors( pntk ) << " XORs\n";
          }
        }
        else
        {
          if ( !first_trial )
          {
            return sol;
          }
          break;
        }
      }
    }

    else 
    {
      cnf_view_params cvps;
      cvps.write_dimacs = ps_.write_dimacs;
      problem_network_t pntk( cvps );
      reset( pntk );

      encode( pntk );

      if ( const auto sat = solve( pntk ); sat )
      {
        const auto sol = extract_network( pntk );
        return sol;
      }
    }

    return std::nullopt;
  }

private:
  void reset( problem_network_t const& pntk )
  {
    ltfi_vars_.clear();
    truth_vars_.clear();
    truth_vars_.resize( func_.num_bits() );

    for ( auto i{ 0u }; i < num_vars_; ++i )
    {
      const auto var_tt = kitty::nth_var<kitty::dynamic_truth_table>( num_vars_, i );
      for ( auto bit{ 0u }; bit < func_.num_bits(); ++bit )
      {
        truth_vars_[bit].emplace_back( pntk.get_constant( kitty::get_bit( var_tt, bit ) ) );
      }
    }
  }

private:
  void add_gate( problem_network_t& pntk )
  {
    for ( auto i{ 0u }; i < num_ands_.size(); ++i )
    {
      for ( auto i_sub{ 0u }; i_sub <= num_ands_[i]; ++i_sub )
      {
        ltfi_vars_.emplace_back( std::vector<signal<problem_network_t>>( num_vars_ + i ) );
        std::generate( ltfi_vars_.back().begin(), ltfi_vars_.back().end(), [&]() { return pntk.create_pi(); } );
      }
    }
  }

  void add_output( problem_network_t& pntk )
  {
    ltfi_vars_.emplace_back( std::vector<signal<problem_network_t>>( num_vars_ + num_ands_.size() ) );
    std::generate( ltfi_vars_.back().begin(), ltfi_vars_.back().end(), [&]() { return pntk.create_pi(); } );
  }

  void add_func_constraint( problem_network_t& pntk )
  {
    const auto create_xor_clause = [&]( std::vector<signal<problem_network_t>> const& ltfi_vars, uint32_t bit ) -> signal<problem_network_t> {
      std::vector<signal<problem_network_t>> ltfi( ltfi_vars.size() );
      for ( auto j{ 0u }; j < ltfi.size(); ++j )
      {
        ltfi[j] = pntk.create_and( ltfi_vars[j], truth_vars_[bit][j] );
      }
      return pntk.create_nary_xor( ltfi );
    };
    for ( auto bit{ 1u }; bit < func_.num_bits(); ++bit )
    {
      uint32_t fanin_index = 0u;
      for ( auto num_and: num_ands_ )
      {
        std::vector<signal<problem_network_t>> abstract_xors;
        for ( auto i_sub{ 0u }; i_sub <= num_and; ++i_sub )
        {
          abstract_xors.emplace_back( create_xor_clause( ltfi_vars_[fanin_index++], bit ) );
        }
        truth_vars_[bit].emplace_back( pntk.create_nary_and( abstract_xors ) );
      }
      assert( ( fanin_index + 1 ) == ltfi_vars_.size() );

      const auto po_signal = create_xor_clause( ltfi_vars_.back(), bit );
      pntk.create_po( kitty::get_bit( func_, bit ) ? po_signal : pntk.create_not( po_signal ) );
    }
  }

  void add_struct_constraint( problem_network_t& pntk )
  {
    /* As long as an AND connects to an XOR, then:         */
    /* either (1) the XOR has more than one fanin;         */
    /* or (2) the AND has fanout size larger than 1.       */

    uint32_t fanin_index_i1{ 0u };

    for ( auto i1{ 0u }; i1 < num_ands_.size() - 1u; ++i1 )
    {
      uint32_t fanin_index_i2{ fanin_index_i1 + num_ands_[i1] + 1u };
      for ( auto i2{ i1 + 1u }; i2 < num_ands_.size(); ++i2 )
      {
        for ( auto i2_sub{ 0u }; i2_sub <= num_ands_[i2]; ++i2_sub )
        {
          uint32_t gate_index_i1 = num_vars_ + i1;
          std::vector<signal<problem_network_t>> clause;
          clause.emplace_back( pntk.create_not( ltfi_vars_[fanin_index_i2][gate_index_i1] ) );

          for ( auto j{ 0u }; j < ltfi_vars_[fanin_index_i2].size(); ++j )
          {
            if ( j != gate_index_i1 )
            {
              clause.emplace_back( ltfi_vars_[fanin_index_i2][j] );
            }
          }

          for ( auto i{ fanin_index_i1 + num_ands_[i1] + 1u }; i <ltfi_vars_.size(); ++i )
          {
            if ( i != fanin_index_i2 )
            {
              clause.emplace_back( ltfi_vars_[i][gate_index_i1] );
            }
          }

          pntk.add_clause( clause );

          ++fanin_index_i2;
        }
      }
      fanin_index_i1 += ( num_ands_[i1] + 1u );
    }
  }

  /* Extra constraints to prune searching space                                   */
  /* All linear functions should contain at least one variable                    */
  void add_no_constant_linear_fanin_constraint( problem_network_t& pntk )
  {
    for ( auto const& ltfi: ltfi_vars_ )
    {
      pntk.add_clause( ltfi );
    }
  }

  /* Considering the commutativity of AND operation, left-side linear function is */
  /* guaranteed to be lexicographically smaller                                   */
  void add_break_commutativity_constraint( problem_network_t& pntk )
  {
    uint32_t fanin_index{ 0u };
    for ( auto num_and: num_ands_ )
    {
      for ( auto i_sub{ 0u }; i_sub < num_and; ++i_sub )
      {
        auto const& ltfi1 = ltfi_vars_[fanin_index++];
        auto const& ltfi2 = ltfi_vars_[fanin_index];

        auto n = ltfi1.size();
        std::vector<signal<problem_network_t>> a( n - 1u );
        std::generate( a.begin(), a.end(), [&]() { return pntk.create_pi(); } );

        pntk.add_clause( !ltfi1[0], ltfi2[0] );
        pntk.add_clause( !ltfi1[0], a[0] );
        pntk.add_clause( ltfi2[0], a[0] );
        for ( auto i{ 1u }; i < n - 1u; ++i )
        {
          pntk.add_clause( !ltfi1[i], ltfi2[i], !a[i - 1] );
          pntk.add_clause( !ltfi1[i], a[i], !a[i - 1] );
          pntk.add_clause( ltfi2[i], a[i], !a[i - 1] );
        }
        pntk.add_clause( !ltfi1.back(), !a.back() );
        pntk.add_clause( ltfi2.back(), !a.back() );
      }
    }
  }

  /* If two variables(pis) are symmetric, make sure the lexicographically smaller */
  /* one is used first                                                            */
  void add_break_symmetric_variables_constraint( problem_network_t& pntk )
  {
    const auto break_symmetric_vars = [&]( auto j1, auto j2 ) {
      for ( auto i2{ 0u }; i2 < ltfi_vars_.size(); ++i2 )
      {
        std::vector<signal<problem_network_t>> clause;
        
        clause.emplace_back( !ltfi_vars_[i2][j2] );
        for ( auto i1{ 0u }; i1 <= i2; ++i1 )
        {
          clause.emplace_back( ltfi_vars_[i1][j1] );
        }

        pntk.add_clause( clause );
      }
    };

    for ( auto j2{ 1u }; j2 < num_vars_; ++j2 )
    {
      for ( auto j1{ 0u }; j1 < j2; ++j1 )
      {
        if ( kitty::is_symmetric_in( func_, j1, j2 ) )
        {
          break_symmetric_vars( j1, j2 );
        }
      }
    }
  }

  /* Make sure that all gates and non-trivial variables are used                  */
  void add_use_all_gates_variables_constraint( problem_network_t& pntk )
  {
    for ( auto j{ 0u }; j < ltfi_vars_.back().size(); ++j )
    {
      if ( j < num_vars_ && !kitty::has_var( func_, j ) )
      {
        /* Skip trivial variable */
        continue;
      }

      std::vector<signal<problem_network_t>> clause;
      for ( auto const& ltfi: ltfi_vars_ )
      {
        if ( j < ltfi.size() )
        {
          clause.emplace_back( ltfi[j] );
        }
      }
      pntk.add_clause( clause );
    }
  }

  /* Make sure that no set of linear fanin is the subset of any other set of      */
  /* fanin                                                                        */
  void add_no_subset_linear_fanin_constraint( problem_network_t& pntk )
  {
    uint32_t gate_index{ 0u };

    for ( auto num_and: num_ands_ )
    {
      for ( auto i_sub1{ 0u }; i_sub1 < num_and; ++i_sub1 )
      {
        for ( auto i_sub2{ i_sub1 + 1u }; i_sub2 <= num_and; ++i_sub2 )
        {
          auto const& ltfi1 = ltfi_vars_[gate_index + i_sub1];
          auto const& ltfi2 = ltfi_vars_[gate_index + i_sub2];

          std::vector<signal<problem_network_t>> clause1( ltfi1.size() );
          std::vector<signal<problem_network_t>> clause2( ltfi1.size() );

          for ( auto j{ 0u }; j < ltfi1.size(); ++j )
          {
            clause1[j] = pntk.create_and( ltfi1[j], pntk.create_not( ltfi2[j] ) );
            clause2[j] = pntk.create_and( ltfi2[j], pntk.create_not( ltfi1[j] ) );
          }

          pntk.add_clause( clause1 );
          pntk.add_clause( clause2 );
        }
      }
      gate_index += ( num_and + 1u );
    }
  }

  /* Rule out subset relations among linear functions to different ANDs           */
  void add_multilevel_subset_relation_constraint( problem_network_t& pntk )
  {
    for ( auto l{ 0u }; l < ltfi_vars_.size(); ++l )
    {
      uint32_t gate_index{ 0u };
      auto const& ltfi = ltfi_vars_[l];

      for ( auto num_and: num_ands_ )
      {
        const auto n = ltfi_vars_[gate_index].size();
        std::vector<signal<problem_network_t>> clause1, clause2;

        clause1.emplace_back( ltfi[num_vars_ + gate_index] );
        for ( auto j{ 0u }; j < n; ++j )
        {
          std::vector<signal<problem_network_t>> clause2_sub;
          for ( auto i_sub{ 0u }; i_sub <= num_and; ++i_sub )
          {
            clause1.emplace_back( pntk.create_or( !ltfi[j], ltfi_vars_[gate_index + i_sub][j] ) );
            clause2_sub.emplace_back( ltfi_vars_[gate_index + i_sub][j] );
          }
          clause2.emplace_back( pntk.create_xnor( ltfi[j], pntk.create_nary_and( clause2_sub ) ) );
        }
        pntk.create_po( pntk.create_or( !pntk.create_nary_and( clause1 ), pntk.create_nary_and( clause2 ) ) );

        gate_index += ( num_and + 1u );
      }
    }
  }

  /* Aggressive constraints for better solution (may lead to UNSAT) */
  void add_forbid_po_xor_constraint( problem_network_t& pntk )
  {
    auto ltfi = ltfi_vars_.back();
    const auto n = ltfi.size();
    std::vector<signal<problem_network_t>> clause;

    for ( auto i{ 0u }; i < ( n - 1u ); ++i )
    {
      clause.emplace_back( pntk.create_not( ltfi[i] ) );
    }
    clause.emplace_back( ltfi.back() );

    pntk.create_po( pntk.create_nary_and( clause ) );
  }


  void encode( problem_network_t& pntk )
  {
    /* Add essential constraints                      */
    add_gate( pntk );
    add_output( pntk );
    add_func_constraint( pntk );
    add_struct_constraint( pntk );

    /* Add extra constraints to prune searching space */
    if ( ps_.no_constant_linear_fanin )
    {
      add_no_constant_linear_fanin_constraint( pntk );
    }

    if ( ps_.break_commutativity )
    {
      add_break_commutativity_constraint( pntk );
    }

    if ( ps_.break_symmetric_variables )
    {
      add_break_symmetric_variables_constraint( pntk );
    }

    if ( ps_.use_all_gates_variables )
    {
      add_use_all_gates_variables_constraint( pntk );
    }

    if ( ps_.no_subset_linear_fanin )
    {
      add_no_subset_linear_fanin_constraint( pntk );
    }

    if ( ps_.multilevel_subset_relation )
    {
      add_multilevel_subset_relation_constraint( pntk );
    }

    /* Add extra constraints for preferable solution  */

    if (ps_.forbid_po_xor)
    {
      add_forbid_po_xor_constraint( pntk );
    }

    st_.num_vars += pntk.num_vars();
    st_.num_clauses += pntk.num_clauses();
  }

  void add_xor_counter( problem_network_t& pntk )
  {
    xor_counter_.clear();
    for ( auto const& ltfi : ltfi_vars_ )
    {
      std::copy( ltfi.begin(), ltfi.end(), std::back_inserter( xor_counter_ ) );
    }

    insertion_sorting_network( static_cast<uint32_t>( xor_counter_.size() ), [&]( auto a, auto b ) {
      auto const aa = pntk.create_and( xor_counter_[a], xor_counter_[b] );
      auto const bb = pntk.create_or( xor_counter_[a], xor_counter_[b] );
      xor_counter_[a] = aa;
      xor_counter_[b] = bb;
    } );
  }

private:
  bool solve( problem_network_t& pntk )
  {
    stopwatch<> time_sol( st_.time_solving );
    bill::result::clause_type assumptions;

    pntk.foreach_po( [&]( auto const& f ) {
      assumptions.emplace_back( pntk.lit( f ) );
    } );

    const auto res = pntk.solve( assumptions, ps_.conflict_limit );

    if ( res && *res )
    {
      return true;
    }

    return false;
  }

  bool solve_xor_opt( problem_network_t& pntk )
  {
    stopwatch<> time_sol( st_.time_solving );
    bill::result::clause_type assumptions;
    
    pntk.foreach_po( [&]( auto const& f ) {
      assumptions.push_back( pntk.lit( f ) );
    } );
    if ( int32_t pos = static_cast<int32_t>( xor_counter_.size() ) - *expected_num_xor_ - 1; pos >= 0 )
    {
      assumptions.push_back( pntk.lit( !xor_counter_[pos] ) );
    }

    const auto res = pntk.solve( assumptions, 0u );

    if ( res && *res )
    {
      --( *expected_num_xor_ );
      return true;
    }

    return false;
  }

  Ntk extract_network( problem_network_t& pntk )
  {
    Ntk ntk;
    std::vector<signal<Ntk>> nodes( num_vars_ );
    std::generate( nodes.begin(), nodes.end(), [&]() { return ntk.create_pi(); } );

    const auto extract_ltfi = [&]( std::vector<signal<problem_network_t>> const& ltfi_vars ) ->signal<Ntk> {
      std::vector<signal<Ntk>> ltfi;
      for ( auto j{ 0u }; j < ltfi_vars.size(); ++j )
      {
        if ( pntk.model_value( ltfi_vars[j] ) )
        {
          ltfi.emplace_back( nodes[j] );
          //std::cout << ( nodes[j].complement ? "!" : "" ) << nodes[j].index << " ";
        }
      }
      //std::cout << "\n";
      return ntk.create_nary_xor( ltfi );
    };

    uint32_t fanin_index{ 0u };

    uint32_t and_ind{ 0u };
    for ( auto num_and: num_ands_ )
    {
      //std::cout << "[m] AND in the " << ++and_ind << " step is " << num_and + 1 << "-input\n";
      std::vector<signal<Ntk>> abstract_xors;

      for ( auto i_sub{ 0u }; i_sub <= num_and; ++i_sub )
      {
        //std::cout << "[m] its " << ( i_sub + 1 ) << " fanin is the XORed result of nodes: ";
        abstract_xors.emplace_back( extract_ltfi( ltfi_vars_[fanin_index++] ) );
      }

      nodes.emplace_back( ntk.create_nary_and( abstract_xors ) );
    }

    //std::cout << "[m] po is the XORed result of nodes: ";
    const auto po = extract_ltfi( ltfi_vars_.back() );
    ntk.create_po( invert_ ? ntk.create_not( po ) : po );

    /* verify the correctness of ntk */
    if ( ps_.verify_solution )
    {
      auto result = simulate<kitty::static_truth_table<5u>>( ntk )[0];
      const std::string result_str = kitty::to_hex( result ).substr( 0, 1 << ( num_vars_ - 2 ) );
      const std::string func_str = kitty::to_hex( invert_ ? ~func_ : func_ );
      if ( result_str != func_str )
      {
        std::cerr << "[e] incorrect implementation for " << func_str << "; "
                  << "whose function is " << result_str << std::endl;
        abort();
      }
      else
      {
        std::cout << "[m] the solution is verified to be correct\n";
      }
    }

    return ntk;
  }

  uint32_t count_xors( problem_network_t& pntk ) const
  {
    uint32_t ctr{};
    for ( auto const& ltfi : ltfi_vars_ )
    {
      for ( auto const& l : ltfi )
      {
        ctr += pntk.model_value( l ) ? 1u : 0u;
      }
    }
    return ctr;
  }

private:
  uint32_t num_vars_;
  std::vector<uint32_t> num_ands_;
  std::vector<std::vector<signal<problem_network_t>>> ltfi_vars_;
  std::vector<std::vector<signal<problem_network_t>>> truth_vars_;
  std::vector<signal<problem_network_t>> xor_counter_;
  kitty::dynamic_truth_table func_;
  bool invert_{false};
  std::optional<uint32_t> expected_num_xor_; 
  low_tcount_exact_synthesis_params const& ps_;
  low_tcount_exact_synthesis_stats& st_;
};

} /* namespace detail */

template<class Ntk = xag_network, bill::solvers Solver = bill::solvers::glucose_41>
std::optional<Ntk> low_tcount_exact_synthesis( kitty::dynamic_truth_table const& func,
                                               std::vector<uint32_t> num_ands,
                                               low_tcount_exact_synthesis_params const& ps = {},
                                               low_tcount_exact_synthesis_stats* pst = nullptr )
{
  low_tcount_exact_synthesis_stats st;

  const auto pntk = detail::low_tcount_exact_synthesis_impl<Ntk, Solver>{ func, num_ands, ps, st }.run();

  if ( ps.verbose )
  {
    st.report();
  }

  if ( pst )
  {
    *pst = st;
  }

  return pntk;
}

} /* namespace mockturtle */