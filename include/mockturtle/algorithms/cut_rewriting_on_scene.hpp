#pragma once

#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/views/topo_view.hpp>

namespace mockturtle
{

namespace detail
{

template<typename Ntk, typename TermCond>
void recursive_deref_no_value( Ntk const& ntk, node<Ntk> const& n, TermCond const& terminate )
{
  if ( terminate( n ) )
    return 0;

  /* recursively collect nodes */
  ntk.foreach_fanin( n, [&]( auto const& f ) {
    if ( ntk.decr_value( ntk.get_node( f ) ) == 0 )
    {
      recursive_deref_no_value<Ntk, TermCond>( ntk, ntk.get_node( f ), terminate );
    }
  } );
}

template<typename Ntk, typename TermCond>
void recursive_ref_no_value( Ntk const& ntk, node<Ntk> const& n, TermCond const& terminate )
{
  if ( terminate( n ) )
    return 0;

  /* recursively collect nodes */
  ntk.foreach_fanin( n, [&]( auto const& f ) {
    if ( ntk.incr_value( ntk.get_node( f ) ) == 0 )
    {
      recursive_ref_no_value<Ntk, TermCond>( ntk, ntk.get_node( f ), terminate );
    }
  } );
}

template<typename Ntk>
void recursive_deref_no_value( Ntk const& ntk, node<Ntk> const& n )
{
	const auto terminate = [&]( auto const& n ) { return ntk.is_constant( n ) || ntk.is_pi( n ); };
	return recursive_deref_no_value<Ntk, decltype( terminate )>( ntk, n, terminate );
}

template<typename Ntk>
void recursive_ref_no_value( Ntk const& ntk, node<Ntk> const& n )
{
	const auto terminate = [&]( auto const& n ) { return ntk.is_constant( n ) || ntk.is_pi( n ); };
	return recursive_ref_no_value<Ntk, decltype( terminate )>( ntk, n, terminate );
}

template<class Ntk, class NtkCostFn>
uint32_t compute_cost( Ntk const& ntk )
{
	NtkCostFn ntk_fn{};
	uint32_t cost_total = ntk_fn( ntk );
	return cost_total;
}

template<typename Ntk, class NtkCostFn>
uint32_t mffc_cost( Ntk const& ntk, node<Ntk> const& n, uint32_t cost_bfr )
{
  recursive_deref_no_value<Ntk>( ntk, n );
  auto value_n = ntk.value( n );
  ntk.set_value( n, 0u );
  uint32_t cost_new = compute_cost<Ntk, NtkCostFn>( ntk );
  ntk.set_value( n, value_n );
  recursive_ref_no_value<Ntk>( ntk, n );
  return cost_bfr - cost_new;
}

template<class Ntk, class RewritingFn, class NtkCostFn>
struct cut_rewriting_on_scene_impl
{
	cut_rewriting_on_scene_impl( Ntk const& ntk, RewritingFn const& rewriting_fn, cut_rewriting_params const& ps, cut_rewriting_stats& st )
		: ntk_( ntk ),
		  rewriting_fn_( rewriting_fn ),
		  ps_( ps ),
		  st_( st )
	{
	} 

	Ntk run()
	{
		stopwatch t_total( st_.time_total );

		/* compute the original cost */
		initialize_values_with_fanout( ntk_ );
		ntk_.foreach_po( [&]( auto const& f ) {
			ntk_.incr_value( f.index );
		} );
		const auto cost_bfr = compute_cost<Ntk, NtkCostFn>( ntk_ );

		/* cut enumeration */
		const auto cuts = call_with_stopwatch( st_.time_cuts, [&]() {
			return cut_enumeration<Ntk, true, cut_enumeration_cut_rewriting_cut>( ntk_, ps_.cut_enumeration_ps );
		} );

		Ntk dest;
		node_map<signal<Ntk>, Ntk> old2new( ntk_ );

		/* generate constant */
		old2new[ntk_.get_constant( false )] = dest.get_constant( false );

		/* generate pis */
		ntk_.foreach_pi( [&]( auto const& n ) {
			old2new[n] = dest.create_pi();
		} );

		/* evaluate each cut of each node */
		progress_bar pbar{ ntk_.num_gates(), "cut_rewriting |{0}| node = {1:>4} / " + std::to_string( ntk_.num_gates() ) + "   original cost = " + std::to_string( cost_bfr ), ps_.progress };
		ntk_.foreach_gate( [&]( auto const& n, auto i ) {
			pbar( i, i );

			int32_t gain_best = -1;
			uint32_t cost_mffc = mffc_cost<Ntk, NtkCostFn>( ntk_, n, cost_bfr );
			//std::cout << "[m] mffc of node " << n << " is " << cost_mffc << "\n";
			signal<Ntk> signal_best;

			for ( auto& cut: cuts.cuts( ntk_.node_to_index( n ) ) )
			{
				if ( cut->size() == 1 || cut->size() < ps_.min_cand_cut_size )
				{
					continue;
				}

				const auto tt = cuts.truth_table( *cut );
				assert( cut->size() == static_cast<unsigned> ( tt.num_vars() ) );

				std::vector<signal<Ntk>> children;
				for ( auto child: *cut )
				{
					children.emplace_back( ntk_.make_signal( ntk_.index_to_node( child ) ) );
				}

				const auto on_signal = [&]( auto const& f_new, uint32_t cost_cut ) {
					int32_t gain = cost_mffc - cost_cut;

					if ( ( gain > 0 || ( ps_.allow_zero_gain && gain == 0 ) ) && gain > gain_best )
					{
						gain_best = gain;
						signal_best = f_new;
					}

					//std::cout << "[m] cost of current cut of node " << n << " is " << cost_cut << "\n";

					return true;
				};

				stopwatch<> t_rewriting( st_.time_rewriting );
				rewriting_fn_( dest, cuts.truth_table( *cut ), children.begin(), children.end(), on_signal );
			}

			if ( gain_best == -1 )
			{
				std::vector<signal<Ntk>> children;
				ntk_.foreach_fanin( n, [&]( auto const& f ) {
					children.emplace_back( ntk_.is_complemented( f ) ? dest.create_not( old2new[f] ) : old2new[f] );
				} );

				old2new[n] = dest.clone_node( ntk_, n, children );
			}
			else
			{
				old2new[n] = signal_best;
			}

			recursive_ref_no_value<Ntk>( dest, dest.get_node( old2new[n] ) );
		} );

		/* generate pos */
		ntk_.foreach_po( [&]( auto const& f ) {
			dest.create_po( ntk_.is_complemented( f ) ? dest.create_not( old2new[f] ) : old2new[f] );
		} );

		dest = cleanup_dangling<Ntk>( dest );

		initialize_values_with_fanout( dest );
		dest.foreach_po( [&]( auto const& f ) {
			dest.incr_value( f.index );
		} );
		uint32_t cost_aft = compute_cost<Ntk, NtkCostFn>( dest );
		dest.clear_values();
		ntk_.clear_values();
		std::cout << "\toptimized cost = " << cost_aft << "\n";
		return cost_aft > cost_bfr ? ntk_ : dest;
	}

private:
	Ntk const& ntk_;
	RewritingFn const& rewriting_fn_;
	cut_rewriting_params const& ps_;
	cut_rewriting_stats& st_;
};

} /* namespace detail */

template<class Ntk, class RewritingFn, class NtkCostFn>
Ntk cut_rewriting_on_scene( Ntk const& ntk, RewritingFn const& rewriting_fn = {}, cut_rewriting_params const& ps = {}, cut_rewriting_stats* pst = nullptr )
{
	cut_rewriting_stats st;

	const auto dest = detail::cut_rewriting_on_scene_impl<Ntk, RewritingFn, NtkCostFn>( ntk, rewriting_fn, ps, st ).run();

	if ( ps.verbose )
	{
		st.report( false );
	}
	if ( pst )
	{
		*pst = st;
	}

	return dest;
}

} /* namespace mockturtle */