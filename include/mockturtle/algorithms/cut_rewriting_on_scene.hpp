#pragma once

#include <mockturtle/algorithms/cut_rewriting.hpp>
//#include <mockturtle/views/topo_view.hpp>

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
int32_t mffc_cost( Ntk const& ntk, node<Ntk> const& n, uint32_t cost_bfr )
{
	//std::cout << "[m] before deref: ";
  //ntk.foreach_gate( [&]( auto const& n ) {
	//	std::cout << "node " << n << "(" << ntk.value( n ) << ") "; 
	//} );
	//std::cout << "\n";
  recursive_deref_no_value<Ntk>( ntk, n );
  //auto value_n = ntk.value( n );
  //ntk.set_value( n, 0u );
  uint32_t cost_new = compute_cost<Ntk, NtkCostFn>( ntk );
  //ntk.set_value( n, value_n );
  recursive_ref_no_value<Ntk>( ntk, n );
  //std::cout << "[m] after ref: ";
  //ntk.foreach_gate( [&]( auto const& n ) {
	//	std::cout << "node " << n << "(" << ntk.value( n ) << ") "; 
	//} );
	//std::cout << "\n";
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
		//std::cout << "[m] ntk originally consists of: ";
		//ntk_.foreach_gate( [&]( auto const& n ) {
		//	std::cout << "node " << n << "(" << ntk_.value( n ) << ") "; 
		//} );
		//std::cout << "\n";
		//std::cout << "[i] computing original cost\n";
		const auto cost_bfr = compute_cost<Ntk, NtkCostFn>( ntk_ );
		//std::cout << "[i] original cost = " << cost_bfr << "\n";

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

			//std::cout << "\n[m] ntk consists of: ";
			//ntk_.foreach_gate( [&]( auto const& n ) {
			//	std::cout << "node " << n << " ";
			//} );
			//std::cout << "\n";
			//std::cout << "[m] computing mffc of node " << n << "\n";

			int32_t cost_mffc = mffc_cost<Ntk, NtkCostFn>( ntk_, n, cost_bfr );
			//std::cout << "[m] mffc of node " << n << " is " << cost_mffc << "\n";
			if ( cost_mffc <= 1 )
			{
				std::vector<signal<Ntk>> children;
				ntk_.foreach_fanin( n, [&]( auto const& f ) {
					children.emplace_back( ntk_.is_complemented( f ) ? dest.create_not( old2new[f] ) : old2new[f] );
				} );

				old2new[n] = dest.clone_node( ntk_, n, children );
			}
			else
			{
				int32_t gain_best = -1;
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

					const auto on_signal = [&]( auto const& f_new ) {
						Ntk dest_tmp = dest.clone();
						dest_tmp.create_po( f_new );
						//std::cout << "[m] in this solution, dest_tmp consists of: ";
						//dest_tmp.foreach_gate( [&]( auto const& n ) {
						//	std::cout << "node " << n << "(" << dest_tmp.value( n ) << ") ";
						//} );
						//std::cout << "\n";
						//std::cout << "[m] before undoing pos, pos of dest_tmp is: ";
						//dest_tmp.foreach_po( [&]( auto const& f ) {
						//	std::cout << "node " << dest_tmp.get_node( f ) << " ";
						//} );
						//std::cout << "\n";
						
						//std::vector<uint32_t> nodes_unpo;
						//for ( auto po_index{ 0u }; po_index < dest_tmp._storage->outputs.size(); ++po_index )
						//{
						//	if ( dest_tmp.value( dest_tmp.get_node( dest_tmp._storage->outputs[po_index] ) ) > 1u )
						//	{
						//		nodes_unpo.emplace_back( dest_tmp.get_node( dest_tmp._storage->outputs[po_index] ) );
						//	}
						//}
						/* undo 'create_po()' */
						//for ( auto node_unpo: nodes_unpo )
						//{
						//	for ( auto po_index{ 0u }; po_index < dest_tmp._storage->outputs.size(); ++po_index )
						//	{
						//		if ( dest_tmp.get_node( dest_tmp._storage->outputs[po_index] ) == node_unpo )
						//		{
									//std::cout << "[m] undo 'create_po' on node " << node_unpo << "\n";
						//			--dest_tmp._storage->nodes[node_unpo].data[0].h1;
						//			dest_tmp.decr_value( node_unpo );
						//			dest_tmp._storage->outputs.erase( dest_tmp._storage->outputs.begin() + po_index );
						//			break;
						//		}
						//	}
						//}

						dest_tmp.incr_value( dest_tmp.get_node( f_new ) );
						recursive_ref_no_value<Ntk>( dest_tmp, dest_tmp.get_node( f_new ) );
						//std::cout << "[m] then, dest_tmp becomes: ";
						//dest_tmp.foreach_gate( [&]( auto const& n ) {
						//	std::cout << "node " << n << "(" << dest_tmp.value( n ) << ") ";
						//} );
						//std::cout << "\n";
						uint32_t cost_tmp = compute_cost<Ntk, NtkCostFn>( dest_tmp );

						recursive_deref_no_value<Ntk>( dest_tmp, dest_tmp.get_node( f_new ) );
						uint32_t cost_last = compute_cost<Ntk, NtkCostFn>( dest_tmp );

						int32_t gain = cost_mffc - ( cost_tmp - cost_last );
						/* TO CHECK: These two data are constantly 0 */
						//std::cout << "[m] cost_last is " << cost_last << ", cost_tmp is " << cost_tmp << "\n";
						//std::cout << "[m] cost of current cut is " << ( cost_tmp - cost_last ) << "\n";

						if ( ( gain > 0 || ( ps_.allow_zero_gain && gain == 0 ) ) && gain > gain_best )
						{
							gain_best = gain;
							signal_best = f_new;
						}

						return true;
					};

					stopwatch<> t_rewriting( st_.time_rewriting );
					rewriting_fn_( dest, tt, children.begin(), children.end(), on_signal );
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
			}

			//std::cout << "[m] computing GC of current dest\n";

			recursive_ref_no_value<Ntk>( dest, dest.get_node( old2new[n] ) );
			//dest.create_po( old2new[n] );
			

			//std::vector<uint32_t> nodes_unpo;
			//for ( auto po_index{ 0u }; po_index < dest._storage->outputs.size(); ++po_index )
			//{
			//	if ( dest.value( dest.get_node( dest._storage->outputs[po_index] ) ) > 1u )
			//	{
			//		nodes_unpo.emplace_back( dest.get_node( dest._storage->outputs[po_index] ) );
			//	}
			//}
			/* undo 'create_po()' */
			//for ( auto node_unpo: nodes_unpo )
			//{
			//	for ( auto po_index{ 0u }; po_index < dest._storage->outputs.size(); ++po_index )
			//	{
			//		if ( dest.get_node( dest._storage->outputs[po_index] ) == node_unpo )
			//		{
			//			//std::cout << "[m] undo 'create_po' on node " << node_unpo << "\n";
			//			--dest._storage->nodes[node_unpo].data[0].h1;
			//			dest.decr_value( node_unpo );
			//			dest._storage->outputs.erase( dest._storage->outputs.begin() + po_index );
			//			break;
			//		}
			//	}
			//}
			//dest.incr_value( dest.get_node( old2new[n] ) );

			//std::cout << "[m] n is " << n << ", "<< "dest consists of: ";
			//dest.foreach_gate( [&]( auto const& n ) {
			//	if ( dest.value( n ) )
			//	{
			//		std::cout << "node " << n << "(" << dest.value( n ) << ") ";
			//		if ( dest.is_and( n ) )
			//		{
			//			std::cout << "(AND)";
			//		}
			//		else
			//		{
			//			std::cout << "(XOR)";
			//		}
			//		std::cout << " ";
			//	}
			//} );
			//std::cout << "\n";
			//cost_last = compute_cost<Ntk, NtkCostFn>( dest );
			//std::cout << "[m] current cost of dest = " << cost_last << "\n";
		} );

		/* undo all the remaining 'po's */
		//dest.foreach_po( [&]( auto const& f ) {
		//	--dest._storage->nodes[dest.get_node( f )].data[0].h1;
		//} );
		//dest._storage->outputs.resize( 0 );

		/* generate pos */
		ntk_.foreach_po( [&]( auto const& f ) {
			dest.create_po( ntk_.is_complemented( f ) ? dest.create_not( old2new[f] ) : old2new[f] );
		} );

		//dest = cleanup_dangling<Ntk>( dest );

		initialize_values_with_fanout( dest );
		uint32_t cost_aft = compute_cost<Ntk, NtkCostFn>( dest );
		dest.clear_values();
		ntk_.clear_values();
		std::cout << "\toptimized cost = " << cost_aft << "\n";
		//if ( cost_last != cost_aft )
		//{
		//	std::cout << "\tcurrent network cost = " << cost_last << "\n";
		//}
		//assert( cost_last == cost_aft );
		//std::cout << "[i] resulted network consists of: \n";
		//dest.foreach_gate( [&]( auto const& n ) {
		//	std::cout << "node " << n << "(" << ( dest.is_and( n ) ? "AND" : "XOR" ) << "): ";
		//	dest.foreach_fanin( n, [&]( auto const& ni ) {
		//		std::cout << ( ni.complement ? "!" : "" ) << "node " << ni.index << " ";
		//	} );
		//	std::cout << "\n";
		//} );
		//std::cout << "[i] pos are: ";
		//dest.foreach_po( [&]( auto const& f ) {
		//	std::cout << ( dest.is_complemented( f ) ? "!" : "" ) << "node" << f.index << " ";
		//} );
		//std::cout << "\n";
		//if ( cost_aft > cost_bfr )
		//{
		//	std::cout << "[i] did not find a better solution, return the input network\n";
		//}
		//std::cout << "[i] The correspondence between two networks: \n";
		//ntk_.foreach_gate( [&]( auto const& n ) {
		//	std::cout << "OLD: node " << n << " " 
		//						<< "NEW: sig. " << ( dest.is_complemented( old2new[n] ) ? "!" : "" ) << "node" << dest.get_node( old2new[n] ) 
		//						<< "\n";
		//} );

		uint32_t num_pos{ 0u };
		std::cout << "[i] The POs in old network are: \n";
		ntk_.foreach_po( [&]( auto const& f ) {
			++num_pos;
			std::cout << ( ntk_.is_complemented( f ) ? "!" : "" ) << "node " << ntk_.get_node( f ) 
								<< "(" << ( dest.is_complemented( old2new[ntk_.get_node( f )] ) ? "!" : "" ) 
								<< "node " <<  dest.get_node( old2new[ntk_.get_node( f )] ) << ") ";
		} );
		std::cout << "\n";
		std::cout << "[i] " << num_pos << " POs in old network\n";

		num_pos = 0u;
		std::cout << "[i] The POs in new network are: \n";
		dest.foreach_po( [&]( auto const& f ) {
			++num_pos;
			std::cout << ( dest.is_complemented( f ) ? "!" : "" ) << "node " << dest.get_node( f ) << " ";
		} );
		std::cout << "\n";
		std::cout << "[i] " << num_pos << " POs in new network\n";
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