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

/*!
  \file x1g_npn.hpp
  \brief Replace with size-optimum X1Gs from NPN (from ABC rewrite)

  \author Heinz Riener
  \author Mathias Soeken
  \author Shubham Rai
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>
#include <iomanip>

#include <fmt/format.h>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/npn.hpp>
#include <kitty/print.hpp>
#include <kitty/static_truth_table.hpp>

#include "../../algorithms/simulation.hpp"
#include "../../io/write_bench.hpp"
#include "../../networks/x1g.hpp"
#include "../../utils/node_map.hpp"
#include "../../utils/stopwatch.hpp"
#include "../../views/topo_view.hpp"

namespace mockturtle
{

struct x1g_npn_resynthesis_params
{
  /*! \brief Be verbose. */
  bool verbose{ false };
};

struct x1g_npn_resynthesis_stats
{
  stopwatch<>::duration time_classes{ 0 };
  stopwatch<>::duration time_db{ 0 };

  uint32_t db_size;
  uint32_t covered_classes;

  void report() const
  {
    std::cout << fmt::format( "[i] build classes time = {:>5.2f} secs\n", to_seconds( time_classes ) );
    std::cout << fmt::format( "[i] build db time      = {:>5.2f} secs\n", to_seconds( time_db ) );
  }
};

/*! \brief Resynthesis function based on pre-computed X1Gs.
 *
 * This resynthesis function can be passed to ``cut_rewriting``.  It will
 * produce a network based on pre-computed X1G with up to at most 4 variables.
 * Consequently, the nodes' fan-in sizes in the input network must not exceed
 * 4.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      const x1g_network x1g = ...;
      x1g_npn_resynthesis<x1g_network> x1g_resyn;
      x1g = cut_rewriting( x1g, x1g_resyn );

   .. note::

      The implementation of this algorithm was heavily inspired by the rewrite
      command in AIG.  It uses the same underlying database of subcircuits.
   \endverbatim
 */
template<class Ntk, class DatabaseNtk = x1g_network>
class x1g_npn_resynthesis
{
public:
  x1g_npn_resynthesis( x1g_npn_resynthesis_params const& ps = {}, x1g_npn_resynthesis_stats* pst = nullptr )
      : ps( ps ),
        pst( pst ),
        _repr( 1u << 16u )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
    static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and method" );
    static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_xor method" );
    static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );

    static_assert( is_network_type_v<DatabaseNtk>, "DatabaseNtk is not a network type" );
    static_assert( has_get_node_v<DatabaseNtk>, "DatabaseNtk does not implement the get_node method" );
    static_assert( has_is_complemented_v<DatabaseNtk>, "DatabaseNtk does not implement the is_complemented method" );
    static_assert( has_is_xor_v<DatabaseNtk>, "DatabaseNtk does not implement the is_xor method" );
    static_assert( has_size_v<DatabaseNtk>, "DatabaseNtk does not implement the size method" );
    static_assert( has_create_pi_v<DatabaseNtk>, "DatabaseNtk does not implement the create_pi method" );
    static_assert( has_create_and_v<DatabaseNtk>, "DatabaseNtk does not implement the create_and method" );
    static_assert( has_create_xor_v<DatabaseNtk>, "DatabaseNtk does not implement the create_xor method" );
    static_assert( has_foreach_fanin_v<DatabaseNtk>, "DatabaseNtk does not implement the foreach_fanin method" );
    static_assert( has_foreach_node_v<DatabaseNtk>, "DatabaseNtk does not implement the foreach_node method" );
    static_assert( has_make_signal_v<DatabaseNtk>, "DatabaseNtk does not implement the make_signal method" );

    build_classes();
    build_db();
  }

  virtual ~x1g_npn_resynthesis()
  {
    if ( ps.verbose )
    {
      st.report();
    }

    if ( pst )
    {
      *pst = st;
    }
  }

  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    kitty::static_truth_table<4u> tt = kitty::extend_to<4u>( function );

    /* get representative of function */
    const auto [repr, phase, perm] = _repr[*tt.cbegin()];

    /* check if representative has circuits */
    const auto it = _repr_to_signal.find( repr );
    assert( it != _repr_to_signal.end() );
    if ( it == _repr_to_signal.end() )
    {
      return;
    }

    std::vector<signal<Ntk>> pis( 4, ntk.get_constant( false ) );
    std::copy( begin, end, pis.begin() );

    std::unordered_map<node<DatabaseNtk>, signal<Ntk>> db_to_ntk;
    db_to_ntk.insert( { 0, ntk.get_constant( false ) } );
    for ( auto i = 0; i < 4; ++i )
    {
      db_to_ntk.insert( { i + 1, ( phase >> perm[i] & 1 ) ? ntk.create_not( pis[perm[i]] ) : pis[perm[i]] } );
    }

    for ( auto const& cand : it->second )
    {
      const auto f = copy_db_entry( ntk, _db.get_node( cand ), db_to_ntk );
      if ( !fn( _db.is_complemented( cand ) != ( phase >> 4 & 1 ) ? ntk.create_not( f ) : f ) )
      {
        return;
      }
    }
  }

private:
  signal<Ntk>
  copy_db_entry( Ntk& ntk, node<DatabaseNtk> const& n, std::unordered_map<node<DatabaseNtk>, signal<Ntk>>& db_to_ntk ) const
  {
    if ( const auto it = db_to_ntk.find( n ); it != db_to_ntk.end() )
    {
      return it->second;
    }

    std::array<signal<Ntk>, 3> fanin{};
    _db.foreach_fanin( n, [&]( auto const& f, auto i ) {
      const auto ntk_f = copy_db_entry( ntk, _db.get_node( f ), db_to_ntk );
      fanin[i] = _db.is_complemented( f ) ? ntk.create_not( ntk_f ) : ntk_f;
    } );

    const auto f = _db.is_xor3( n ) ? ntk.create_xor3( fanin[0], fanin[1], fanin[2] ) : ntk.create_onehot( fanin[0], fanin[1], fanin[2] );
    db_to_ntk.insert( { n, f } );
    return f;
  }

  void build_classes()
  {
    stopwatch t( st.time_classes );

    kitty::static_truth_table<4u> tt;
    do
    {
      _repr[*tt.cbegin()] = kitty::exact_npn_canonization( tt );
      kitty::next_inplace( tt );
    } while ( !kitty::is_const0( tt ) );
  }

  void build_db()
  {
    stopwatch t( st.time_db );

    _db.get_constant( false );
    /* four primary inputs */
    _db.create_pi();
    _db.create_pi();
    _db.create_pi();
    _db.create_pi();

    auto* p = subgraphs;
    while ( true )
    {
      auto entry0 = *p++;
      auto entry1 = *p++;
      auto entry2 = *p++;

      if ( entry0 == 0 && entry1 == 0 && entry2 == 0 )
      {
        break;
      }

      auto is_xor = entry0 & 1;
      entry0 >>= 1;

      const auto child0 = _db.make_signal( entry0 >> 1 ) ^ ( entry0 & 1 );
      const auto child1 = _db.make_signal( entry1 >> 1 ) ^ ( entry1 & 1 );
      const auto child2 = _db.make_signal( entry2 >> 1 ) ^ ( entry2 & 1 );

      if ( is_xor )
      {
        _db.create_xor3( child0, child1, child2 );
      }
      else
      {
        _db.create_onehot( child0, child1, child2 );
      }
    }

    const auto sim_res = simulate_nodes<kitty::static_truth_table<4u>>( _db );

    _db.foreach_node( [&]( auto n ) {
      if ( std::get<0>( _repr[*sim_res[n].cbegin()] ) == sim_res[n] )
      {
        if ( _repr_to_signal.count( sim_res[n] ) == 0 )
        {
          _repr_to_signal.insert( { sim_res[n], { _db.make_signal( n ) } } );
        }
        else
        {
          _repr_to_signal[sim_res[n]].push_back( _db.make_signal( n ) );
        }
      }
      else
      {
        const auto f = ~sim_res[n];
        if ( std::get<0>( _repr[*f.cbegin()] ) == f )
        {
          if ( _repr_to_signal.count( f ) == 0 )
          {
            _repr_to_signal.insert( { f, { !_db.make_signal( n ) } } );
          }
          else
          {
            _repr_to_signal[f].push_back( !_db.make_signal( n ) );
          }
        }
      }
    } );

    st.db_size = _db.size();
    st.covered_classes = static_cast<uint32_t>( _repr_to_signal.size() );
  }

  x1g_npn_resynthesis_params ps;
  x1g_npn_resynthesis_stats st;
  x1g_npn_resynthesis_stats* pst{ nullptr };

  std::vector<std::tuple<kitty::static_truth_table<4u>, uint32_t, std::vector<uint8_t>>> _repr;
  std::unordered_map<kitty::static_truth_table<4u>, std::vector<signal<DatabaseNtk>>, kitty::hash<kitty::static_truth_table<4u>>> _repr_to_signal;

  DatabaseNtk _db;

  // clang-format off
  inline static const uint16_t subgraphs[]
  {
    0x2,0x6,0x8,0x1,0x4,0xb,0x6,0x5,0xa,0x5,0xd,0xe,0x9,0x6,0x8,0xa,0x7,0x9,0x1,0x12,0x14,0x5,0x4,0x0,0xe,0x9,0x18,0xd,0x8,0x1a,0x31,0x0,0x1c,0x5,0x4,0x8,0x4,0x4,0x21,0xc,0x1,0x23,0x2,0x6,0x8,0x4,0x4,0x6,0x2,0x8,0x27,0x4,0x4,0x9,0x5,0x4,0x6,0xc,0x2b,0x2d,0x5,0x6,0x8,0x5,0x4,0x0,0x12,0x30,0x19,0x9,0x6,0x32,0x9,0x6,0x0,0x4,0x6,0x37,0x10,0x1,0x39,0x6,0x4,0x9,0x9,0x6,0x3c,0x4,0x6,0x3f,0x4,0x4,0x1,0xd,0x8,0x43,0x2,0x8,0x44,0x4,0x6,0x9,0x5,0x4,0x49,0x8,0x6,0x4a,0x9,0x6,0x8,0x2,0x8,0x13,0x8,0x6,0x9,0x5,0x4,0x6,0x4,0x50,0x2c,0x9,0x6,0x8,0x8,0x6,0x13,0x5,0x4,0x6,0x2,0x8,0x2c,0x4,0x5,0x7,0x2,0x8,0x58,0x5,0x4,0x0,0xc,0x9,0x19,0xd,0x18,0x5c,0x9,0x6,0x0,0x5,0x0,0x6,0x5,0x4,0x8,0x6e,0x61,0x21,0x6,0x7,0x9,0x9,0x8,0x64,0x8,0x64,0x67,0x5,0x6,0x0,0x8,0x8,0x60,0x4,0x6,0x6a,0xc1,0x0,0x6c,0x5,0x4,0x8,0x9,0x6,0x8,0x2,0x21,0x13,0x8,0x8,0x71,0x4,0x6,0x8,0x9,0x6,0x8,0x2,0x75,0x13,0x1,0x6,0x8,0x4,0x4,0x78,0xc,0x8,0x7b,0x4,0x6,0x8,0x6,0x4,0x74,0x5,0x4,0x7f,0x9,0x6,0x8,0x4,0x1,0x13,0xc,0x8,0x83,0x4,0x4,0x1,0xc,0x8,0x43,0x5,0x6,0x8,0x6,0x5,0x30,0x9,0x0,0x88,0xe,0x9,0x8b,0x9,0x6,0x0,0x4,0x8,0x37,0x5,0x0,0x6,0xc,0x8f,0x60,0x5,0x4,0x8,0x8,0x1,0x21,0xc,0x8,0x93,0x8,0x7,0x8,0x4,0x5,0x97,0x5,0x8,0x99,0x4,0x5,0x8,0x5,0x8,0x9d,0xc,0x8,0x9f,0x5,0x4,0x8,0xa,0x7,0x20,0x8,0x6,0x9,0x41,0xa2,0x51,0x8,0x6,0x9,0x6,0x5,0x7,0x5,0x51,0xa6,0x5,0x4,0x6,0x6,0x8,0x2d,0x8,0x6,0xab,0x5,0x4,0x6,0x6,0x5,0x7,0x12,0x2c,0xa7,0x8,0x6,0x8,0x4,0x8,0xb1,0x8,0x6,0x1,0x5,0x4,0x6,0x10,0xb5,0x2c,0x9,0x6,0x8,0x4,0x5,0x13,0x8,0x8,0xb9,0x4,0x4,0x7,0xc,0x8,0xbd,0x6,0x5,0x8,0x5,0x4,0x6,0xc,0xc1,0x2c,0x9,0x6,0x8,0x5,0x4,0x12,0x6,0x5,0x8,0x26,0xc5,0xc1,0x5,0x4,0x0,0x5,0x0,0x8,0x5,0x6,0x18,0x32,0xc8,0xcb,0x1,0x6,0xcc,0x9,0x6,0x0,0x5,0x8,0x0,0x5,0x4,0x0,0x6e,0xc8,0x19,0xd,0x8,0xd0,0x4,0x6,0x8,0x5,0x4,0x0,0x1,0x8,0x74,0x2,0x18,0xd4,0x1,0x4,0x8,0x6,0x6,0xd9,0x5,0x0,0x8,0x2,0xdb,0xc9,0x5,0x0,0x8,0x6,0x4,0x6,0x10,0xc9,0xdf,0x4,0x5,0x1,0x9,0x6,0xe2,0x5,0x8,0xe2,0x2,0xe4,0xe7,0x4,0x4,0x1,0x5,0x4,0x8,0xd,0x0,0x20,0x10,0x43,0xeb,0x5,0x4,0x8,0x5,0x6,0x0,0x4,0x8,0x20,0xc,0x60,0xef,0x1,0x4,0x8,0x6,0x1,0xd8,0x5,0x6,0xf2,0xa,0x6,0xf4,0x1,0x6,0x8,0x6,0x5,0x78,0x5,0x4,0x0,0xc,0xf9,0x18,0x8,0x6,0x8,0x5,0x0,0x8,0x10,0xb1,0xc9,0x9,0x6,0x8,0x5,0x0,0x12,0x6,0x5,0x7,0x12,0xfe,0xa7,0x4,0x6,0x1,0x5,0x4,0x0,0x1,0x8,0x103,0x2,0x18,0x104,0x5,0x6,0x0,0x8,0x6,0x8,0x10,0x60,0xb1,0x8,0x6,0x1,0x5,0x4,0x0,0x10,0xb5,0x18,0x9,0x0,0x8,0x4,0x7,0xd9,0xc,0x8,0x10d,0x5,0x6,0x0,0x8,0x7,0x60,0xc,0x8,0x111,0x5,0x6,0x0,0x5,0x4,0x0,0xa,0x8,0x61,0xc,0x18,0x115,0x5,0x0,0x8,0x8,0x6,0x1,0x1,0x8,0xb5,0x2,0xc9,0x118,0x8,0x6,0x1,0x4,0x8,0xb5,0x1,0x8,0x11c,0x8,0x6,0x8,0x6,0x8,0xb1,0x1,0xb0,0x120,0x5,0x6,0x8,0x6,0x1,0x31,0x6,0x4,0x7,0x9,0x124,0x126,0x6,0x5,0x1,0x9,0x6,0x8,0x5,0x6,0x0,0x256,0x13,0x60,0x5,0x4,0x8,0xa,0x7,0x20,0x6,0x6,0xa2,0x6,0x6,0x9,0x6,0x5,0x7,0x9,0x130,0xa6,0x6,0x6,0x9,0x6,0x5,0x130,0x9,0x6,0x134,0x5,0x0,0x8,0x8,0x6,0xc8,0x10,0xc9,0x139,0x9,0x6,0x0,0x4,0x6,0x8,0x2,0x36,0x75,0x1,0x8,0x13c,0x9,0x6,0x8,0x6,0x5,0x7,0x6,0x13,0xa6,0x5,0x4,0x8,0x1,0x4,0x6,0x6,0x20,0x37,0xd,0x0,0x142,0xa,0x7,0x1,0x5,0x0,0x8,0x5,0x4,0x6,0x28e,0xc9,0x2c,0x1,0x4,0x8,0x4,0x6,0xd8,0x8,0xd9,0x14a,0x9,0x6,0x0,0x6,0x4,0x6,0x10,0x36,0xdf,0x6,0x7,0x9,0x6,0x5,0x65,0x9,0x6,0x150,0x4,0x4,0x1,0xc,0x8,0x42,0x1,0x6,0x155,0x9,0x6,0x8,0x6,0x5,0x12,0x5,0x6,0x158,0x5,0x4,0x8,0x6,0x5,0x7,0xd,0x20,0xa6,0x8,0x6,0x8,0x9,0x6,0x8,0x5,0x6,0x12,0x6,0x13,0x15f,0x8,0x8,0x161,0x1,0x6,0x8,0x9,0x0,0x8,0x2,0x79,0xd8,0xd,0x0,0x164,0x8,0x6,0x1,0x4,0x1,0x8,0x1,0xb5,0x169,0x8,0x6,0x1,0x5,0x8,0xb5,0x6,0x9,0x16d,0x8,0x6,0x1,0x5,0x6,0xb5,0x5,0x6,0x0,0x12,0x171,0x61,0x5,0x6,0x0,0x8,0x6,0x1,0x2,0x9,0x61,0xc1,0xb5,0x174,0x8,0x7,0x8,0x6,0x1,0x8,0x9,0x97,0x178,0x5,0x6,0x8,0x2,0x4,0x6,0x5,0x6,0xb5,0x12,0x30,0x170,0x6,0x1,0x8,0x8,0x7,0x179,0x9,0x8,0x17e,0xa,0x7,0x8,0x5,0x4,0x182,0x4,0x6,0x185,0x6,0x1,0x8,0xd,0x8,0x178,0x9,0x6,0x188,0x2f2,0x189,0x18b,0x4,0x1,0x8,0x8,0x6,0x8,0x1,0x169,0xb0,0x8,0x6,0x1,0x5,0x4,0x6,0x10,0x1,0x2c,0x16b,0x0,0x57,0xa,0x7,0x1,0x5,0x8,0x146,0x5,0x4,0x6,0x12,0x192,0x2d,0x6,0x7,0x1,0x9,0x6,0x196,0xc,0x8,0x198,0x5,0x4,0x8,0xa,0x6,0x1,0x12,0x21,0x19d,0xd,0x20,0x19e,0xa,0x6,0x1,0x5,0x4,0x6,0x12,0x19d,0x2c,0xd,0x2c,0x1a2,0x8,0x7,0x8,0x5,0x0,0x97,0x5,0x4,0x0,0x12,0x1a6,0x18,0x6,0x4,0x8,0x8,0x6,0x1ab,0x11,0x1ab,0x1ac,0x8,0x6,0x8,0x5,0x8,0xb0,0x6,0x9,0x1b1,0x8,0x6,0x8,0x5,0x4,0x8,0x5,0x4,0xb0,0x12,0x20,0x1b4,0x9,0x6,0x0,0x9,0x0,0x8,0x2,0x36,0xd9,0x1,0x6,0x8,0x4,0x6,0x79,0x5,0x4,0x0,0xf2,0x1bb,0x18,0x9,0x0,0x8,0xc,0x8,0xd9,0x5,0x6,0x8,0x6,0x4,0x31,0x1,0x6,0x1c0,0x4,0x30,0x1c3,0x5,0x0,0x8,0x4,0x7,0xc8,0xa,0x6,0x1c6,0x5,0x4,0x8,0x6,0x4,0x6,0x8,0x20,0xdf,0x11,0x0,0x1ca,0x1,0x6,0x8,0x8,0x1,0x79,0x1,0x8,0x1ce,0x1,0x4,0x8,0x6,0x4,0x7,0x4,0xd9,0x127,0xa,0x7,0x1,0x5,0x4,0x6,0x12,0x147,0x2d,0x5,0x0,0x1d4,0x6,0x1,0x8,0x8,0x6,0x179,0x11,0x0,0x1d9,0x8,0x6,0x8,0x5,0x0,0xb0,0x6,0x9,0x1dc,0x5,0x6,0x8,0x9,0x0,0x30,0x6,0x9,0x1e0,0xc,0x8,0x1e2,0x9,0x0,0x8,0x6,0x1,0x8,0xc,0xd8,0x179,0xd,0x8,0x1e7,0xa,0x7,0x8,0x5,0x6,0x0,0x5,0x4,0x0,0x304,0x61,0x18,0x8,0x7,0x1,0x5,0x4,0x8,0x4,0x1ec,0x20,0xa,0x6,0x8,0x5,0x6,0x8,0x1,0x1f1,0x30,0x6,0x9,0x1f2,0x4,0x5,0x9,0x8,0x7,0x1,0x5,0x1f6,0x1ec,0xa,0x6,0x1,0xd,0x8,0x19c,0x5,0x4,0x0,0x1,0x6,0x8,0x2,0x19,0x79,0x5,0x4,0x0,0xc,0x8,0x19,0x4,0x19,0x1fe,0x9,0x0,0x200,0x1,0x6,0x8,0xa,0x6,0x8,0x6,0x79,0x1f1,0x6,0x7,0x9,0x9,0x0,0x64,0xc,0x8,0x207,0x1,0x6,0x8,0x5,0x4,0x6,0x10,0x79,0x2d,0x1,0x6,0x8,0x6,0x5,0x79,0xc,0x8,0x20d,0x5,0x4,0x0,0xc,0x8,0x19,0x5,0x4,0x8,0x6,0x4,0x7,0x2,0x21,0x126,0xd,0x0,0x210,0x5,0x4,0x8,0x1,0x6,0x8,0x2,0x20,0x79,0x1,0x8,0x214,0x5,0x0,0x8,0xa,0x7,0xc8,0x4,0x4,0x218,0x1,0x6,0x8,0x6,0x1,0x79,0xa,0x6,0x21d,0xd,0x78,0x21e,0xa,0x6,0x8,0x6,0x6,0x1f1,0x11,0x0,0x223,0x6,0x5,0x9,0x1,0x6,0x8,0x10,0x227,0x79,0x1,0x6,0x8,0x6,0x5,0x6,0xc,0x79,0x22a,0xa,0x1,0x8,0x5,0x4,0x8,0xc,0x22f,0x20,0x11,0x0,0x231,0x5,0x6,0x8,0xa,0x9,0x30,0xd,0x0,0x234,0xc,0x8,0x237,0x5,0x4,0x8,0x4,0x4,0x21,0xe,0x20,0x1,0x41,0x23,0x23a,0x5,0x4,0x8,0x5,0x4,0x6,0x2,0x20,0x2d,0x1,0x6,0x8,0x4,0x8,0x79,0x8,0x79,0x241,0x4,0x7,0x9,0x9,0x8,0x244,0xc,0x8,0x247,0x5,0x4,0x8,0xc,0x8,0x20,0x1,0x6,0x8,0x6,0x5,0x79,0xc,0x79,0x20c,0x5,0x4,0x0,0xd,0x8,0x18,0x2,0x18,0x24f,0x1,0x6,0x250,0x4,0x5,0x9,0xd,0x8,0x1f6,0x2,0x1f6,0x255,0x5,0x0,0x8,0x6,0x5,0x7,0x8,0xc9,0xa6,0x5,0x4,0x8,0x6,0x9,0x20,0xe,0x20,0x1,0x5,0x25a,0x23a,0x4,0x1,0x8,0x5,0x4,0x8,0xc,0x169,0x21,0xd,0x8,0x25f,0x4,0x5,0x9,0xc,0x8,0x1f6,0x1,0x6,0x8,0x2,0x5,0x79,0x4,0x6,0x265,0xf1,0x0,0x267,0x6,0x4,0x9,0x2,0x7,0x3c,0x5,0x4,0x26a,0x5,0x6,0x0,0xa,0x9,0x61,0xd,0x8,0x0,0xc,0x26e,0x79,0x4,0x5,0x6,0xc,0x8,0x272,0x5,0x4,0x0,0x6,0x9,0x18,0xe,0x19,0x1,0x5,0x276,0x278,0x5,0x4,0x8,0xd,0x0,0x20,0x10,0x20,0xea,0x5,0x4,0x0,0x6,0x5,0x7,0x10,0x19,0xa6,0x6,0x4,0x9,0xc,0x1,0x3c,0x1,0x8,0x281,0x5,0x4,0x8,0xc,0x1,0x21,0xd,0x8,0x284,0x5,0x4,0x0,0xc,0x1,0x19,0x1,0x8,0x288,0x1,0x6,0x8,0x6,0x5,0x78,0xc,0x8,0xf8,0x6,0x5,0x1,0xc,0x8,0x12a,0x9,0x6,0x8,0x6,0x5,0x12,0x1,0x12,0x158,0xc,0x8,0x291,0x9,0x6,0x8,0x5,0x6,0x8,0xa,0x12,0x31,0xc,0x8,0x294,0x5,0x4,0x8,0xd,0x8,0x20,0x4,0x4,0x1,0x40,0x298,0x43,0x5,0x4,0x29a,0x5,0x4,0x8,0x5,0x6,0x0,0x8,0x20,0x60,0x9,0x6,0x8,0x6,0x9,0x13,0x5,0x6,0x0,0x26,0x2a0,0x60,0x5,0x0,0x8,0x2,0x5,0xc8,0xc,0x8,0x2a4,0x5,0x4,0x0,0x6,0x9,0x18,0xe,0x19,0x277,0x9,0x18,0x2a8,0x6,0x5,0x8,0x4,0x6,0xc0,0x5,0x8,0x2ac,0x6,0x4,0x8,0x4,0x4,0x6,0x9,0x1ab,0x26,0x5,0x6,0x0,0x9,0x8,0x60,0x9,0x6,0x0,0xc0,0x2b2,0x36,0x5,0x4,0x0,0x6,0x9,0x19,0x5,0x6,0x0,0x32,0x2b6,0x60,0x1,0x4,0x8,0x5,0x4,0xd8,0x4,0x7,0xd9,0x1b2,0x2bb,0x10c,0x6,0x4,0x8,0x4,0x6,0x1ab,0x11,0x0,0x2be,0x5,0x6,0x8,0xa,0x9,0x30,0x8,0x7,0x31,0x1,0x234,0x2c2,0x6,0x5,0x9,0x4,0x4,0x7,0x1,0x226,0xbd,0x6,0x5,0x1,0xe,0x8,0x12a,0x1,0x12a,0x2c9,0x5,0x4,0x0,0x5,0x0,0x8,0xe,0x19,0xc9,0x5,0x18,0x2cc,0x5,0x4,0x8,0x4,0x7,0x20,0x1,0x4,0x2d1,0x5,0x4,0x0,0x4,0x4,0x7,0x11,0x18,0xbd,0x1,0x6,0x8,0x1,0x6,0x8,0x5,0x0,0x6,0x5,0x4,0x60,0xf2,0x61,0x2d7,0x8,0x6,0x8,0x9,0x6,0x8,0x6,0xb1,0x12,0x1,0x12,0x2da,0x5,0x0,0x8,0x1,0x4,0x8,0xa,0x7,0xd9,0x192,0xd9,0x2de,0x9,0x6,0x0,0x9,0x8,0x36,0x4,0x37,0x2e3,0xd,0x0,0x2e4,0x6,0x6,0x8,0x5,0x4,0x6,0x4,0x2e9,0x2d,0x11,0x0,0x2eb,0x4,0x1,0x9,0x5,0x4,0x8,0x5,0x6,0x8,0x5de,0x20,0x30,0x1,0x8,0x2f1,0x4,0x1,0x9,0x5,0x4,0x8,0x9,0x6,0x20,0x5de,0x20,0x2f4,0x4,0x5,0x8,0x5,0x4,0x6,0x8,0x9d,0x2d,0x1,0x4,0x8,0x5,0x4,0xd8,0xc,0xd9,0x2bb,0x5,0x0,0x8,0x6,0x5,0xc9,0xc,0xc9,0x2fc,0x5,0x6,0x8,0x6,0x8,0x31,0xa,0x30,0x300,0x4,0x1,0x9,0x5,0x6,0x8,0x8,0x2ef,0x30,0xd,0x30,0x305,0x9,0x6,0x8,0x4,0x4,0x12,0xc,0x12,0x308,0x2,0x7,0x9,0x9,0x6,0x30c,0x4,0x30c,0x30e,0x1,0x4,0x8,0x1,0x4,0x6,0x5,0x6,0xd8,0x1b2,0x37,0x313,0x9,0x36,0x314,0x5,0x0,0x8,0x4,0x7,0xc8,0x8,0xc9,0x1c6,0x5,0x0,0x8,0x2,0x7,0x9,0x8,0xc9,0x30c,0x8,0x7,0x9,0x4,0x4,0x31c,0x9,0x8,0x31e,0x6,0x7,0x9,0x4,0x4,0x65,0x5,0x8,0x323,0x1,0x6,0x8,0x4,0x4,0x79,0x1,0x8,0x327,0x9,0x6,0x8,0xa,0x7,0x1,0x6,0x12,0x146,0x5,0x6,0x8,0x4,0x1,0x30,0xa,0x6,0x32d,0x1,0x30,0x32f,0x6,0x4,0x8,0x4,0x4,0x7,0x5,0x1ab,0xbd,0x6,0x1,0x8,0x4,0x4,0x6,0x1,0x178,0x26,0x4,0x4,0x6,0x1,0x8,0x26,0x5,0x4,0x8,0x9,0x6,0x20,0x4,0x1,0x8,0x2,0x20,0x2f5,0x9,0x169,0x338,0x5,0x4,0x0,0x9,0x6,0x8,0x2,0x19,0x13,0x5,0x8,0x33c,0x5,0x6,0x0,0x9,0x6,0x60,0x10,0x61,0x341,0x5,0x8,0x342,0x5,0x4,0x8,0x2,0x7,0x20,0x4,0x4,0x23b,0x41,0x0,0x347,0x9,0x6,0x8,0x6,0x8,0x12,0x2,0x5,0x13,0x1,0x34b,0x34c,0x6,0x4,0x8,0x5,0x4,0x6,0xe,0x1,0x2c,0x5,0x1ab,0x350,0x5,0x6,0x0,0x9,0x6,0x60,0x2,0x61,0x341,0x5,0x8,0x354,0x5,0x6,0x0,0x5,0x4,0x60,0x2,0x61,0x2d7,0x11,0x0,0x358,0x2,0x6,0x9,0x4,0x4,0x35d,0xd,0x8,0x35f,0x6,0x7,0x9,0x4,0x4,0x6,0x5,0x64,0x26,0x4,0x4,0x6,0xd,0x8,0x26,0x5,0x0,0x8,0x9,0x6,0xc8,0x2,0xc8,0x366,0x5,0x4,0x369,0x9,0x6,0x0,0x4,0x1,0x37,0xd,0x8,0x36c,0x4,0x4,0x1,0xd,0x8,0x43,0x9,0x6,0x8,0x1,0x6,0x8,0x5,0x4,0x78,0x0,0x0,0x0
  };// clang-format on
}; // namespace mockturtle

} /* namespace mockturtle */