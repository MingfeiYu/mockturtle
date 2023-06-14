#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <fmt/format.h>

#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/cut_enumeration.hpp>
#include <mockturtle/algorithms/cut_enumeration/rewrite_cut.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/utils/cost_functions.hpp>
#include <mockturtle/utils/progress_bar.hpp>
#include <mockturtle/views/fanout_view.hpp>

namespace mockturtle
{

struct rewrite_params
{
  rewrite_params()
  {
    cut_enumeration_ps.cut_size = 6;
    cut_enumeration_ps.cut_limit = 12;
    cut_enumeration_ps.minimize_truth_table = true;
  }

  cut_enumeration_params cut_enumeration_ps{};
  bool allow_zero_gain{ false };
  bool progress{ false };
  bool verbose{ false };
  uint32_t min_cand_cut_size{ 3u };
};

struct rewrite_stats
{
  stopwatch<>::duration time_total{ 0 };
  stopwatch<>::duration time_rewriting{ 0 };

  void report() const
  {
    fmt::print( "[i] total time     = {:>5.2f} secs\n", to_seconds( time_total ) );
    fmt::print( "[i] rewriting time = {:>5.2f} secs\n", to_seconds( time_rewriting ) );
  }
};

namespace detail
{

template<class Ntk, class RewritingFn, class NodeCostFn>
struct rewrite_impl
{
	static constexpr uint32_t num_vars = 5u;
  using network_cuts_t = dynamic_network_cuts<Ntk, num_vars, true, cut_enumeration_rewrite_cut>;
  using cut_manager_t = detail::dynamic_cut_enumeration_impl<Ntk, num_vars, true, cut_enumeration_rewrite_cut>;
  using cut_t = typename network_cuts_t::cut_t;

public:
	rewrite_impl( Ntk const& ntk, RewritingFn const& rewriting_fn, rewrite_params const& ps, rewrite_stats& st )
    : ntk_( ntk ),
      rewriting_fn_( rewriting_fn ),
      ps_( ps ),
      st_( st ) {}

  Ntk run()
  {
  	stopwatch t_total( st_.time_total );
  	fanout_view<Ntk> ntk_fanout_{ ntk_ };

  	cut_enumeration_stats cut_enum_st;
    network_cuts_t cuts( ntk_.size() + ( ntk_.size() >> 1 ) );
    cut_manager_t cut_manager( ntk_, ps_.cut_enumeration_ps, cut_enum_st, cuts );
    cut_manager.init_cuts();

    std::array<signal<Ntk>, num_vars> best_leaves;
    //std::vector<signal<Ntk>> best_leaves( num_vars );
    const auto orig_cost = costs<Ntk, NodeCostFn>( ntk_ );

    progress_bar pbar{ ntk_.num_gates(), "rewriting |{0}| node = {1:>4} / " + std::to_string( ntk_.num_gates() ) + "   original cost = " + std::to_string( orig_cost ), ps_.progress };
    ntk_.foreach_gate( [&]( auto const& n, auto i ) {
    	//std::cout << "\n[i] working on node " << n << "\n";
    	pbar( i, i );

    	if ( ntk_.fanout_size( n ) == 0u )
    	{
        return;
    	}

      cut_manager.clear_cuts( n );
      cut_manager.compute_cuts( n );

      uint32_t cut_index = 0;
      int32_t best_gain = -1;
      uint32_t best_cut_index{ 0u };

      for ( auto& cut : cuts.cuts( ntk_.node_to_index( n ) ) )
      {
      	++cut_index;
      	//std::cout << "[i] working on the " << cut_index << "th cut of current node\n";

      	if ( cut->size() == 1 || cut->size() < ps_.min_cand_cut_size )
        {
          continue;
        }

        const auto tt = cuts.truth_table( *cut );
				//assert( cut->size() == static_cast<unsigned> ( tt.num_vars() ) );

				//std::cout << "[i] loading leaves started\n";
				//std::vector<signal<Ntk>> children( cut->size() );
				std::array<signal<Ntk>, num_vars> children;
	      auto ctr = 0u;
	      for ( auto l : *cut )
        {
          children[ctr++] = ntk_.make_signal( ntk_.index_to_node( l ) );
        }
        //std::cout << "[i] loading leaves ended\n";

        //std::cout << "[i] mffc calculation started\n";
        int32_t cost_mffc = measure_mffc_deref( n, cut );
        measure_mffc_ref( n, cut );
        //std::cout << "[i] mffc calculation ended\n";

        const auto on_signal = [&]( auto const& f_new, uint32_t cost_cut ) {
					int32_t gain = cost_mffc - cost_cut;

					if ( ( gain > 0 || ( ps_.allow_zero_gain && gain == 0 ) ) && gain > best_gain )
					{
						best_gain = gain;
						best_leaves = children;
						best_cut_index = cut_index - 1;
					}

					return true;
				};

				stopwatch<> t_rewriting( st_.time_rewriting );
				rewriting_fn_( ntk_, tt, children.begin(), children.end(), on_signal, false );
      }

      if ( best_gain > 0 || ( ps_.allow_zero_gain && best_gain == 0 ) )
      {
      	auto cut = ( cuts.cuts( ntk_.node_to_index( n ) ) )[best_cut_index];
      	const auto tt = cuts.truth_table( cut );
      	signal<Ntk> best_signal;
      	const auto on_signal = [&]( auto const& f_new, uint32_t cost_cut) {
					best_signal = f_new;

					return true;
				};

      	stopwatch<> t_rewriting( st_.time_rewriting );
				rewriting_fn_( ntk_, tt, best_leaves.begin(), best_leaves.end(), on_signal );
				//assert( n != ntk_.get_node( best_signal ) );
				if ( n != ntk_.get_node( best_signal ) )
				{
					ntk_.substitute_node( n, best_signal );
					clear_cuts_fanout_rec( ntk_fanout_, cuts, cut_manager, ntk_.get_node( best_signal ) );
				}
      }
    } );

		uint32_t cost_aft = costs<Ntk, NodeCostFn>( ntk_ );
		std::cout << "\toptimized cost = " << cost_aft << "\n";
		return ntk_;
  }

private:
  int32_t measure_mffc_ref( node<Ntk> const& n, cut_t const* cut )
  {
    /* reference cut leaves */
    for ( auto leaf : *cut )
    {
      ntk_.incr_fanout_size( ntk_.index_to_node( leaf ) );
    }

    int32_t mffc_size = static_cast<int32_t>( recursive_ref( n ) );

    /* dereference leaves */
    for ( auto leaf : *cut )
    {
      ntk_.decr_fanout_size( ntk_.index_to_node( leaf ) );
    }

    return mffc_size;
  }

  int32_t measure_mffc_deref( node<Ntk> const& n, cut_t const* cut )
  {
    /* reference cut leaves */
    for ( auto leaf : *cut )
    {
      ntk_.incr_fanout_size( ntk_.index_to_node( leaf ) );
    }

    int32_t mffc_size = static_cast<int32_t>( recursive_deref( n ) );

    /* dereference leaves */
    for ( auto leaf : *cut )
    {
      ntk_.decr_fanout_size( ntk_.index_to_node( leaf ) );
    }

    return mffc_size;
  }

  uint32_t recursive_deref( node<Ntk> const& n )
  {
    /* terminate? */
    if ( ntk_.is_constant( n ) || ntk_.is_pi( n ) )
      return 0;

    /* recursively collect nodes */
    uint32_t value{ NodeCostFn{}( ntk_, n ) };
    ntk_.foreach_fanin( n, [&]( auto const& s ) {
      if ( ntk_.decr_fanout_size( ntk_.get_node( s ) ) == 0 )
      {
        value += recursive_deref( ntk_.get_node( s ) );
      }
    } );
    return value;
  }

  uint32_t recursive_ref( node<Ntk> const& n )
  {
    /* terminate? */
    if ( ntk_.is_constant( n ) || ntk_.is_pi( n ) )
      return 0;

    /* recursively collect nodes */
    uint32_t value{ NodeCostFn{}( ntk_, n ) };
    ntk_.foreach_fanin( n, [&]( auto const& s ) {
      if ( ntk_.incr_fanout_size( ntk_.get_node( s ) ) == 0 )
      {
        value += recursive_ref( ntk_.get_node( s ) );
      }
    } );
    return value;
  }

  void clear_cuts_fanout_rec( fanout_view<Ntk>& ntk_fanout_, network_cuts_t& cuts, cut_manager_t& cut_manager, node<Ntk> const& n )
  {
    ntk_fanout_.foreach_fanout( n, [&]( auto const& g ) {
      auto const index = ntk_.node_to_index( g );
      if ( cuts.cuts( index ).size() > 0 )
      {
        cut_manager.clear_cuts( g );
        clear_cuts_fanout_rec( ntk_fanout_, cuts, cut_manager, g );
      }
    } );
  }

private:
  Ntk ntk_;
  RewritingFn const& rewriting_fn_;
  rewrite_params const& ps_;
  rewrite_stats& st_;
};

} /* namespace detail */

template<class Ntk, class RewritingFn, class NodeCostFn>
Ntk rewrite( Ntk const& ntk, RewritingFn const& rewriting_fn = {}, rewrite_params const& ps = {}, rewrite_stats* pst = nullptr )
{
	rewrite_stats st;

	Ntk res = detail::rewrite_impl<Ntk, RewritingFn, NodeCostFn>( ntk, rewriting_fn, ps, st ).run();

	if ( ps.verbose )
	{
		st.report( );
	}
	if ( pst )
	{
		*pst = st;
	}
	return cleanup_dangling( res );
}

} /* namespace mockturtle */