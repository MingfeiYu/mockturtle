#pragma once

#include "retiming.hpp"
#include "../networks/generic.hpp"
#include "../utils/cost_functions.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/fanout_view.hpp"

#include <fmt/format.h>

#include <cstdint>
#include <limits>

namespace mockturtle
{

struct area_retime_params
{
	bool is_delay_constrained{ false };
	bool forward_only{ false };
	bool backward_only{ false };
	uint32_t iterations{ 1u };
	uint32_t delay_cons{ 0u };
	bool verbose{ false };
};

struct area_retime_stats
{
	uint32_t num_reg_pre{ 0u };
	uint32_t num_reg_post{ 0u };
	uint32_t delay{ 0u };
	uint32_t delay_cons{ 0u };
	stopwatch<>::duration time_total{ 0 };
	void report() const
	{
		std::cout << fmt::format( "[i] Optimized delay    = {:7d}\t Delay constraint     = {:7d}\n", delay, delay_cons );
		std::cout << fmt::format( "[i] Initial #registers = {:7d}\t Optimized #registers = {:7d}\n", num_reg_pre, num_reg_post );
    std::cout << fmt::format( "[i] Total runtime      = {:>5.2f} secs\n", to_seconds( time_total ) );
	}
};

namespace detail
{

template<class Ntk = fanout_view<generic_network>, class NodeCostFn = unit_cost<Ntk>>
class area_retime_impl
{
public:
	using node = typename Ntk::node;
	using signal = typename Ntk::signal;
	const uint32_t sink = UINT32_MAX;

public:
	area_retime_impl( Ntk& ntk, NodeCostFn const& cost_fn = unit_cost<Ntk>(), area_retime_params const& ps = {}, area_retime_stats* pst = nullptr )
		: _ntk( ntk ),
		  _ps( ps ),
		  _pst( pst ),
		  _flow_path( ntk ),
		  _levels_asap( ntk ),
		  _levels_alap( ntk ),
		  _exact_cons( ntk ),
		  _cost_fn( cost_fn ),
		  _delay_cons( ps.delay_cons )
	{}

public:
	void run()
	{
		stopwatch time( _pst->time_total );
		_pst->num_reg_pre = _ntk.num_registers();
		_levels_asap.reset( 0u );
		compute_levels_asap();
		std::cout << fmt::format( "[i] Circuit delay before optimization is {}\n", _delay );
		_pst->delay = _delay;
		_delay_cons = _ps.is_delay_constrained ? std::max( _delay_cons, _delay ) : UINT32_MAX;
		_pst->delay_cons = _delay_cons;
		_delay = 0u;

		bool improvement{ true };

		if ( !_ps.backward_only )
		{
			for ( uint8_t i{ 0u }; i < _ps.iterations && improvement; ++i )
			{
				if ( _ps.verbose )
				{
					std::cout << fmt::format( "[i] Conducting the {}-th round of forward retiming...\n", ( i + 1 ) );
				}
				improvement = operate_retime<true>();
				_flow_path.reset();
				_levels_asap.reset();
				_levels_alap.reset();
				_exact_cons.reset();
			}
		}

		improvement = true;
		if ( !_ps.forward_only )
		{
			for ( uint8_t i{ 0u }; i < _ps.iterations && improvement; ++i )
			{
				if ( _ps.verbose )
				{
					std::cout << fmt::format( "[i] Conducting the {}-th round of backward retiming...\n", ( i + 1 ) );
				}
				improvement = operate_retime<false>();
				_flow_path.reset();
				_levels_asap.reset();
				_levels_alap.reset();
				_exact_cons.reset();
			}
		}

		_pst->num_reg_post = _ntk.num_registers();
	}

private:
uint32_t compute_level_asap_rec( node const& n )
{
	if ( _ntk.visited( n ) == _ntk.trav_id() )
	{
		return _levels_asap[n];
	}

	_ntk.set_visited( n, _ntk.trav_id() );

	if ( _ntk.is_constant( n ) || _ntk.is_pi( n ) || _ntk.is_box_input( n ) )
	{
		_levels_asap[n] = 0u;
		return 0u;
	}

	uint32_t node_level{ 0u };
	_ntk.foreach_fanin( n, [&]( signal const& f ) {
		node_level = std::max( node_level, compute_level_asap_rec( _ntk.get_node( f ) ) );
	} );
	node_level += _cost_fn( _ntk, n );
	// assert( node_level <= _delay_cons );
	_delay = std::max( _delay, node_level );
	_levels_asap[n] = node_level;
	return node_level;
}

uint32_t compute_level_asap_rec_detect_cons( node const& n )
{
	if ( !_ntk.value2( n ) || _ntk.visited( n ) == _ntk.trav_id() )
	{
		return _levels_asap[n];
	}

	_ntk.set_visited( n, _ntk.trav_id() );

	if ( _ntk.is_constant( n ) || _ntk.is_pi( n ) )
	{
		_levels_asap[n] = 0u;
		return 0u;
	}

	uint32_t node_level{ 0u };
	_ntk.foreach_fanin( n, [&]( signal const& f ) {
		node_level = std::max( node_level, compute_level_asap_rec_detect_cons( _ntk.get_node( f ) ) );
	} );
	node_level += _cost_fn( _ntk, n );
	_levels_asap[n] = node_level;
	if ( node_level > _delay_cons )
	{
		_ntk.set_value( n, 1u );
	}
	return node_level;
}

void compute_levels_asap()
{
	_levels_asap.reset( 0u );
	_ntk.incr_trav_id();

	_ntk.foreach_node( [&]( node const& n ) {
		uint32_t node_level = compute_level_asap_rec( n );
		// std::cout << fmt::format( "[m] Level of Node {} is {}\n", _ntk.node_to_index( n ), node_level );
	} );
}

uint32_t compute_level_alap_rec( node const& n )
{
	if ( _ntk.visited( n ) == _ntk.trav_id() )
	{
		return _levels_alap[n];
	}

	_ntk.set_visited( n, _ntk.trav_id() );

	if ( _ntk.is_constant( n ) || _ntk.is_pi( n ) )
	{
		_levels_alap[n] = 0u;
		return 0u;
	}

	if ( _ntk.is_register( n ) )
	{
		_levels_alap[n] = _delay_cons;
		return _delay_cons;
	}

	uint32_t node_level{ 0u };
	_ntk.foreach_fanout( n, [&]( node const& no ) {
		node_level = std::min( node_level, compute_level_alap_rec( no ) );
	} );
	node_level -= _cost_fn( _ntk, n );
	_levels_alap[n] = node_level;
	return node_level;
}

uint32_t compute_level_alap_rec_detect_cons( node const& n )
{
	if ( !_ntk.value2( n ) || _ntk.visited( n ) == _ntk.trav_id() )
	{
		return _levels_alap[n];
	}

	_ntk.set_visited( n, _ntk.trav_id() );

	if ( _ntk.is_po( n ) )
	{
		_levels_alap[n] = 0u;
		return 0u;
	}

	uint32_t node_level{ 0u };
	_ntk.foreach_fanout( n, [&]( node const& no ) {
		node_level = std::max( node_level, compute_level_alap_rec_detect_cons( no ) );
	} );
	node_level += _cost_fn( _ntk, n );
	_levels_asap[n] = node_level;
	if ( node_level > _delay_cons )
	{
		_ntk.set_value( n, 1u );
	}
	return node_level;
}

void compute_levels_alap()
{
	_levels_alap.reset( 0u );
	_ntk.trav_id();

	_ntk.foreach_node_reverse( [&]( node const& n ) {
		uint32_t node_level = compute_level_alap_rec( n );
	} );
}

void mark_tfi_rec( node const& n )
{
	if ( _ntk.value( n ) )
	{
		return;
	}

	_ntk.set_value( n, 1u );
	_ntk.foreach_fanin( n, [&]( signal const& f ) {
		if ( _ntk.is_constant( _ntk.get_node( f ) ) )
		{
			return true;
		}
		mark_tfi_rec( _ntk.get_node( f ) );
		return true;
	} );
}

void mark_tfo_rec( node const& n )
{
	if ( _ntk.value( n ) )
	{
		return;
	}

	_ntk.set_value( n, 1u );
	_ntk.foreach_fanout( n, [&]( node const& no ) {
		mark_tfo_rec( no );
	} );
}

void mark_tfi_rec_k_reg( node const& n, uint32_t num_enc_reg, uint32_t const& k )
{
	if ( num_enc_reg > k || _ntk.value2( n ) )
	{
		return;
	}

	_ntk.set_value2( n, 1u );

	if ( _ntk.is_box_output() )
	{
		++num_enc_reg;
	}
	_ntk.foreach_fanin( n, [&]( signal const& f ) {
		const node ni = _ntk.get_node( f );
		mark_tfi_rec_k_reg( ni, num_enc_reg, k );
	} );
}

void mark_tfo_rec_k_reg( node const& n, uint32_t num_enc_reg, uint32_t const& k )
{
	if ( num_enc_reg > k || _ntk.value2( n ) )
	{
		return;
	}

	_ntk.set_value2( n, 1u );

	if ( _ntk.is_box_input() )
	{
		++num_enc_reg;
	}
	_ntk.foreach_fanout( n, [&]( node const& no ) {
		mark_tfo_rec_k_reg( no, num_enc_reg, k );
	} );
}

template<bool forward>
void init_value()
{
	_ntk.clear_values();

	/* Mark the sink */
	if constexpr ( forward )
	{
		/* Mark POs */
		_ntk.foreach_po( [&]( signal const& f ) {
			_ntk.set_value( _ntk.get_node( f ), 1u );
		} );

		/* Mark existing registers (and box inputs) */
		_ntk.foreach_register( [&]( node const& n ) {
			_ntk.foreach_fanin( n, [&]( signal const& f ) {
				if ( _ntk.is_constant( _ntk.get_node( f ) ) )
				{
					return true;
				}
				_ntk.set_value( _ntk.get_node( f ), 1u );
				return true;
			} );
			_ntk.set_value( n, 1u );
		} );

		/* Mark PIs and their transitive fanouts */
		_ntk.foreach_pi( [&]( node const& n ) {
			mark_tfo_rec( n );
		} );

		/* Mark fanins of nodes marked so far (?) */
		std::vector<node> to_mark;
		to_mark.reserve( 200 );
		_ntk.foreach_gate( [&]( node const& n ) {
			if ( _ntk.value( n ) )
			{
				_ntk.foreach_fanin( n, [&]( signal const& f ) {
					if ( _ntk.is_constant( _ntk.get_node( f ) ) )
					{
						return true;
					}
					if ( _ntk.value( _ntk.get_node( f ) ) == 0u )
					{
						to_mark.push_back( _ntk.get_node( f ) );
					}
					return true;
				} );
			}
		} );
		for ( node const& n : to_mark )
		{
			_ntk.set_value( n, 1u );
		}
	}
	else
	{
		/* Mark PIs */
		_ntk.foreach_pi( [&]( signal const& f ) {
			_ntk.set_value( _ntk.get_node( f ), 1u );
		} );

		/* Mark existing registers (and box outputs) */
		_ntk.foreach_register( [&]( node const& n ) {
			_ntk.foreach_fanout( n, [&]( node const& no ) {
				_ntk.set_value( no, 1u );
			} );
			_ntk.set_value( n, 1u );
		} );

		/* Mark POs and their transitive fanins */
		_ntk.foreach_po( [&]( signal const& f ) {
			mark_tfi_rec( _ntk.get_node( f ) );
		} );
	}


	// std::cout << "[m] Nodes whose value is 1: { ";
  //   _ntk.foreach_node( [&]( auto const& n ) {
  //     if ( _ntk.value( n ) == 1u )
  //     {
  //       if ( _ntk.is_pi( n ) )
  //       {
  //         std::cout << "pi ";
  //       }
  //       else if ( _ntk.is_po( n ) )
  //       {
  //         std::cout << fmt::format( "po{} ", ( n - _ntk.num_gates() - _ntk.num_pis() - 1 ) );
  //       }
  //       else
  //       {
  //         std::cout << fmt::format( "n{} ", n - 2 );
  //       }
  //     }
  //   } );
  //   std::cout << "}\n";
}

template<bool forward>
void add_hard_constraints()
{
	if ( forward )
	{
		/* Highlight nodes that are either the TFO or the TFO_1 of PIs. */
		_ntk.clear_values2();
		_ntk.foreach_pi( [&]( node const& n ) {
			mark_tfo_rec_k_reg( n, 0u, 1u );
		} );

		/* Apply ASAP level computation to the highlighted nodes */
		_levels_asap.reset( 0u );
		_ntk.incr_trav_id();
		_ntk.foreach_node( [&]( node const& n ) {
			uint32_t node_level = compute_level_asap_rec_detect_cons( n );
		} );

		_ntk.clear_values2();
	}
	else
	{
		/* Highlight nodes that are either the TFI or the TFI_1 of POs. */
		_ntk.clear_values2();
		_ntk.foreach_po( [&]( signal const& f ) {
			const node n = _ntk.get_node( f );
			mark_tfi_rec_k_reg( n, 0u, 1u );
		} );

		/* Apply ALAP level computation (indeed, ASAP in a reverse order) 
		   to the highlighted nodes */
		_levels_alap.reset( 0u );
		_ntk.incr_trav_id();
		_ntk.foreach_node( [&]( node const& n ) {
			uint32_t node_level = compute_level_alap_rec_detect_cons( n );
		} );

		_ntk.clear_values2();
	}
}

void collect_tfo_dfs_rec( node const& n, std::vector<node>& tfo )
{
	if ( _ntk.is_register( n ) || _ntk.visited( n ) == _ntk.trav_id() )
	{
		return;
	}

	_ntk.set_visited( n, _ntk.trav_id() );

	_ntk.foreach_fanout( n, [&]( node const& no ) {
		if ( !( _ntk.visited( no ) == _ntk.trav_id() ) )
		{
			collect_tfo_dfs_rec( no, tfo );
		}
	} );

	tfo.push_back( n );
}

void collect_tfi_dfs_rec( node const& n, std::vector<node>& tfi )
{
	if ( _ntk.is_register( n ) || _ntk.visited( n ) == _ntk.trav_id() )
	{
		return;
	}

	_ntk.set_visited( n, _ntk.trav_id() );

	_ntk.foreach_fanin( n, [&]( signal const& f ) {
		const node ni = _ntk.get_node( f );
		if ( !( _ntk.visited( ni ) == _ntk.trav_id() ) )
		{
			collect_tfi_dfs_rec( ni, tfi );
		}
	} );

	tfi.push_back( n );
}

void collect_cut_backward_rec( node const& n, std::vector<node>& cut )
{
	if ( _ntk.visited( n ) == _ntk.trav_id() )
	{
		return;
	}

	_ntk.set_visited( n, _ntk.trav_id() );

	if ( _ntk.value( n ) )
	{
		cut.push_back( n );
		return;
	}

	_ntk.foreach_fanin( n, [&]( signal const& f ) {
		const node ni = _ntk.get_node( f );
		if ( _ntk.is_constant( ni ) )
		{
			return true;
		}
		collect_cut_backward_rec( ni, cut );
		return true;
	} );
}

/* For combinational paths that either start at a PI or end at a PO
   they feature that one end of the path is fixed during retiming.
   This results in two frontiers of nodes that the forward retiming
   and the backward retiming shall not pass.
   These nodes are termed "hard constraints."
   This function detects hard constraints, mark their value to one. */
template<bool forward>
void add_hard_conservative_constraints()
{
	_ntk.incr_trav_id();
	std::vector<node> nodes{};
	nodes.reserve( _ntk.num_gates() );
	_ntk.clear_values2();

	if constexpr ( forward )
	{
		/* Obtain the TFO of PIs. */
		_ntk.foreach_pi( [&]( node const& n ) {
			collect_tfo_dfs_rec( n, nodes );
		} );

		_levels_asap.reset( 0u );
		for ( auto it{ nodes.rbegin() }; it != nodes.rend(); ++it )
		{
			node n = *it;
			if ( _ntk.is_pi( n ) )
			{
				continue;
			}

			uint32_t node_level{ 0u };
			_ntk.foreach_fanin( n, [&]( signal const& f ) {
				const node ni = _ntk.get_node( f );
				node_level = std::max( node_level, _levels_asap[ni] );
			} );
			node_level += _cost_fn( _ntk, n );
			_levels_asap[n] = node_level;
			assert( node_level <= _delay_cons );

			if ( _ntk.is_box_input( n ) )
			{
				_ntk.set_value2( n, 1u );
			}
		}

		nodes.clear();
		nodes.reserve( _ntk.num_gates() );
		_ntk.incr_trav_id();
		_ntk.foreach_register( [&]( node const& n ) {
			const node n_bi = _ntk.get_node( _ntk.get_fanin0( n ) );
			const node n_bo = _ntk.fanout( n )[0];
			_ntk.set_visited( n, _ntk.trav_id() );
			collect_tfo_dfs_rec( n_bo, nodes );

			if ( _ntk.value2( n_bi ) )
			{
				/* The current register is the first one. The TFO of this register */
				/* serves as part of the TFO-1 of PIs. */
				_ntk.set_value2( n_bi, 0u );
				_levels_asap[n] = _levels_asap[n_bi];
				assert( _levels_asap[n] <= _delay_cons );
			}
		} );

		for ( auto it{ nodes.rbegin() }; it != nodes.rend(); ++it )
		{
			node n = *it;
			uint32_t node_level{ 0u };
			_ntk.foreach_fanin( n, [&]( signal const& f ) {
				const node ni = _ntk.get_node( f );
				node_level = std::max( node_level, _levels_asap[ni] );
			} );
			node_level += _cost_fn( _ntk, n );
			_levels_asap[n] = node_level;

			if ( node_level > _delay_cons )
			{
				/* Detect a hard-constraint node. */
				_ntk.set_value( n, 1u );
				std::cout << fmt::format( "[m] Node {} is a hard constraint ", ( _ntk.node_to_index( n ) - 2 ) );
				std::cout << fmt::format( "because {} > {}.\n", node_level, _delay_cons );
			}
		}

		_ntk.foreach_register( [&]( node const& n ) {
			_levels_asap[n] = 0u;
		});
		for ( auto it{ nodes.rbegin() }; it != nodes.rend(); ++it )
		{
			node n = *it;

			uint32_t node_level{ 0u };
			_ntk.foreach_fanin( n, [&]( signal const& f ) {
				const node ni = _ntk.get_node( f );
				node_level = std::max( node_level, _levels_asap[ni] );
			} );
			node_level += _cost_fn( _ntk, n );
			_levels_asap[n] = node_level;
			assert( node_level <= _delay_cons );

			if ( _ntk.is_box_input( n ) )
			{
				_ntk.set_value2( n, 1u );
			}
		}

		_ntk.foreach_register( [&]( node const& n ) {
			const node n_bi = _ntk.get_node( _ntk.get_fanin0( n ) );
			const node n_bo = _ntk.fanout( n )[0];
			_ntk.set_visited( n, _ntk.trav_id() );
			if ( _ntk.value2( n_bi ) )
			{
				/* There exists a register before the current one.
				   Thus, nodes on the path starting from the current register have the potential
				   to impose a conservative constraint. */
				_ntk.set_value2( n_bi, 0u );
				_levels_asap[n] = _levels_asap[n_bi];
				assert( _levels_asap[n] <= _delay_cons );
			}
		} );

		for ( auto it{ nodes.rbegin() }; it != nodes.rend(); ++it )
		{
			node n = *it;
			uint32_t node_level{ 0u };
			_ntk.foreach_fanin( n, [&]( signal const& f ) {
				const node ni = _ntk.get_node( f );
				node_level = std::max( node_level, _levels_asap[ni] );
			} );
			node_level += _cost_fn( _ntk, n );
			_levels_asap[n] = node_level;

			if ( node_level > _delay_cons )
			{
				/* Detect a conservative-constraint node. */
				if ( _ntk.value( n ) != 1u )
				{
					_ntk.set_value( n, 2u );
					std::cout << fmt::format( "[m] Node {} is a conservative constraint ", ( _ntk.node_to_index( n ) - 2 ) );
					std::cout << fmt::format( "because {} > {}.\n", node_level, _delay_cons );
				}
			}
		}

		_ntk.clear_values2();
		_levels_asap.reset( 0u );
	}

	else
	{
		/* Obtain the TFI of POs. */
		_ntk.foreach_po( [&]( node const& n ) {
			collect_tfi_dfs_rec( n, nodes );
		} );

		_levels_alap.reset( 0u );
		for ( auto it{ nodes.rbegin() }; it != nodes.rend(); ++it )
		{
			node n = *it;
			if ( _ntk.is_po( n ) )
			{
				continue;
			}

			uint32_t node_level{ 0u };
			_ntk.foreach_fanout( n, [&]( node const& no ) {
				node_level = std::max( node_level, _levels_alap[no] );
			} );
			node_level += _cost_fn( _ntk, n );
			_levels_alap[n] = node_level;
			assert( node_level <= _delay_cons );

			if ( _ntk.is_box_output( n ) )
			{
				_ntk.set_value2( n, 1u );
			}
		}

		nodes.clear();
		nodes.reserve( _ntk.num_gates() );
		_ntk.incr_trav_id();
		_ntk.foreach_register( [&]( node const& n ) {
			const node n_bi = _ntk.get_node( _ntk.get_fanin0( n ) );
			const node n_bo = _ntk.fanout( n )[0];
			_ntk.set_visited( n, _ntk.trav_id() );
			collect_tfi_dfs_rec( n_bi, nodes );
			if ( _ntk.value2( n_bo ) )
			{
				/* The current register is the first one. The TFO of this register */
				/* is part of the TFO-1 of PIs. */
				_ntk.set_value2( n_bo, 0u );
				_levels_alap[n] = _levels_alap[n_bo];
				assert( _levels_alap[n] <= _delay_cons );
			}
		} );

		for ( auto it{ nodes.rbegin() }; it != nodes.rend(); ++it )
		{
			node n = *it;
			uint32_t node_level{ 0u };
			_ntk.foreach_fanout( n, [&]( node const& no ) {
				node_level = std::max( node_level, _levels_alap[no] );
			} );
			node_level += _cost_fn( _ntk, n );
			_levels_alap[n] = node_level;

			if ( node_level > _delay_cons )
			{
				/* Detect a hard-constraint node. */
				_ntk.set_value( n, 1u );
				std::cout << fmt::format( "[m] Node {} is a hard constraint ", ( _ntk.node_to_index( n ) - 2 ) );
				std::cout << fmt::format( "because {} > {}.\n", node_level, _delay_cons );
			}
		}

		_ntk.foreach_register( [&]( node const& n ) {
			_levels_alap[n] = 0u;
		});
		for ( auto it{ nodes.rbegin() }; it != nodes.rend(); ++it )
		{
			node n = *it;

			uint32_t node_level{ 0u };
			_ntk.foreach_fanout( n, [&]( node const& no ) {
				node_level = std::max( node_level, _levels_alap[no] );
			} );
			node_level += _cost_fn( _ntk, n );
			_levels_alap[n] = node_level;
			assert( node_level <= _delay_cons );

			if ( _ntk.is_box_output( n ) )
			{
				_ntk.set_value2( n, 1u );
			}
		}

		_ntk.foreach_register( [&]( node const& n ) {
			const node n_bi = _ntk.get_node( _ntk.get_fanin0( n ) );
			const node n_bo = _ntk.fanout( n )[0];
			_ntk.set_visited( n, _ntk.trav_id() );
			if ( _ntk.value2( n_bo ) )
			{
				/* There exists a register after the current one.
				   Thus, nodes on the path ending at the current register have the potential
				   to impose a conservative constraint. */
				_ntk.set_value2( n_bo, 0u );
				_levels_alap[n] = _levels_alap[n_bo];
				assert( _levels_alap[n] <= _delay_cons );
			}
		} );

		for ( auto it{ nodes.rbegin() }; it != nodes.rend(); ++it )
		{
			node n = *it;
			uint32_t node_level{ 0u };
			_ntk.foreach_fanout( n, [&]( node const& no ) {
				node_level = std::max( node_level, _levels_alap[no] );
			} );
			node_level += _cost_fn( _ntk, n );
			_levels_alap[n] = node_level;

			if ( node_level > _delay_cons )
			{
				/* Detect a conservative-constraint node. */
				if ( _ntk.value( n ) != 1u )
				{
					_ntk.set_value( n, 2u );
					std::cout << fmt::format( "[m] Node {} is a conservative constraint ", ( _ntk.node_to_index( n ) - 2 ) );
					std::cout << fmt::format( "because {} > {}.\n", node_level, _delay_cons );
				}
			}
		}

		_ntk.clear_values2();
		_levels_alap.reset( 0u );
	}
}

/* Use the "conserv_cons" signal to control if keeping both the hard and 
   the conservative constraints active */
template<bool conserv_cons>
uint32_t max_flow_rec_forward( node const& n )
{
	if ( _ntk.visited( n ) == _ntk.trav_id() )
	{
		return 0u;
	}

	_ntk.set_visited( n, _ntk.trav_id() );

	/* Handle exact constraints. */
	if ( _exact_cons[n].size() )
	{
		// std::cout << "[m] Node " << _ntk.node_to_index( n ) << " has exact constraints.\n";
		for ( node const& exact_con : _exact_cons[n] )
		{
			if ( !max_flow_rec_forward<conserv_cons>( exact_con ) )
			{
				return 0u;
			}
		}
	}

	/* The current node is not yet in a flow path. */
	uint32_t found_path{ 0u };
	if ( _flow_path[n] == 0u )
	{
		if constexpr ( conserv_cons )
		{
			if ( _ntk.value( n ) )
			{
				_flow_path[n] = sink;
				return 1u;
			}
		}
		else
		{
			if ( _ntk.value( n ) & 1 )
			{
				_flow_path[n] = sink;
				return 1u;
			}
		}
		
		_ntk.foreach_fanout( n, [&]( node const& no ) {
			if ( max_flow_rec_forward<conserv_cons>( no ) )
			{
				_flow_path[n] = _ntk.node_to_index( no );
				found_path = 1u;
				return false;
			}
			return true;
		} );

		return found_path;
	}

	/* The current node is already in a flow path. */
	/* Check if the previous node in the current flow path has another flow path */
	node const0{ _ntk.get_node( _ntk.get_constant( false ) ) };
	node ni{ const0 };
	_ntk.foreach_fanin( n, [&]( signal const& f ) {
		const node n_i = _ntk.get_node( f );
		if ( _ntk.is_constant( n_i ) )
		{
			return true;
		}
		if ( _flow_path[n_i] == _ntk.node_to_index( n ) )
		{
			ni = n_i;
			return false;
		}
		return true;
	} );
	if ( ni == const0 )
	{
		return 0u;
	}

	_ntk.foreach_fanout( ni, [&]( node const& nio ) {
		if ( max_flow_rec_forward<conserv_cons>( nio ) )
		{
			_flow_path[ni] = _ntk.node_to_index( nio );
			found_path = 1u;
			return false;
		}
		return true;
	} );

	if ( found_path )
	{
		return 1u;
	}

	if ( max_flow_rec_forward<conserv_cons>( ni ) )
	{
		_flow_path[ni] = 0u;
		return 1u;
	}

	return 0u;
}

template<bool conserv_cons>
uint32_t max_flow_rec_backward( node const& n )
{
	if ( _ntk.visited( n ) == _ntk.trav_id() )
	{
		return 0u;
	}

	_ntk.set_visited( n, _ntk.trav_id() );

	/* Handle exact constraints. */
	if ( _exact_cons[n].size() )
	{
		// std::cout << "[m] Has exact constraints!\n";
		for ( node const& exact_con : _exact_cons[n] )
		{
			if ( !max_flow_rec_backward<conserv_cons>( exact_con ) )
			{
				return 0u;
			}
		}
	}

	/* The current node is not yet in a flow path. */
	uint32_t found_path{ 0u };
	if ( _flow_path[n] == 0u )
	{
		if constexpr ( conserv_cons )
		{
			if ( _ntk.value( n ) )
			{
				_flow_path[n] = sink;
				return 1u;
			}
		}
		else
		{
			if ( _ntk.value( n ) & 1 )
			{
				_flow_path[n] = sink;
				return 1u;
			}
		}

		_ntk.foreach_fanin( n, [&]( signal const& f ) {
			const node ni = _ntk.get_node( f );
			if ( _ntk.is_constant( ni ) )
			{
				return true;
			}
			if ( max_flow_rec_backward<conserv_cons>( ni ) )
			{
				_flow_path[n] = _ntk.node_to_index( ni );
				found_path = 1u;
				return false;
			}
			return true;
		} );

		return found_path;
	}

	node const0{ _ntk.get_node( _ntk.get_constant( false ) ) };
	node no{ const0 };
	_ntk.foreach_fanout( n, [&]( node const& n_o ) {
		if ( _flow_path[n_o] == _ntk.node_to_index( n ) )
		{
			no = n_o;
			return false;
		}
		return true;
	} );

	if ( no == const0 )
	{
		return 0u;
	}

	_ntk.foreach_fanin( no, [&]( signal const& f ) {
		const node noi = _ntk.get_node( f );
		if ( _ntk.is_constant( noi ) )
		{
			return true;
		}
		if ( max_flow_rec_backward<conserv_cons>( noi ) )
		{
			_flow_path[no] = _ntk.node_to_index( noi );
			found_path = 1u;
			return false;
		}
		return true;
	} );

	if ( found_path )
	{
		return 1u;
	}

	if ( max_flow_rec_backward<conserv_cons>( no ) )
	{
		_flow_path[no] = 0u;
		return 1u;
	}

	return 0u;
}

void add_exact_constraints_forward( node const& n )
{
	std::vector<node> nodes{}, exact_cons{};
	nodes.reserve( _ntk.num_gates() );
	exact_cons.reserve( _ntk.num_gates() );

	std::function<void( node const&, bool )> collect_fanins = [&]( node const& n, bool encount_reg ) {
		if ( _ntk.visited( n ) == _ntk.trav_id() )
		{
			return;
		}

		_ntk.set_visited( n, _ntk.trav_id() );

		if ( _ntk.is_register( n ) )
		{
			if ( encount_reg )
			{
				return;
			}
			encount_reg = true;
		}

		_ntk.foreach_fanin( n, [&]( signal const& f ) {
			const node ni{ _ntk.get_node( f ) };
			collect_fanins( ni, encount_reg );
		} );

		nodes.push_back( n );
	};

	_ntk.incr_trav_id();
	_levels_alap.reset( 0u );
	collect_fanins( n, false );

	for ( auto it{ nodes.rbegin() }; it != nodes.rend(); ++it )
	{
		node nt = *it;
		if ( !_ntk.is_register( nt ) )
		{
			_ntk.foreach_fanin( nt, [&]( signal const& f ) {
				const node nti = _ntk.get_node( f );
				_levels_alap[nti] = std::max( _levels_alap[nti], _levels_alap[nt] + ( _cost_fn( _ntk, nt ) ) );
			} );

			if ( _levels_alap[nt] == _delay_cons )
			{
				exact_cons.push_back( nt );
				// std::cout << "[m] An exact constraint is detected\n";
			}
		}
	}

	_levels_alap.reset( 0u );
	_exact_cons[n] = exact_cons;
}

void add_exact_constraints_backward( node const& n )
{

	std::vector<node> nodes{}, exact_cons{};
	nodes.reserve( _ntk.num_gates() );
	exact_cons.reserve( _ntk.num_gates() );
	
	std::function<void( node const&, bool )> collect_fanouts = [&]( node const& n, bool encount_reg ) {
		if ( _ntk.visited( n ) == _ntk.trav_id() )
		{
			return;
		}

		_ntk.set_visited( n, _ntk.trav_id() );

		if ( _ntk.is_register( n ) )
		{
			if ( encount_reg )
			{
				return;
			}
			encount_reg = true;
		}

		_ntk.foreach_fanout( n, [&]( node const& no ) {
			collect_fanouts( no, encount_reg );
		} );

		nodes.push_back( n );
	};

	_ntk.incr_trav_id();
	_levels_asap.reset( 0u );
	collect_fanouts( n, false );

	for ( auto it{ nodes.rbegin() }; it != nodes.rend(); ++it )
	{
		node nt = *it;
		if ( !_ntk.is_register( nt ) )
		{
			_ntk.foreach_fanout( nt, [&]( node const& nto ) {
				_levels_asap[nto] = std::max( _levels_alap[nto], _levels_alap[nt] + ( _cost_fn( _ntk, nt ) ) );
			} );

			if ( _levels_asap[nt] == _delay_cons )
			{
				exact_cons.push_back( nt );
			}
		}
	}

	_levels_asap.reset( 0u );
	_exact_cons[n] = exact_cons;
}

template<bool forward>
void add_exact_constraints( node const& n )
{
	if constexpr ( forward )
	{
		add_exact_constraints_forward( n );
	}
	else
	{
		add_exact_constraints_backward( n );
	}
}

template<bool forward>
bool max_flow_under_cons()
{
	bool constraints_updated{ false };
	uint32_t flow{ 0u };
	uint32_t trav_id_init{ _ntk.trav_id() };
	_flow_path.reset();
	_ntk.incr_trav_id();
	int cnt{ 1u };

	std::vector<node> conserv_cons_nodes{};
	conserv_cons_nodes.reserve( _ntk.num_gates() );


	/* Compute max-flow under hard, conservative, and exact constraints */
	_ntk.foreach_register( [&]( node const& n ) {
		// if ( _ntk.value( n ) == 2u )
		// {
		// 	conserv_cons_nodes.push_back( n );
		// 	// std::cout << "[m] There exist conservative constraints!\n";
		// }

		uint32_t local_flow{ 0u };
		if constexpr ( forward )
		{
			local_flow = max_flow_rec_forward<true>( _ntk.fanout( n )[0] );
		}
		else
		{
			local_flow = max_flow_rec_backward<true>( _ntk.get_node( _ntk.get_fanin0( n ) ) );
		}

		if ( local_flow )
		{
			_ntk.incr_trav_id();


			std::cout << fmt::format( "[m] Flow paths in the {}th iteration:\n", cnt++ );
      print_current_flow_path( _ntk, _flow_path, true );


			flow += local_flow;
		}
		print_current_info( _ntk );


		return true;
	} );

	_ntk.incr_trav_id();
	_ntk.foreach_register( [&]( node const& n ) {
		uint32_t local_flow{ 0u };
		if constexpr ( forward )
		{
			local_flow = max_flow_rec_forward<true>( _ntk.fanout( n )[0] );
		}
		else
		{
			local_flow = max_flow_rec_backward<true>( _ntk.get_node( _ntk.get_fanin0( n ) ) );
		}

		assert( local_flow == 0u );
	} );

	/* mark the nodes on flow paths */
	// std::cout << "[m] Untouched nodes: ";


	_ntk.clear_values2();
	_ntk.foreach_node( [&]( node const& n ) {
		if ( _ntk.visited( n ) > trav_id_init )
		{
			_ntk.set_value2( n, 1u );
		}
		// else
		// {
		// 	std::cout << fmt::format( "node {}, ", ( _ntk.node_to_index( n ) - 2 ) );
		// }
	} );

	// std::cout << '\n';


	std::cout << "[m] The (fake) eventual flow paths and trav_id:\n";
  print_current_flow_path( _ntk, _flow_path, true );
  print_current_info( _ntk );

	conserv_cons_nodes.clear();
	if ( conserv_cons_nodes.empty() )
	{
	// std::cout << "[m] Without conservative constrains detected\n";
		return false;
	}


	/* Compute max-flow under hard and exact constraints only */
	_flow_path.reset();
	trav_id_init = _ntk.trav_id();
	_ntk.incr_trav_id();
	flow = 0u;

	// _ntk.foreach_register( [&]( node const& n ) {
	// 	uint32_t local_flow{ 0u };
	// 	if constexpr ( forward )
	// 	{
	// 		local_flow = max_flow_rec_forward<false>( _ntk.fanout( n )[0] );
	// 	}
	// 	else
	// 	{
	// 		local_flow = max_flow_rec_backward<false>( _ntk.get_node( _ntk.get_fanin0( n ) ) );
	// 	}

	// 	if ( local_flow > 0u && std::find( conserv_cons_nodes.begin(), conserv_cons_nodes.end(), n ) != conserv_cons_nodes.end() )
	// 	{
	// 		add_exact_constraints<forward>( n );
	// 		constraints_updated = true;
	// 		_ntk.incr_trav_id();
	// 	}

	// 	return true;
	// } );

	_ntk.foreach_register( [&]( node const& n ) {
		uint32_t local_flow{ 0u };
		if constexpr ( forward )
		{
			local_flow = max_flow_rec_forward<false>( _ntk.fanout( n )[0] );
		}
		else
		{
			local_flow = max_flow_rec_backward<false>( _ntk.get_node( _ntk.get_fanin0( n ) ) );
		}

		if ( local_flow )
		{
			_ntk.incr_trav_id();


			std::cout << fmt::format( "[m] Flow paths in the {}th iteration:\n", cnt++ );
      print_current_flow_path( _ntk, _flow_path, true );


			flow += local_flow;
		}
		// print_current_info( _ntk );


		return true;
	} );

	_ntk.incr_trav_id();
	_ntk.foreach_register( [&]( node const& n ) {
		uint32_t local_flow{ 0u };
		if constexpr ( forward )
		{
			local_flow = max_flow_rec_forward<false>( _ntk.fanout( n )[0] );
		}
		else
		{
			local_flow = max_flow_rec_backward<false>( _ntk.get_node( _ntk.get_fanin0( n ) ) );
		}

		assert( local_flow == 0u );
	} );


	std::cout << "[m] The (real) eventual flow paths and trav_id after exact cons computation:\n";
  print_current_flow_path( _ntk, _flow_path, true );


	_ntk.foreach_node( [&]( node const& n ) {
		if ( !_ntk.value2( n ) &&
		     _ntk.visited( n ) > trav_id_init &&
		     _ntk.value( n ) == 2u )
		{
			_ntk.set_value( n, 4u );
			add_exact_constraints<forward>( n );
			std::cout << fmt::format( "[m] Node {} have exact constraints: ", ( _ntk.node_to_index( n ) - 2 ) );
			for ( const node n_e : _exact_cons[n] )
			{
				std::cout << fmt::format( "node {}, ", _ntk.node_to_index( n_e ) );
			}
			constraints_updated = true;
		}
	} );


	return constraints_updated;
	// return false;
}

std::vector<node> get_min_cut()
{
	std::vector<node> min_cut;
	min_cut.reserve(_ntk.num_registers());

	_ntk.foreach_node([&](auto const& n) {
		if ( _flow_path[n] == 0u )
		{
			return true;
		}

		if ( _ntk.visited( n ) != _ntk.trav_id() )
		{
			return true;
		}

		if ( ( _ntk.value( n ) & 1 ) || ( _ntk.visited( _flow_path[n] ) != _ntk.trav_id() ))
		{
			min_cut.push_back( n );
		}

		return true;
	});

	return min_cut;
}

template<bool forward>
void legalize_retiming( std::vector<node>& min_cut )
{
  _ntk.clear_values();

  _ntk.foreach_register( [&]( node const& n ) {
    _ntk.set_value( _ntk.fanout( n )[0], 1u );
  } );

  for ( node const& n : min_cut )
  {
    mark_tfi_rec( n );
  }

  min_cut.clear();

  if constexpr ( forward )
  {
    _ntk.foreach_gate( [&]( node const& n ) {
      if ( _ntk.value( n ) )
      {
        /* if is sink or before a register */
        _ntk.foreach_fanout( n, [&]( node const& no ) {
          if ( !_ntk.value( no ) )
          {
          	min_cut.push_back( n );
            return false;
          }
          return true;
        } );
      }
    } );
  }
  else
  {
    _ntk.incr_trav_id();
    _ntk.foreach_register( [&]( node const& n ) {
      node ni = _ntk.get_node( _ntk.get_fanin0( n ) );
      collect_cut_backward_rec( ni, min_cut );
      return true;
    } );

    _ntk.foreach_node( [&]( node const& n ) {
      if ( _ntk.visited( n ) == _ntk.trav_id() )
      {
        _ntk.set_value( n, 1u );
      }
      else
      {
        _ntk.set_value( n, 0u );
      }
    } );
    for ( auto const& n : min_cut )
    {
      _ntk.set_value( n, 0u );
    }
  }
}

template<bool forward>
void update_registers_position( std::vector<node> const& min_cut )
{
  _ntk.incr_trav_id();

  /* create new registers and mark the ones to reuse */
  for ( node const& n : min_cut )
  {
    if constexpr ( forward )
    {
      if ( _ntk.is_box_output( n ) )
      {
        /* reuse the current register */
        node node_register = _ntk.get_node( _ntk.get_fanin0( n ) );
        node in_register = _ntk.get_node( _ntk.get_fanin0( node_register ) );
        node in_in_register = _ntk.get_node( _ntk.get_fanin0( in_register ) );

        /* check for marked fanouts to connect to register input */
        std::vector<node> nos = _ntk.fanout( n );
        for ( auto const& no : nos )
        {
          if ( _ntk.value( no ) )
          {
            _ntk.replace_in_node( no, n, in_in_register );
            _ntk.decr_fanout_size( n );
          }
        }

        _ntk.set_visited( node_register, _ntk.trav_id() );
      }
      else
      {
        /* create a new register */
        const node in_register = _ntk.create_box_input( _ntk.make_signal( n ) );
        const node node_register = _ntk.create_register( in_register );
        const node node_register_out = _ntk.create_box_output( node_register );

        /* replace in n fanout */
        std::vector<node> nos = _ntk.fanout( n );
        for ( node const& no : nos )
        {
          if ( no != _ntk.get_node( in_register ) && !_ntk.value( no ) )
          {
            _ntk.replace_in_node( no, n, node_register_out );
            _ntk.decr_fanout_size( n );
          }
        }

        _ntk.set_visited( _ntk.get_node( node_register ), _ntk.trav_id() );
      }
    }
    else
    {
      if ( _ntk.is_box_input( n ) )
      {
        _ntk.foreach_fanout( n, [&]( node const& no ) {
          _ntk.set_visited( no, _ntk.trav_id() );
        } );
      }
      else
      {
        /* create a new register */
        const node in_register = _ntk.create_box_input( _ntk.make_signal( n ) );
        const node node_register = _ntk.create_register( in_register );
        const node node_register_out = _ntk.create_box_output( node_register );

        /* replace in n fanout */
        std::vector<node> nos = _ntk.fanout( n );
        for ( node const& no : nos )
        {
          if ( no != _ntk.get_node( in_register ) && _ntk.value( no ) )
          {
            _ntk.replace_in_node( no, n, node_register_out );
            _ntk.decr_fanout_size( n );
          }
        }

        _ntk.set_visited( _ntk.get_node( node_register ), _ntk.trav_id() );
      }
    }
  }

  /* remove retimed registers */
  
  _ntk.foreach_register( [&]( node const& n ) {
    if ( _ntk.visited( n ) == _ntk.trav_id() )
    {
      return true;
    }

    node node_register_out = _ntk.fanout( n )[0];
    node node_register_in = _ntk.get_node( _ntk.get_fanin0( n ) );
    signal node_register_in_in = _ntk.get_fanin0( node_register_in );

    // _ntk.foreach_fanout( n, [&]( node const& no ) {
    //   node_register_out = no;
    // } );

    // auto node_register_fanout = _ntk.fanout_size( node_register_out );
    // auto fanin_fanout = _ntk.fanout_size( _ntk.get_node( node_register_in_in ) );
    // auto fanin_type = _ntk.is_box_output( _ntk.get_node( node_register_in_in ) );

    _ntk.substitute_node( node_register_out, node_register_in_in );

    return true;
  } );
}

template<bool forward>
bool operate_retime()
{
	const uint32_t num_reg_pre = _ntk.num_registers();
	init_value<forward>();
	std::cout << "[m] Initialization finished\n";
	add_hard_conservative_constraints<forward>();


	std::cout << "[m] Nodes whose value is 1: { ";
    _ntk.foreach_node( [&]( auto const& n ) {
      if ( _ntk.value( n ) == 1u )
      {
        if ( _ntk.is_pi( n ) )
        {
          std::cout << "pi ";
        }
        else if ( _ntk.is_po( n ) )
        {
          std::cout << fmt::format( "po{} ", ( n - _ntk.num_gates() - _ntk.num_pis() - 1 ) );
        }
        else
        {
          std::cout << fmt::format( "n{} ", n - 2 );
        }
      }
    } );
    std::cout << "}\n";


	std::cout << "[m] Conservative constraints loaded\n";
	bool constraints_updated{ true };
	uint32_t cnt{ 1u };
	while ( constraints_updated )
	{
		constraints_updated = max_flow_under_cons<forward>();
		std::cout << fmt::format( "[m] Exact constraints loaded ( {}-th round )\n", cnt++ );
	}
	std::vector<node> min_cut{};
	min_cut = get_min_cut();


	// std::cout << "[m] Before legalizing retiming:\n";
  // print_min_cut( _ntk, min_cut );


	legalize_retiming<forward>( min_cut );


	std::cout << "[m] After legalizing retiming:\n";
  print_min_cut( _ntk, min_cut );

	if ( _ps.verbose )
  {
    float num_reg_improve = static_cast<float>( num_reg_pre - min_cut.size() ) / num_reg_pre * 100;
    std::cout << fmt::format( "[i] Retiming {}\t pre = {:7d}\t post = {:7d}\t improvement = {:>5.2f}%\n",
                              forward ? "forward" : "backward", num_reg_pre, min_cut.size(), num_reg_improve );
  }

  if ( min_cut.size() >= num_reg_pre )
  {
  	return false;
  }

	update_registers_position<forward>( min_cut );
	print_reg_pos( _ntk );
	_delay = 0u;
	compute_levels_asap();
	_pst->delay = _delay;

	return true;
}

private:
	Ntk& _ntk;
	uint32_t _delay_cons;
	area_retime_params const& _ps;
	area_retime_stats* _pst;
	node_map<uint32_t, Ntk> _flow_path;
	node_map<uint32_t, Ntk> _levels_asap;
	node_map<uint32_t, Ntk> _levels_alap;
	node_map<std::vector<node>, Ntk> _exact_cons;
	NodeCostFn _cost_fn;
	uint32_t _delay{};
};

} /* namespace detail */

template<class Ntk = generic_network, class NodeCostFn = unit_cost<fanout_view<generic_network>>>
void area_retime( Ntk& ntk, NodeCostFn const& cost_fn = {}, area_retime_params const& ps = {}, area_retime_stats* pst = nullptr )
{
	area_retime_stats st;
	fanout_view<Ntk> ntk_fanout{ ntk };
	detail::area_retime_impl impl( ntk_fanout, cost_fn, ps, &st );
	impl.run();

	if ( ps.verbose )
	{
		st.report();
	}

	if ( pst )
	{
		*pst = st;
	}
}

} /* namespace mockturtle */