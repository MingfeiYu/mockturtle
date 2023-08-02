#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <vector>

#include <fmt/format.h>

#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/cut_enumeration.hpp>
#include <mockturtle/algorithms/cut_enumeration/rewrite_cut.hpp>
#include <mockturtle/algorithms/merge_split_xag.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/utils/cost_functions.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/utils/progress_bar.hpp>
#include <mockturtle/views/cut_view.hpp>
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

void print_xag( xag_network const& ntk, cut_view<xag_network> const& cut )
{
  std::cout << "PIs: ";
  cut.foreach_pi( [&]( auto const& n ) {
    std::cout << n << " ";
  } );
  std::cout << "\n";

  cut.foreach_gate( [&]( auto const& n ) {
    std::cout << "Node " << n << " = " 
              << ( ntk.is_and( n ) ? "AND( " : "XOR( " );
    ntk.foreach_fanin( n, [&]( auto const& ni ) {
      std::cout << ( ntk.is_complemented( ni ) ? "!" : "" ) 
                << ntk.get_node( ni ) << " ";
    } );
    std::cout << ")\n";
  } );

  std::cout << "POs are: ";
  cut.foreach_po( [&]( auto const& f ) {
    std::cout << ( ntk.is_complemented( f ) ? "!" : "" ) << "node " << ntk.get_node( f ) << " ";
  } );
  std::cout << "\n";
}

void print_xag( xag_network const& ntk )
{
  ntk.foreach_gate( [&]( auto const& n ) {
    std::cout << "Node " << n << " = " 
              << ( ntk.is_and( n ) ? "AND( " : "XOR( " );
    ntk.foreach_fanin( n, [&]( auto const& ni ) {
      std::cout << ( ntk.is_complemented( ni ) ? "!" : "" ) 
                << ntk.get_node( ni ) << " ";
    } );
    std::cout << ")\n";
  } );
  std::cout << "POs are: ";
  ntk.foreach_po( [&]( auto const& f ) {
    std::cout << ( ntk.is_complemented( f ) ? "!" : "" ) << "node " << ntk.get_node( f ) << " ";
  } );
  std::cout << "\n";
}

template<class Ntk = fanout_view<xag_network>>
struct root_of_interest
{
  bool operator()( Ntk const& ntk, typename Ntk::node const& n ) const
  {
    bool is_root_of_interest{ true };
    if ( ntk.is_and( n ) )
    {
      ntk.foreach_fanin( n, [&]( auto const& ni ) {
        auto const child = ntk.get_node( ni );
        if ( ntk.is_and( child ) && !ntk.is_complemented( ni ) && ( ntk.fanout( child ).size() == 1u ) )
        {
          is_root_of_interest = false;
        }
      } );

      if ( false ) {
      if ( is_root_of_interest && ( ntk.fanout( n ).size() == 1 ) )
      {
        auto const no = ( ntk.fanout( n ) )[0];
        if ( ntk.is_and( no ) )
        {
          ntk.foreach_fanin( no, [&]( auto const& ni ) {
            if ( !is_root_of_interest )
            {
              return false;
            }
            if ( ( ntk.get_node( ni ) == n ) && !ntk.is_complemented( ni ) )
            {
              is_root_of_interest = false;
            }
          } );
        }
      }}
    }
    return is_root_of_interest;
  }

  bool quant( Ntk const& ntk, typename Ntk::node const& n ) const
  {
    bool is_root_to_skip{ false };
    if ( ntk.is_and( n ) )
    {
      ntk.foreach_fanin( n, [&]( auto const& ni ) {
        auto const child = ntk.get_node( ni );
        if ( ntk.is_and( child ) && !ntk.is_complemented( ni ) && ( ntk.fanout( child ).size() == 1u ) )
        {
          is_root_to_skip = true;
        }
      } );

      if ( false ) {
      if ( !is_root_to_skip && ( ntk.fanout( n ).size() == 1 ) )
      {
        auto const no = ( ntk.fanout( n ) )[0];
        if ( ntk.is_and( no ) )
        {
          ntk.foreach_fanin( no, [&]( auto const& ni ) {
            if ( is_root_to_skip )
            {
              return false;
            }
            if ( ( ntk.get_node( ni ) == n ) && !ntk.is_complemented( ni ) )
            {
              is_root_to_skip = true;
            }
          } );
        }
      }}
    }
    return is_root_to_skip;
  }
};

template<class Ntk = fanout_view<xag_network>>
struct leaf_of_interest
{
  bool operator()( Ntk const& ntk, typename Ntk::signal const& f ) const
  {
    bool is_leaf_of_interest{ true };
    auto n = ntk.get_node( f );
    if ( ntk.is_and( n ) && ( ntk.fanout( n ).size() == 1 ) )
    {
      auto const no = ( ntk.fanout( n ) )[0];
      if ( ntk.is_and( no ) )
      {
        /* it is assured that 'f' is not complemented */
        //ntk.foreach_fanin( no, [&]( auto const& ni ) {
        //  if ( !is_leaf_of_interest )
        //  {
        //    return false;
        //  }
        //  if ( ( ntk.get_node( ni ) == n ) && !ntk.is_complemented( f ) )
        //  {
        //    is_leaf_of_interest = false;
        //  }
        //} );
        is_leaf_of_interest = false;
      }
    }

    if ( false ) {
    if ( is_leaf_of_interest && ntk.is_and( n ) )
    {
      ntk.foreach_fanin( n, [&]( auto const& ni ) {
        auto const child = ntk.get_node( ni );
        if ( ntk.is_and( child ) && !ntk.is_complemented( ni ) && ( ntk.fanout( child ).size() == 1u ) )
        {
          is_leaf_of_interest = false;
        }
      } );
    } }

    return is_leaf_of_interest;
  }

  bool quant( Ntk const& ntk, typename Ntk::signal const& f ) const
  {
    bool is_leaf_to_skip{ false };
    auto n = ntk.get_node( f );
    if ( ntk.is_and( n ) && ( ntk.fanout( n ).size() == 1 ) )
    {
      auto const no = ( ntk.fanout( n ) )[0];
      if ( ntk.is_and( no ) )
      {
        is_leaf_to_skip = true;
      }
    }

    if ( false ) {
    if ( !is_leaf_to_skip && ntk.is_and( n ) )
    {
      ntk.foreach_fanin( n, [&]( auto const& ni ) {
        auto const child = ntk.get_node( ni );
        if ( ntk.is_and( child ) && !ntk.is_complemented( ni ) && ( ntk.fanout( child ).size() == 1u ) )
        {
          is_leaf_to_skip = true;
        }
      } );
    } }

    return is_leaf_to_skip;
  }
};

template<class Ntk, class RewritingFn, class NodeCostFn>
struct rewrite_impl
{
	static constexpr uint32_t num_vars = 5u;
  using network_cuts_t = dynamic_network_cuts<Ntk, num_vars, true, cut_enumeration_rewrite_cut>;
  using cut_manager_t = detail::dynamic_cut_enumeration_impl<Ntk, num_vars, true, cut_enumeration_rewrite_cut>;
  using cut_t = typename network_cuts_t::cut_t;

public:
	rewrite_impl( Ntk const& ntk, RewritingFn const& rewriting_fn, rewrite_params const& ps, rewrite_stats& st )
    : ntk_ori_( ntk ),
      rewriting_fn_( rewriting_fn ),
      ps_( ps ),
      st_( st )
  {
    node_map<signal<Ntk>, Ntk> old2new( ntk_ori_ );

    /* constants */
    old2new[ntk_ori_.get_constant( false )] = ntk_.get_constant( false );
    if ( ntk_ori_.get_node( ntk_ori_.get_constant( true ) ) != ntk_ori_.get_node( ntk_ori_.get_constant( false ) ) )
    {
      old2new[ntk_ori_.get_constant( true )] = ntk_.get_constant( true );
    }

    /* pis */
    ntk_ori_.foreach_pi( [&]( auto const& n ) {
      old2new[n] = ntk_.create_pi();
    } );

    /* gates */
    ntk_ori_.foreach_gate( [&]( auto const& n ) {
      std::vector<signal<Ntk>> children( ntk_ori_.fanin_size( n ) );
      ntk_ori_.foreach_fanin( n, [&]( auto const& f, auto i ) {
          children[i] = ntk_ori_.is_complemented( f ) ? ntk_.create_not( old2new[f] ) : old2new[f];
        } );

      old2new[n] = ntk_.clone_node( ntk_ori_, n, children );
    } );

    /* pos */
    ntk_ori_.foreach_po( [&]( auto const& f ) {
      ntk_.create_po( ntk_ori_.is_complemented( f ) ? ntk_.create_not( old2new[f] ) : old2new[f] );
    } );
  }

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

  Ntk run_strict()
  {
    stopwatch t_total( st_.time_total );
    fanout_view<Ntk> ntk_fanout_{ ntk_ };

    cut_enumeration_stats cut_enum_st;
    network_cuts_t cuts( ntk_.size() + ( ntk_.size() >> 1 ) );
    cut_manager_t cut_manager( ntk_, ps_.cut_enumeration_ps, cut_enum_st, cuts );
    cut_manager.init_cuts();

    std::array<signal<Ntk>, num_vars> best_leaves;
    const auto orig_cost = costs<Ntk, NodeCostFn>( ntk_ );

    progress_bar pbar{ ntk_.num_gates(), "rewriting |{0}| node = {1:>4} / " + std::to_string( ntk_.num_gates() ) + "   original cost = " + std::to_string( orig_cost ), ps_.progress };
    ntk_.foreach_gate( [&]( auto const& n, auto i ) {
      //std::cout << "\n[i] working on node " << n << "\n";
      pbar( i, i );

      if ( ntk_.fanout_size( n ) == 0u || !root_of_interest{}( ntk_fanout_, n ) )
      {
        return true;
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
        bool skip{ false };
        for ( auto l : *cut )
        {
          auto signal_tmp = ntk_.make_signal( ntk_.index_to_node( l ) );
          children[ctr++] = signal_tmp;
          if ( !leaf_of_interest{}( ntk_fanout_, signal_tmp ) )
          {
            skip = true;
          }
        }
        if ( skip )
        {
          continue;
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

  Ntk run_vote()
  {
    stopwatch t_total( st_.time_total );
    fanout_view<Ntk> ntk_fanout_{ ntk_ };

    cut_enumeration_stats cut_enum_st;
    network_cuts_t cuts( ntk_.size() + ( ntk_.size() >> 1 ) );
    cut_manager_t cut_manager( ntk_, ps_.cut_enumeration_ps, cut_enum_st, cuts );
    cut_manager.init_cuts();

    std::array<signal<Ntk>, num_vars> best_leaves;
    const auto orig_cost = costs<Ntk, NodeCostFn>( ntk_ );

    progress_bar pbar{ ntk_.num_gates(), "rewriting |{0}| node = {1:>4} / " + std::to_string( ntk_.num_gates() ) + "   original cost = " + std::to_string( orig_cost ), ps_.progress };
    ntk_.foreach_gate( [&]( auto const& n, auto i ) {
      //std::cout << "\n[i] working on node " << n << "\n";
      pbar( i, i );

      if ( ntk_.fanout_size( n ) == 0u )
      {
        return true;
      }

      uint8_t root_to_skip = 2 * static_cast<uint8_t>( root_of_interest{}.quant( ntk_fanout_, n ) );

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
        uint8_t cut_to_skip{ root_to_skip };
        std::array<signal<Ntk>, num_vars> children;
        auto ctr = 0u;
        for ( auto l : *cut )
        {
          auto signal_tmp = ntk_.make_signal( ntk_.index_to_node( l ) );
          children[ctr++] = signal_tmp;
          cut_to_skip += static_cast<uint8_t>( leaf_of_interest{}.quant( ntk_fanout_, signal_tmp ) );
        }
        if ( ( cut_to_skip * 3 ) >= ( children.size() + 1 ) )
        {
          continue;
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

  Ntk run_precise_vote()
  {
    stopwatch t_total( st_.time_total );
    fanout_view<Ntk> ntk_fanout_{ ntk_ };

    cut_enumeration_stats cut_enum_st;
    network_cuts_t cuts( ntk_.size() + ( ntk_.size() >> 1 ) );
    cut_manager_t cut_manager( ntk_, ps_.cut_enumeration_ps, cut_enum_st, cuts );
    cut_manager.init_cuts();

    std::array<signal<Ntk>, num_vars> best_leaves;
    const auto orig_cost = costs<Ntk, NodeCostFn>( ntk_ );

    progress_bar pbar{ ntk_.num_gates(), "rewriting |{0}| node = {1:>4} / " + std::to_string( ntk_.num_gates() ) + "   original cost = " + std::to_string( orig_cost ), ps_.progress };
    ntk_.foreach_gate( [&]( auto const& n, auto i ) {
      pbar( i, i );

      if ( ntk_.fanout_size( n ) == 0u )
      {
        return true;
      }

      bool root_to_skip = root_of_interest{}.quant( ntk_fanout_, n );

      cut_manager.clear_cuts( n );
      cut_manager.compute_cuts( n );

      uint32_t cut_index = 0;
      int32_t best_gain = -1;
      uint32_t best_cut_index{ 0u };

      for ( auto& cut : cuts.cuts( ntk_.node_to_index( n ) ) )
      {
        ++cut_index;
        //std::cout << "[m] working on the " << cut_index << "th cut of current node\n";

        if ( cut->size() == 1 || cut->size() < ps_.min_cand_cut_size )
        {
          continue;
        }

        const auto tt = cuts.truth_table( *cut );

        //std::cout << "[i] loading leaves started\n";
        std::vector<bool> to_skip;
        to_skip.emplace_back( root_to_skip );
        std::array<signal<Ntk>, num_vars> children;
        auto ctr = 0u;
        for ( auto l : *cut )
        {
          auto signal_tmp = ntk_.make_signal( ntk_.index_to_node( l ) );
          children[ctr++] = signal_tmp;
          to_skip.emplace_back( leaf_of_interest{}.quant( ntk_fanout_, signal_tmp ) );
        }
        //std::cout << "[i] loading leaves ended\n";

        /* replace with the t-count of the current cut */
        int32_t cost_mffc = measure_mffc_deref( n, cut );
        measure_mffc_ref( n, cut );

        // std::vector<signal<Ntk>> children_vec;
        // for ( auto l : *cut )
        // {
        //   children_vec.emplace_back( ntk_.make_signal( ntk_.index_to_node( l ) ) );
        // }

        // cut_view<xag_network> cut_current_impl{ ntk_, children_vec, ntk_.make_signal( n ) };
        // /* check validity */
        // /* seems there is a bug in 'cut_enumeration' */
        // /* since it is witnessed that a pi is recognized as an internal node in a cone, rather than a leaf */
        // bool valid{ true };
        // cut_current_impl.foreach_gate( [&]( auto const& nl ) {
        //   if ( ntk_.fanin_size( nl ) == 0 )
        //   {
        //     valid = false;
        //     return false;
        //   }

        //   return true;
        // } );
        // if ( !valid )
        // {
        //   continue;
        // }

        // std::cout << "children array: ";
        // for ( auto const& children_each: children )
        // {
        //   std::cout << ( children_each.complement ? "!" : "" ) << children_each.index << " ";
        // }
        // std::cout << "\n";
        // std::cout << "children vector: ";
        // for ( auto const& children_vec_each: children_vec )
        // {
        //   std::cout << ( children_vec_each.complement ? "!" : "" ) << children_vec_each.index << " ";
        // }
        // std::cout << "\n";
        // print_xag( ntk_, cut_current_impl );

        // xag_network xag_cur_impl;
        // std::vector<signal<Ntk>> xag_cur_impl_pis;

        // for ( auto const& children_vec_each: children_vec )
        // {
        //   xag_cur_impl_pis.emplace_back( xag_cur_impl.create_pi() );
        // }
        // cleanup_dangling( cut_current_impl, xag_cur_impl, xag_cur_impl_pis.begin(), xag_cur_impl_pis.end() );

        // /* implement a node map-like map, */
        // /* based on which convert the cut into an XAG for cost evaluation */
        // std::map<int, signal<Ntk>> node_map_mine;

        // /* const */
        // node_map_mine.insert( std::pair<int, signal<Ntk>>( ntk_.get_constant( false ).index, xag_cur_impl.get_constant( false ) ) );
        // if ( ntk_.get_node( ntk_.get_constant( true ) ) != ntk_.get_node( ntk_.get_constant( false ) ) )
        // {
        //   node_map_mine.insert( std::pair<int, signal<Ntk>>( ntk_.get_constant( true ).index, xag_cur_impl.get_constant( true ) ) );
        // }

        // /* pis */
        // cut_current_impl.foreach_pi( [&]( auto const& nl ) {
        //   node_map_mine.insert( std::pair<int, signal<Ntk>>( ntk_.node_to_index( nl ), xag_cur_impl.create_pi() ) );
        // } );

        // /* gates */
        // cut_current_impl.foreach_gate( [&]( auto const& nl ) {
        //   std::vector<signal<Ntk>> fanin( ntk_.fanin_size( nl ) );
        //   ntk_.foreach_fanin( nl, [&]( auto const& fi, auto i ) {
        //     // if ( node_map_mine.find( fi.index ) == node_map_mine.end() )
        //     // {
        //     //   std::cout << "[e] topology issue!\n";
        //     //   abort();
        //     // }
        //     fanin[i] = ntk_.is_complemented( fi ) ? ntk_.create_not( node_map_mine[fi.index] ) : node_map_mine[fi.index];
        //   } );
        //   node_map_mine.insert( std::pair<int, signal<Ntk>>( ntk_.node_to_index( nl ), xag_cur_impl.clone_node( ntk_, nl, fanin ) ) );
        // } );

        // /* po */
        // cut_current_impl.foreach_po( [&]( auto const& f ) {
        //   xag_cur_impl.create_po( node_map_mine[f.index] );
        // } );

        // uint32_t cost_origin = merge_split_xag( xag_cur_impl );

        const auto on_signal = [&]( uint32_t cost_cut, std::vector<bool> blocked ) {
          auto cut_to_skip{ 0u }; 
          //std::cout << "[m] cut evaluation result: \n";
          for ( auto i{ 0u }; i < to_skip.size(); ++i )
          {
            //if ( i == 0u )
            //{
            //  std::cout << "Root information: ";
            //}
            //else
            //{
            //  std::cout << "Leaf" << i << " information: ";
            //}
            //std::cout << "can be skipped - " << ( to_skip[i] ? "yes" : "no" ) << "; ";
            //std::cout << "can be a problem - " << ( blocked[i] ? "yes" : "no" ) << ".\n";

            if ( to_skip[i] && blocked[i] )
            {
              cut_to_skip += 1u;
            }
          }

          //if ( cut_to_skip > 0u )
          if ( ( cut_to_skip * 5 ) >= to_skip.size() )
          {
            return true;
          }

          int32_t gain = cost_mffc - cost_cut;
          //int32_t gain = cost_mffc - cost_cut - cut_to_skip;
          //int32_t gain = cost_origin - cost_cut - cut_to_skip;
          // std::cout << "[m] T-count of current cut: " << cost_origin << "; ";
          // std::cout << "Penalty: " << cut_to_skip << "; ";
          // std::cout << "T-count of new impl.: " << cost_cut << "; ";
          // std::cout << "GAIN: " << gain << "\n";

          if ( ( gain > 0 || ( ps_.allow_zero_gain && gain == 0 ) ) && gain > best_gain )
          {
            best_gain = gain;
            best_leaves = children;
            best_cut_index = cut_index - 1;
          }

          return true;
        };
        rewriting_fn_.profile_block_situation( tt, children.begin(), children.end(), on_signal );
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
          //std::cout << "[m] --------rewriting operated--------\n";
        }
      }
    } );

    uint32_t cost_aft = costs<Ntk, NodeCostFn>( ntk_ );
    //uint32_t cost_aft = merge_split_xag( ntk_ );
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
  Ntk const& ntk_ori_;
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

	//Ntk res = detail::rewrite_impl<Ntk, RewritingFn, NodeCostFn>( ntk, rewriting_fn, ps, st ).run();
  //Ntk res = detail::rewrite_impl<Ntk, RewritingFn, NodeCostFn>( ntk, rewriting_fn, ps, st ).run_strict();
  //Ntk res = detail::rewrite_impl<Ntk, RewritingFn, NodeCostFn>( ntk, rewriting_fn, ps, st ).run_vote();
  Ntk res = detail::rewrite_impl<Ntk, RewritingFn, NodeCostFn>( ntk, rewriting_fn, ps, st ).run_precise_vote();

	if ( ps.verbose )
	{
		st.report();
	}
	if ( pst )
	{
		*pst = st;
	}
	return cleanup_dangling( res );
}

} /* namespace mockturtle */