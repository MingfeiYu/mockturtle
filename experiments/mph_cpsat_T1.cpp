
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

#include <string>
#include <vector>
#include <set>
#include <cstdio>
#include <filesystem>

#include <fmt/format.h>
#include <lorina/aiger.hpp>
#include <lorina/blif.hpp>
#include <lorina/genlib.hpp>
#include <lorina/bench.hpp>
#include <mockturtle/algorithms/rsfq/rsfq_network_conversion.hpp>
#include <mockturtle/algorithms/rsfq/rsfq_path_balancing.hpp>

#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/multiphase.hpp>
// #include <mockturtle/algorithms/compound_gate_mapping.hpp>

#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/algorithms/nodes.hpp>
#include <mockturtle/algorithms/node_resynthesis.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>
#include <mockturtle/algorithms/retiming.hpp>
// #include <mockturtle/algorithms/refactoring.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/io/blif_reader.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/io/bench_reader.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/utils/tech_library.hpp>
#include <mockturtle/views/binding_view.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/rsfq_view.hpp>
#include <mockturtle/algorithms/functional_reduction.hpp>
#include <mockturtle/algorithms/klut_to_graph.hpp>

// mockturtle/algorithms/mig_algebraic_rewriting.hpp

#include <mockturtle/io/auxiliary_genlib.hpp>

// #include <mockturtle/utils/GNM_global.hpp> // GNM global is stored here

#include <mockturtle/utils/misc.hpp>

#include <experiments.hpp>

#include <chrono>



template <size_t N, typename T>
using array_map = phmap::flat_hash_map<std::array<klut::node, N>, T, ArrayHash<N>>;

// // Sunmagnetics Technology Library
// constexpr std::array<int,12> COSTS_MAP = {7, 9, 8, 8, 12, 8, 999, 999, 999, 8, 3, 0};
// Sunmagnetics Technology Library
constexpr std::array<int,12> COSTS_MAP = COSTS_SUNMAGNETICS;

template <typename Ntk>
std::tuple<mockturtle::binding_view<klut>, mockturtle::map_stats> map_wo_pb 
( 
  const Ntk & input_ntk, 
  const mockturtle::tech_library<4u, mockturtle::classification_type::p_configurations> & tech_lib, 
  const bool area_oriented = false,
  const bool verbose = false
)
{
  mockturtle::map_params ps;
  ps.verbose = verbose;
  ps.cut_enumeration_ps.minimize_truth_table = true;
  ps.cut_enumeration_ps.cut_limit = 24;
  // ps.cut_enumeration_ps.very_verbose = true;
  ps.cut_enumeration_ps.verbose = true;
  ps.buffer_pis = false;
  if (area_oriented)
  {
      ps.skip_delay_round = true;
      ps.required_time = std::numeric_limits<float>::max();
  }
  mockturtle::map_stats st;
  mockturtle::binding_view<klut> res = map( input_ntk, tech_lib, ps, &st );
  return std::make_tuple( res, st );
}

template <typename Ntk>
std::tuple<mockturtle::binding_view<klut>, mockturtle::map_stats, double, double, bool> map_with_pb 
( 
  const std::string & benchmark, 
  const Ntk & input_ntk, 
  const mockturtle::tech_library<4u, mockturtle::classification_type::p_configurations> & tech_lib, 
  phmap::flat_hash_map<std::string, int> & nDFF_global, 
  bool area_oriented = false
)
{
  fmt::print("Started mapping of {}\n", benchmark);
  auto [res, st] = map_wo_pb(input_ntk, tech_lib, area_oriented);
  fmt::print("Finished mapping of {}\n", benchmark);

  std::map<klut::node, int> dff_count;
  std::map<klut::node, int> fanout_count;

  /* RSFQ path balancing */
  fmt::print("Started RSFQ path balancing of {}\n", benchmark);
  auto balanced_res = mockturtle::rsfq_path_balancing( res );
  fmt::print("Finished RSFQ path balancing of {}\n", benchmark);

  mockturtle::retime_params rps;
  mockturtle::retime_stats rst;
  fmt::print("Started rsfq_generic_network_create_from_mapped of {}->net\n", benchmark);
  auto net = mockturtle::rsfq_generic_network_create_from_mapped( balanced_res );
  fmt::print("Finished rsfq_generic_network_create_from_mapped of {}->net\n", benchmark);
  fmt::print("Started retime of {}\n", benchmark);
  mockturtle::retime( net, rps, &rst );
  fmt::print("Finished retime of {}\n", benchmark);
  fmt::print("Started rsfq_generic_network_create_from_mapped of net->{}\n", benchmark);
  auto retime_res = mockturtle::rsfq_mapped_create_from_generic_network( net );
  fmt::print("Finished rsfq_generic_network_create_from_mapped of net->{}\n", benchmark);

  uint32_t num_ext_dffs = retime_res.num_dffs();
  
  uint32_t num_int_dffs = 0;

  retime_res.foreach_node( 
    [&]( auto const& n ) 
    {
      if ( !retime_res.has_binding( n ) )
        return;
      auto const& g = retime_res.get_binding( n );
      num_int_dffs += nDFF_global[g.name];
      // fmt::print("Node {}\tGate {}\tnDFF {}\n", n, g.name, nDFF_global.at(g.name));
    } 
  );

  /* RSFQ splitter insertion */
  uint32_t num_splitters = 0;
  retime_res.foreach_node( [&]( auto const& n ) {
    if ( !retime_res.is_constant( n ) )
      num_splitters += retime_res.fanout_size( n ) - 1;
  } );

  fmt::print("Started rsfq_check_buffering of {}\n", benchmark);
  bool cec = rsfq_check_buffering( retime_res );
  fmt::print("Finished rsfq_check_buffering of {}\n", benchmark);
  fmt::print("Started abc_cec of {}\n", benchmark);
  cec &= benchmark == "hyp" ? true : experiments::abc_cec( retime_res, benchmark );
  fmt::print("Finished abc_cec of {}\n", benchmark);

  // Internal DFF area is already counted in the library
  // External DFF area is already counted after retiming
  double total_ndff = num_int_dffs + num_ext_dffs;
  double total_area = st.area + COSTS_MAP[fSPL] * num_splitters;
  //  +  COSTS_MAP[fDFF] * num_ext_dffs;
  fmt::print("\t{} : Int: {}, Ext: {}, ratio: {}\n", benchmark, num_int_dffs, num_ext_dffs, (float)num_int_dffs / (num_int_dffs + num_ext_dffs) );
  return std::make_tuple( res, st, total_ndff, total_area, cec );
}

struct Snake
{
  std::deque<std::vector<uint64_t>> sections;

  Snake(): sections({}) {}
  Snake( const uint64_t head ): sections({ { head } }) {}
  Snake( const std::deque<std::vector<uint64_t>> _sections ): sections(_sections) {}
  Snake( const Snake & _other ): sections(_other.sections) {}

  bool append(const uint64_t dff_hash, DFF_registry &DFF_REG, const uint8_t n_phases)
  {
    DFF_var & dff = DFF_REG.at( dff_hash );

    std::vector<uint64_t> & head_section = sections.back();
    uint64_t & head_hash = head_section.back();
    DFF_var & head_dff = DFF_REG.at( head_hash );
    if (dff.sigma == head_dff.sigma) // add to the same section
    {
      head_section.push_back( dff_hash );
      return false;
    }
    else
    {
      // assert( head_dff.phase - dff.phase == 1 );
      sections.push_back( { dff_hash } );
      if (sections.size() > n_phases)
      {
        sections.pop_front();
      }
      return true;
    }
  }

  uint32_t append( const uint64_t dff_hash, DFF_registry& DFF_REG )
  {
    DFF_var& dff = DFF_REG.at( dff_hash );
    std::vector<uint64_t>& head_section = sections.back();
    uint64_t& head_hash = head_section.back();
    DFF_var& head_dff = DFF_REG.at( head_hash );

    if ( dff.sigma == head_dff.sigma ) // add to the same section
    {
      head_section.push_back( dff_hash );
    }
    else
    {
      // assert( head_dff.phase - dff.phase == 1 );
      sections.push_back( { dff_hash } );
    }
    return sections.size();
  }

  /* for adding helper variables, which */
  /* are not stored in 'DFF_REG'        */
  void append( DFF_var const& dff )
  {
    sections.emplace_back( dff_hash( dff ) );
  }
};

struct Standard_T1_CELL
{
  // 3 bits to store input negations
  unsigned in_phase : 3;
  /* truth tables of the 3 outputs under the current input phase */
  uint8_t sum_truth_table{};
  uint8_t carry_truth_table{};
  uint8_t cbar_truth_table{};

  Standard_T1_CELL( const unsigned in_phase_, const uint8_t sum_truth_table_, const uint8_t carry_truth_table_, const uint8_t cbar_truth_table_ )
    : in_phase( in_phase_ ), sum_truth_table( sum_truth_table_ ), carry_truth_table( carry_truth_table_ ), cbar_truth_table( cbar_truth_table_ ) {}
  
  /// @brief Checks whether the truth table matches the carry TT of this T1 cell
  /// @param tt truth table to match
  /// @param phase - set to *true* if the matched to negated output, or *false* if matched to non-negated output
  /// @return *true* if there is a match at all
  bool check_carry( const kitty::dynamic_truth_table & tt, bool & phase ) const
  {
    if ( tt._bits[0] == ~carry_truth_table )
    {
      phase = true;
      return true;
    }
    else if ( tt._bits[0] != carry_truth_table )
    {
      return false;
    }
    phase = false;
    return true;
  }

  /// @brief Checks whether the truth table matches the cbar TT of this T1 cell
  /// @param tt truth table to match
  /// @param phase - set to *true* if the matched to negated output, or *false* if matched to non-negated output
  /// @return *true* if there is a match at all
  bool check_cbar( const kitty::dynamic_truth_table & tt, bool & phase ) const
  {
    if ( tt._bits[0] == ~cbar_truth_table )
    {
      phase = true;
      return true;
    }
    else if ( tt._bits[0] != cbar_truth_table )
    {
      return false;
    }
    phase = false;
    return true;
  }
};

static std::array<Standard_T1_CELL, 8> standard_T1_cells = 
{
  Standard_T1_CELL( 0, 0x96, 0xe8, 0xfe ), 
  Standard_T1_CELL( 1, 0x69, 0xd4, 0xfd ), 
  Standard_T1_CELL( 2, 0x69, 0xb2, 0xfb ), 
  Standard_T1_CELL( 3, 0x96, 0x71, 0xf7 ), 
  Standard_T1_CELL( 4, 0x69, 0x8e, 0xef ), 
  Standard_T1_CELL( 5, 0x96, 0x4d, 0xdf ), 
  Standard_T1_CELL( 6, 0x96, 0x2b, 0xbf ),
  Standard_T1_CELL( 7, 0x69, 0x17, 0x7f )
};

struct T1_OUTPUTS
{
  // 3 bits to store input negations
  unsigned in_phase          : 3;
  // whether carry (maj3) output is used with inverter, feel free to rename if other naming is more convenient for you
  bool     has_carry_inverted: 1;
  // whether cbar (or3) output is used with inverter, feel free to rename if other naming is more convenient for you
  bool     has_cbar_inverted : 1;
  bool     has_sum           : 1;
  // whether carry (maj3) output is used with DFF
  bool     has_carry         : 1;
  // whether cbar (or3) output is used with DFF
  bool     has_cbar          : 1;
  /* TTs of the 3 outputs */
  // uint8_t   sum_truth_table{};
  // uint8_t carry_truth_table{};
  // uint8_t  cbar_truth_table{};
  /* usage of the 5 output ports */
  klut::node       sum_to;
  klut::node     carry_to;
  klut::node inv_carry_to;
  klut::node      cbar_to;
  klut::node  inv_cbar_to;

  // TODO : calculate cost, add mask for specific outputs
  T1_OUTPUTS() : in_phase(0u), has_carry_inverted(false), has_cbar_inverted(false), has_carry(false), has_cbar(false) {}

  // Explicit Constructor
  T1_OUTPUTS(const uint8_t in, const bool p_c, const bool p_cb, const bool h_c, const bool h_cb)
    : in_phase(in), has_carry_inverted(p_c), has_cbar_inverted(p_cb), has_carry(h_c), has_cbar(h_cb) {}

  /* Explicit constructor with output functions provided */
  T1_OUTPUTS( const uint8_t in, const bool p_c, const bool p_cb, const bool h_s, const bool h_c, const bool h_cb, const klut::node sum_node, 
              const klut::node carry_node, const klut::node inv_carry_node, const klut::node cbar_node, const klut::node inv_cbar_node )
    : in_phase( in ), has_carry_inverted( p_c ), has_cbar_inverted( p_cb ), has_sum( h_s ), has_carry( h_c ), has_cbar( h_cb ), 
      sum_to( sum_node ), carry_to( carry_node ), inv_carry_to( inv_carry_node ), cbar_to( cbar_node ), inv_cbar_to( inv_cbar_node ) {}
    
  // returns the TT of the sum output of the T1 cell
  /* Q: does tt information really matter? */
  // uint8_t sum_tt() const
  // {
  //   return (__builtin_popcount(in_phase) & 1) ? 0x69 : 0x96;
  // }
  // // returns the TT of the carry output of the T1 cell
  // uint8_t carry_tt(const bool output_phase = false) const
  // {
  //   TT3 tt_in;
  //   tt_in._bits = MAJ3;
  //   const uint32_t phase = in_phase | (output_phase << CUT_SIZE);
    
  //   TT3 tt_out = kitty::create_from_npn_config( std::make_tuple( tt_in, phase, perm ) );
  //   return static_cast<uint8_t>(tt_out._bits);
  // }
  // // returns the TT of the cbar output of the T1 cell
  // uint8_t cbar_tt(const bool output_phase = false) const
  // {
  //   TT3 tt_in;
  //   tt_in._bits = OR3;
  //   const uint32_t phase = in_phase | (output_phase << CUT_SIZE);
    
  //   TT3 tt_out = kitty::create_from_npn_config( std::make_tuple( tt_in, phase, perm ) );
  //   return static_cast<uint8_t>(tt_out._bits);
  // }
  // uint8_t cost() const
  // {
  //   return T1_COST // base cost (2x merger + T1 cell)
  //     + __builtin_popcount(in_phase) * (COSTS_MAP[fNOT] - COSTS_MAP[fDFF]) // cost of negating the inputs
  //     + COSTS_MAP[fDFF] * (has_carry + has_cbar)                          // cost of MAJ3 and OR3
  //     + COSTS_MAP[fNOT] * (has_carry_inverted + has_cbar_inverted)        // cost of ~(MAJ3) and ~(OR3)
  //     + COSTS_MAP[fSPL] * ((has_carry & has_carry_inverted) + (has_cbar & has_cbar_inverted)); //cost of splitters, if needed 
  // }
  void report() const
  {
    fmt::print("\t\tInput phases : {{ {}0,{}1,{}2 }}\n", ( ( in_phase & 1 ) ? "~" : " " ), ( ( in_phase >> 1 & 1 ) ? "~" : " " ), ( ( in_phase >> 2 & 1 ) ? "~" : " " ) );
    // fmt::print("\t\t      Sum ( Node {0} ): {1:08b} ( 0x{1:02x} )\n",       sum_to,    sum_truth_table );
    // fmt::print("\t\t    Carry ( Node {0} ): {1:08b} ( 0x{1:02x} )\n",     carry_to,  carry_truth_table );
    // fmt::print("\t\tInv carry ( Node {0} ): {1:08b} ( 0x{1:02x} )\n", inv_carry_to, ~carry_truth_table );
    // fmt::print("\t\t     Cbar ( Node {0} ): {1:08b} ( 0x{1:02x} )\n",      cbar_to,   cbar_truth_table );
    // fmt::print("\t\t Inv cbar ( Node {0} ): {1:08b} ( 0x{1:02x} )\n",  inv_cbar_to,  ~cbar_truth_table );
    fmt::print("\t\t      Sum : {}\n", (            has_sum  ? "Node " + std::to_string(       sum_to ) : "N/A" ) );
    fmt::print("\t\t    Carry : {}\n", (          has_carry  ? "Node " + std::to_string(     carry_to ) : "N/A" ) );
    fmt::print("\t\tInv carry : {}\n", ( has_carry_inverted  ? "Node " + std::to_string( inv_carry_to ) : "N/A" ) );
    fmt::print("\t\t     Cbar : {}\n", (           has_cbar  ? "Node " + std::to_string(      cbar_to ) : "N/A" ) );
    fmt::print("\t\t Inv cbar : {}\n", (  has_cbar_inverted  ? "Node " + std::to_string(  inv_cbar_to ) : "N/A" ) );

  }
};

template <typename CutType>
array_map<3, std::tuple<kitty::dynamic_truth_table, klut::node>> match_cuts( const std::vector<TT3> & template_tts, const klut & ntk, const CutType & cuts)
{
  array_map<3, std::tuple<kitty::dynamic_truth_table, klut::node>> matching_cuts;

  ntk.foreach_node( [&]( const klut::signal & node ) 
  { 
    const auto idx = ntk.node_to_index( node );
    const auto & node_cut_set = cuts.cuts( idx );
    // fmt::print("[{}] Extracted {} cuts for node {} (idx={})\n", benchmark, node_cut_set.size(), node, idx);
    // if (node_cut_set.size() == 0) { return; }

    /* check if the function of the cut is NPN-equivalent to any of the template_tts */
    for (const auto & cut_entry : node_cut_set)
    {
      const auto tt = cuts.truth_table( *cut_entry );
      if (std::find_if(template_tts.begin(), template_tts.end(), [&](const TT3 & template_tt){return tt._bits[0] == template_tt._bits;}) != template_tts.end())
      {
        std::array<klut::node,3> leaves;
        std::copy(cut_entry->begin(), cut_entry->end(), leaves.begin());
        matching_cuts.emplace( leaves, std::make_tuple(tt, node) );
      }
    }
  } );
  return matching_cuts;
}

void config_t1_ports_connection( klut const& ntk, const bool output_phase, const klut::node target_node, klut::node& port, klut::node& inv_port )
{
  if ( !output_phase )
  {
    /* connect the node to the output port */
    port = target_node;
    /* if (1) the target node is an inverter, and    */
    /* (2) its fanin node has more than one fanout,  */
    /* the fanin node of the target node shall be    */
    /* connected to the output port of inverted port */
    if ( ntk.func_lit( target_node ) == 3 )
    {
      ntk.foreach_fanin( port, [&inv_port, &ntk]( auto const& ni ) {
        if ( ntk.fanout_size( ni ) > 1 )
        {
          inv_port = ni;
        }
      } );
    }
  }
  else
  {
    inv_port = target_node;
    if ( ntk.func_lit( target_node ) == 3 )
    {
      ntk.foreach_fanin( inv_port, [&port, &ntk]( auto const& ni ) {
        if ( ntk.fanout_size( ni ) > 1 )
        {
          port = ni;
        }
      } );
    }
  }
}

array_map<3, T1_OUTPUTS> find_t1_candidates( 
  const klut & ntk,
  const array_map<3, std::tuple<kitty::dynamic_truth_table, klut::node>>& xor3_cuts,
  const array_map<3, std::tuple<kitty::dynamic_truth_table, klut::node>>& maj3_cuts, 
  const array_map<3, std::tuple<kitty::dynamic_truth_table, klut::node>>& or3_cuts
) 
{
  array_map<3, T1_OUTPUTS> t1_candidates;

  for ( auto const& [target_leaves, xor3_tt_and_node] : xor3_cuts )
  {
    if ( !( maj3_cuts.contains( target_leaves ) &&  // if there's a match with MAJ3
             or3_cuts.contains( target_leaves ) ) ) // and a match with OR3
    {
      continue;
    }
    const auto & [xor3_tt, xor_index] = xor3_tt_and_node;
    const auto & [maj3_tt, maj_index] = maj3_cuts.at(target_leaves);
    const auto & [ or3_tt,  or_index] = or3_cuts.at(target_leaves); 
    /* find a candidate potentially using all 3 outputs of an T1 cell */
    /* check whether the combination of truth table is valid          */
    for ( auto const& t1_cell : standard_T1_cells )
    {
      bool maj_phase{ false }, or_phase{ false };
      if (
        xor3_tt._bits[0] != t1_cell.sum_truth_table ||
        !t1_cell.check_carry(maj3_tt, maj_phase) || 
        !t1_cell.check_cbar(or3_tt, or_phase)
      )
      {
        continue;
      }

      /* managed to find a T1 cell-based implementation      */
      /* figure out the nodes connecting to the output ports */
      klut::node sum_to       = ntk.index_to_node( xor_index );

      klut::node carry_to     = ntk.get_constant( false );
      klut::node inv_carry_to = ntk.get_constant( false );
      config_t1_ports_connection( ntk, maj_phase, maj_index, carry_to, inv_carry_to );

      klut::node cbar_to      = ntk.get_constant( false );
      klut::node inv_cbar_to  = ntk.get_constant( false );
      config_t1_ports_connection( ntk,  or_phase, or_index, cbar_to,  inv_cbar_to );

      /* instantiate an T1 cell */
      t1_candidates.emplace( target_leaves, T1_OUTPUTS( 
        t1_cell.in_phase, 
        inv_carry_to != ntk.get_constant( false ), 
        inv_cbar_to  != ntk.get_constant( false ), 
        sum_to       != ntk.get_constant( false ), 
        carry_to     != ntk.get_constant( false ), 
        cbar_to      != ntk.get_constant( false ), 
        sum_to, carry_to, inv_carry_to, cbar_to, inv_cbar_to ) );
      break;
      /* TODO: think about if it is possible that a design can be implemented under */
      /* different input phases of a T1 cell; if yes, it is better to enumerate all */
      /* the potential input phases and choose the one requires the minimum cost    */
    }
  }

  for ( auto const& [target_leaves, xor3_tt_and_node] : xor3_cuts )
  {
    if ( !maj3_cuts.contains( target_leaves ) ||   // if there's no XOR-MAJ match
          t1_candidates.contains( target_leaves ) ) // or a 3-way match already found
    {
      continue;
    }

    auto [xor3_tt, xor_index] = xor3_tt_and_node;
    auto [maj3_tt, maj_index] = maj3_cuts.at(target_leaves);
    /* find a candidate potentially using 2 outputs of an T1 cell  */
    /* ( XOR3, MAJ3 ), check whether the combnation of tt is valid */
    for ( auto const& t1_cell : standard_T1_cells )
    {
      if ( xor3_tt._bits[0] != t1_cell.sum_truth_table )
      {
        continue;
      }

      bool maj_phase{ false };
      if (!t1_cell.check_carry(maj3_tt, maj_phase))
      {
        continue;
      }

      /* managed to find a T1 cell-based implementation      */
      /* figure out the nodes connecting to the output ports */
      klut::node sum_to       = ntk.index_to_node( xor_index );
      klut::node carry_to     = ntk.get_constant( false );
      klut::node inv_carry_to = ntk.get_constant( false );
      config_t1_ports_connection( ntk, maj_phase, maj_index, carry_to, inv_carry_to );

      /* instantiate an T1 cell */
      t1_candidates.emplace( target_leaves, T1_OUTPUTS( 
        t1_cell.in_phase, 
        inv_carry_to != ntk.get_constant( false ), 
        false, 
        true, 
        carry_to != ntk.get_constant( false ), 
        false, 
        sum_to, 
        carry_to, 
        inv_carry_to, 
        ntk.get_constant( false ), 
        ntk.get_constant( false ) ) );
      break;
    }
  }

  for ( auto const& [target_leaves, xor3_tt_and_node] : xor3_cuts )
  {
    if ( !or3_cuts.contains( target_leaves ) ||  // if there's no match with OR3
        t1_candidates.contains( target_leaves )) // or a 3-way match already found
    {
      continue;
    }
    auto [xor3_tt, xor_index] = xor3_tt_and_node;
    auto [or3_tt, or_index] = or3_cuts.at(target_leaves); 
    /* find a candidate potentially using 2 outputs of an T1 cell  */
    /* ( XOR3,  OR3 ), check whether the combnation of tt is valid */
    for ( auto const& t1_cell : standard_T1_cells )
    {
      if ( xor3_tt._bits[0] != t1_cell.sum_truth_table )
      {
        continue;
      }

      bool or_phase{ false };
      if (!t1_cell.check_cbar(or3_tt, or_phase))
      {
        continue;
      }

      /* managed to find a T1 cell-based implementation      */
      /* figure out the nodes connecting to the output ports */
      klut::node sum_to      = ntk.index_to_node( xor_index );
      klut::node cbar_to     = ntk.get_constant( false );
      klut::node inv_cbar_to = ntk.get_constant( false );
      config_t1_ports_connection( ntk, or_phase, or_index, cbar_to, inv_cbar_to );

      /* instantiate an T1 cell */
      t1_candidates.emplace( target_leaves, T1_OUTPUTS( 
        t1_cell.in_phase, 
        false, 
        inv_cbar_to != ntk.get_constant( false ), 
        true, 
        false, 
        cbar_to != ntk.get_constant( false ), 
        sum_to, 
        ntk.get_constant( false ), 
        ntk.get_constant( false ), 
        cbar_to, 
        inv_cbar_to ) );
      break;
    }
  }

  for ( auto const& [target_leaves, maj3_tt_and_node] : maj3_cuts )
  {
    if ( !or3_cuts.contains( target_leaves ) ||  // if there's no match with OR3
        t1_candidates.contains( target_leaves )) // or a 3-way match already found
    {
      continue;
    }
    auto [maj3_tt, maj_index] = maj3_tt_and_node;
    auto [or3_tt, or_index] = or3_cuts.at(target_leaves); 
    /* find a candidate potentially using 2 outputs of an T1 cell  */
    /* ( MAJ3,  OR3 ), check whether the combnation of tt is valid */
    for ( auto const& t1_cell : standard_T1_cells )
    {
      
      bool maj_phase{ false }, or_phase{ false };
      if (
        !t1_cell.check_carry(maj3_tt, maj_phase) || 
        !t1_cell.check_cbar(or3_tt, or_phase)
      )
      {
        continue;
      }

      /* managed to find a T1 cell-based implementation      */
      /* figure out the nodes connecting to the output ports */
      klut::node carry_to     = ntk.get_constant( false );
      klut::node inv_carry_to = ntk.get_constant( false );
      klut::node cbar_to      = ntk.get_constant( false );
      klut::node inv_cbar_to  = ntk.get_constant( false );
      config_t1_ports_connection( ntk, maj_phase, maj_index, carry_to, inv_carry_to );
      config_t1_ports_connection( ntk,  or_phase, or_index,  cbar_to,  inv_cbar_to );

      /* instantiate an T1 cell */
      t1_candidates.emplace( target_leaves, T1_OUTPUTS( 
        t1_cell.in_phase, 
        inv_carry_to != ntk.get_constant( false ), 
        inv_cbar_to != ntk.get_constant( false ), 
        false, 
        carry_to != ntk.get_constant( false ), 
        cbar_to != ntk.get_constant( false ), 
        ntk.get_constant( false ), 
        carry_to, 
        inv_carry_to, 
        cbar_to, 
        inv_cbar_to ) );
      break;
    }
  }

  return t1_candidates;
}

void write_snakes(const std::vector<Snake> & snakes, DFF_registry & DFF_REG, const std::vector<uint64_t> & required_SA_DFFs, const std::string cfg_name, uint8_t n_phases, bool verbose = false)
{
  std::ofstream spec_file (cfg_name);

  for (const Snake & snake : snakes)
  {
    std::vector<std::string> vars_bucket;
    for (const std::vector<uint64_t> & section : snake.sections)
    {
      std::vector<std::string> vars;
      for (uint64_t hash : section)
      {
        vars.push_back(DFF_REG.str( hash ));
      }
      DEBUG_PRINT("New single phase conflict : {}≤1\n", fmt::join(vars, "+"));
      vars_bucket.push_back(fmt::format(vars.size()>1?"({})":"{}", fmt::join(vars, "+")));
      if (vars.size() > 1)
      {
        spec_file << fmt::format("PHASE,{}\n", fmt::join(vars, ","));
      }
    }
    std::reverse(vars_bucket.begin(), vars_bucket.end());
    DEBUG_PRINT("New buffer requirement : ({})\n", fmt::join(vars_bucket, "|"));
    if (vars_bucket.size() == n_phases)
    {
      spec_file << fmt::format("BUFFER,{}\n", fmt::join(vars_bucket, ","));
    }
  }

  for (const uint64_t & hash : required_SA_DFFs)
  {
    DEBUG_PRINT("New SA_REQUIRED : {}\n", DFF_REG.str( hash ));
    spec_file << fmt::format("SA_REQUIRED,{}\n", DFF_REG.str( hash ));
  }
}
void write_snakes_t1( std::vector<Snake> const& snakes, phmap::flat_hash_map<klut::node, std::vector<Snake>> const& t1_input_constraint, 
                      phmap::flat_hash_map<klut::node, std::array<bool, 3>> const& input_phases, std::vector<DFF_var> const& helpers, 
                      DFF_registry & DFF_REG, std::vector<uint64_t> const& required_SA_DFFs, const std::string cfg_name, uint8_t n_phases, bool verbose = false )
{
  std::ofstream spec_file( cfg_name );

  /* basic constraints guanranteeing the correct functionality of the multiphase clocking scheme */
  for (const Snake & snake : snakes)
  {
    std::vector<std::string> vars_bucket;
    for (const std::vector<uint64_t> & section : snake.sections)
    {
      std::vector<std::string> vars;
      for (uint64_t hash : section)
      {
        vars.push_back(DFF_REG.str( hash ));
      }
      DEBUG_PRINT("New single phase conflict : {}≤1\n", fmt::join(vars, "+"));
      vars_bucket.push_back(fmt::format(vars.size()>1?"({})":"{}", fmt::join(vars, "+")));
      if (vars.size() > 1)
      {
        spec_file << fmt::format("PHASE,{}\n", fmt::join(vars, ","));
      }
    }
    std::reverse(vars_bucket.begin(), vars_bucket.end());
    DEBUG_PRINT("New buffer requirement : ({})\n", fmt::join(vars_bucket, "|"));
    if (vars_bucket.size() == n_phases)
    {
      spec_file << fmt::format("BUFFER,{}\n", fmt::join(vars_bucket, ","));
    }
  }
  for (const uint64_t & hash : required_SA_DFFs)
  {
    DEBUG_PRINT("New SA_REQUIRED : {}\n", DFF_REG.str( hash ));
    spec_file << fmt::format("SA_REQUIRED,{}\n", DFF_REG.str( hash ));
  }

  /* constraints supporting the usage of T1 gates under the multiphase clocking scheme */

  /* (1) the phases of the three inputs to a T1 gate shall be different from each other*/
  for ( auto it{ t1_input_constraint.begin() }; it != t1_input_constraint.end(); ++it )
  {
    const klut::node repr{ it->first };
    for ( Snake const& snake : it->second )
    {
      /* for each of the three input signals */
      spec_file << fmt::format( "T1,{},", repr );
      for ( std::vector<uint64_t> const& section : snake.sections )
      {
        std::vector<std::string> vars;
        for ( uint64_t hash : section )
        {
          vars.push_back( DFF_var( hash ).str() );
        }
        spec_file << fmt::format( "{},", fmt::join( vars, "|" ) );
      }
      spec_file << fmt::format( "\n" );
    }
  }

  /* (2) a helper variable indicates that the source is an AS gate and the distance    */
  /* between the source and the T1 gate is no more than one epoch; a helper variable   */
  /* shall always be assigned to true                                                  */
  for ( DFF_var const& helper : helpers )
  {
    spec_file << fmt::format( "HELPER,{}\n", helper.str() );
  }

  /* (3) if a input to a T1 is negated, there should be an inverter inserted in the    */
  /* last epoche before the phase of the T1                                            */
  for ( auto it{ t1_input_constraint.begin() }; it != t1_input_constraint.end(); ++it )
  {
    const klut::node repr{ it->first };
    for ( auto i{ 0u }; i < 3u; ++i )
    {
      if ( input_phases.at( repr )[i] )
      {
        /* this input signal shall be negated, therefore */
        /* at lease one of the variables have to be true */
        auto const& sections = it->second[i].sections;
        spec_file << fmt::format( "INVERTED_INPUT," );
        for ( std::vector<uint64_t> const& section : sections )
        {
          std::vector<std::string> vars_no_helper;
          for ( uint64_t hash : section )
          {
            if ( ( uint64_t )( hash >> 40 ) != 0u )
            {
              /* helper variables are skipped            */
              vars_no_helper.push_back( DFF_var( hash ).str() );
            }
          }
          spec_file << fmt::format( "{},", fmt::join( vars_no_helper, "," ) );
        }
        spec_file << fmt::format( "\n" );
      }
    }
  }
}


void write_snakes_t1( std::vector<Snake> const& snakes, phmap::flat_hash_map<klut::node, std::vector<Snake>> const& t1_input_constraint, 
                       phmap::flat_hash_map<klut::node, std::array<bool, 3>> const& input_phases, std::vector<DFF_var> const& helpers, 
                       DFF_registry & DFF_REG, std::vector<uint64_t> const& required_SA_DFFs, const std::string cfg_name, uint8_t n_phases, bool verbose = false )
{
  std::ofstream spec_file( cfg_name );

  /* basic constraints guanranteeing the correct functionality of the multiphase clocking scheme */
  for (const Snake & snake : snakes)
  {
    std::vector<std::string> vars_bucket;
    for (const std::vector<uint64_t> & section : snake.sections)
    {
      std::vector<std::string> vars;
      for (uint64_t hash : section)
      {
        vars.push_back(DFF_REG.str( hash ));
      }
      DEBUG_PRINT("New single phase conflict : {}≤1\n", fmt::join(vars, "+"));
      vars_bucket.push_back(fmt::format(vars.size()>1?"({})":"{}", fmt::join(vars, "+")));
      if (vars.size() > 1)
      {
        spec_file << fmt::format("PHASE,{}\n", fmt::join(vars, ","));
      }
    }
    std::reverse(vars_bucket.begin(), vars_bucket.end());
    DEBUG_PRINT("New buffer requirement : ({})\n", fmt::join(vars_bucket, "|"));
    if (vars_bucket.size() == n_phases)
    {
      spec_file << fmt::format("BUFFER,{}\n", fmt::join(vars_bucket, ","));
    }
  }
  for (const uint64_t & hash : required_SA_DFFs)
  {
    DEBUG_PRINT("New SA_REQUIRED : {}\n", DFF_REG.str( hash ));
    spec_file << fmt::format("SA_REQUIRED,{}\n", DFF_REG.str( hash ));
  }

  /* constraints supporting the usage of T1 gates under the multiphase clocking scheme */

  /* (1) the phases of the three inputs to a T1 gate shall be different from each other*/
  for ( auto it{ t1_input_constraint.begin() }; it != t1_input_constraint.end(); ++it )
  {
    const klut::node repr{ it->first };
    for ( Snake const& snake : it->second )
    {
      /* for each of the three input signals */
      spec_file << fmt::format( "T1,{},", repr );
      for ( std::vector<uint64_t> const& section : snake.sections )
      {
        std::vector<std::string> vars;
        for ( uint64_t hash : section )
        {
          vars.push_back( DFF_var( hash ).str() );
        }
        spec_file << fmt::format( "{},", fmt::join( vars, "|" ) );
      }
      spec_file << fmt::format( "\n" );
    }
  }

  /* (2) a helper variable indicates that the source is an AS gate and the distance    */
  /* between the source and the T1 gate is no more than one epoch; a helper variable   */
  /* shall always be assigned to true                                                  */
  for ( DFF_var const& helper : helpers )
  {
    spec_file << fmt::format( "HELPER,{}\n", helper.str() );
  }

  /* (3) if a input to a T1 is negated, there should be an inverter inserted in the    */
  /* last epoche before the phase of the T1                                            */
  for ( auto it{ t1_input_constraint.begin() }; it != t1_input_constraint.end(); ++it )
  {
    const klut::node repr{ it->first };
    for ( auto i{ 0u }; i < 3u; ++i )
    {
      if ( input_phases.at( repr )[i] )
      {
        /* this input signal shall be negated, therefore */
        /* at lease one of the variables have to be true */
        auto const& sections = it->second[i].sections;
        spec_file << fmt::format( "INVERTED_INPUT," );
        for ( std::vector<uint64_t> const& section : sections )
        {
          std::vector<std::string> vars_no_helper;
          for ( uint64_t hash : section )
          {
            if ( ( uint64_t )( hash >> 40 ) != 0u )
            {
              /* helper variables are skipped            */
              vars_no_helper.push_back( DFF_var( hash ).str() );
            }
          }
          spec_file << fmt::format( "{},", fmt::join( vars_no_helper, "," ) );
        }
        spec_file << fmt::format( "\n" );
      }
    }
  }
}



/// @brief 
/// @param path 
/// @param NR 
/// @param DFF_REG 
/// @param n_phases 
/// @param verbose 
/// @return 

std::vector<Snake> sectional_snake(const Path & path, klut & ntk,  DFF_registry & DFF_REG, uint8_t n_phases, bool verbose = false)
{
  std::vector<Snake> out_snakes; 
  std::vector<Snake> stack;
  
  DEBUG_PRINT("[i]: Starting extraction of worms \n");
  // get all DFFs 
  for (const auto & [hash, dff]: DFF_REG.variables)
  {
    NodeData fo_data { ntk.value( dff.fanout ) };
    auto fanout_sigma = fo_data.sigma - ( fo_data.type == AS_GATE );
    auto it = std::find(path.targets.begin(), path.targets.end(), dff.fanout);
    if (it != path.targets.end() && ( ( fanout_sigma < 0 ) || ( ( fanout_sigma >= 0 ) && ( dff.sigma >= static_cast<uint32_t>( fanout_sigma ) ) ) ))
    {
      stack.emplace_back( hash );
    }
  }
  
  while (!stack.empty())
  {
    DEBUG_PRINT("[i] Stack size is {} \n", stack.size());
    Snake snake = stack.back();
    stack.pop_back();

    DEBUG_PRINT("\t[i] The snake has {} sections\n", snake.sections.size());
    uint64_t hash = snake.sections.back().back();
    DFF_var & dff = DFF_REG.at( hash );

    // fmt::print("\tCurrent worm size {}, between phases {} and {} \n", worm.size(), DFF_REG.str(worm.front()), DFF_REG.str(worm.back()));


    DEBUG_PRINT("\t\t[i] The DFF {} has {} parents\n", DFF_REG.at( hash ).str(),  dff.parent_hashes.size() );

    bool returned_current_snake = false;
    for (const uint64_t parent_hash : dff.parent_hashes)
    {
      Snake snake_copy = snake; 
      DEBUG_PRINT("\t\t[i] Advancing towards fanin {}\n", DFF_REG.at( parent_hash ).str() );
      bool status = snake_copy.append(parent_hash, DFF_REG, n_phases);
      DEBUG_PRINT((status) ? "\t\t\tAdded new section!\n" :"\t\t\tExtended existing section!\n"  );
      DEBUG_PRINT("\t\t\tThe new length is {}\n", snake_copy.sections.size() );
      
      stack.push_back( snake_copy );
      if (status && !returned_current_snake && snake_copy.sections.size() == n_phases)
      {
        DEBUG_PRINT("\t\tAdding the snake to the output\n");
        out_snakes.push_back(snake);
        returned_current_snake = true;
      }
    }
  }
  return out_snakes;
}

std::tuple<std::vector<Snake>, phmap::flat_hash_map<klut::node, std::vector<Snake>>, std::vector<DFF_var>> 
sectional_snake_t1( Path const& path, klut const& ntk, 
                    phmap::flat_hash_map<klut::node, std::array<uint64_t, 3>> const& DFF_closest_to_t1s, 
                    DFF_registry& DFF_REG, uint8_t n_phases, bool verbose = false )
{
  std::vector<Snake> out_snakes; 
  std::vector<Snake> stack;
  std::vector<DFF_var> helpers;

  DEBUG_PRINT("[i]: Starting extraction of worms \n");
  // get all DFFs 
  for ( const auto & [hash, dff]: DFF_REG.variables )
  {
    NodeData fo_data { ntk.value( dff.fanout ) };
    auto fanout_sigma = fo_data.sigma - ( fo_data.type == AS_GATE || fo_data.type == T1_GATE );
    if ( auto it = std::find( path.targets.begin(), path.targets.end(), dff.fanout ); it != path.targets.end() && ( dff.sigma == static_cast<uint32_t>( fanout_sigma ) ) )
    {
      stack.emplace_back( hash );
    }
  }
  
  while ( !stack.empty() )
  {
    DEBUG_PRINT("[i] Stack size is {} \n", stack.size());
    Snake snake = stack.back();
    stack.pop_back();

    DEBUG_PRINT("\t[i] The snake has {} sections\n", snake.sections.size());
    uint64_t hash = snake.sections.back().back();
    DFF_var & dff = DFF_REG.at( hash );

    // fmt::print("\tCurrent worm size {}, between phases {} and {} \n", worm.size(), DFF_REG.str(worm.front()), DFF_REG.str(worm.back()));


    DEBUG_PRINT("\t\t[i] The DFF {} has {} parents\n", DFF_REG.at( hash ).str(),  dff.parent_hashes.size() );

    bool returned_current_snake = false;
    for (const uint64_t parent_hash : dff.parent_hashes)
    {
      Snake snake_copy = snake; 
      DEBUG_PRINT("\t\t[i] Advancing towards fanin {}\n", DFF_REG.at( parent_hash ).str() );
      bool status = snake_copy.append(parent_hash, DFF_REG, n_phases);
      DEBUG_PRINT((status) ? "\t\t\tAdded new section!\n" :"\t\t\tExtended existing section!\n"  );
      DEBUG_PRINT("\t\t\tThe new length is {}\n", snake_copy.sections.size() );
      
      stack.push_back( snake_copy );
      if (status && !returned_current_snake && snake_copy.sections.size() == n_phases)
      {
        DEBUG_PRINT("\t\tAdding the snake to the output\n");
        out_snakes.push_back(snake);
        returned_current_snake = true;
      }
    }
  }

  /* for the proper functioning of T1 gates, constraints shall be    */
  /* created to guarantee that the phases of the three inputs of an  */
  /* T1 gate are different                                           */
  phmap::flat_hash_map<klut::node, std::vector<Snake>> t1_input_constraint; 
  for ( auto it = DFF_closest_to_t1s.begin() ; it != DFF_closest_to_t1s.end(); ++it )
  {
    auto const& target_DFFs = it->second;
    /* the eventual size of the 'snakes' vector is three             */
    std::vector<Snake> snakes;

    for ( auto target_DFF_hash : target_DFFs )
    {
      if ( target_DFF_hash == 0u )
      {
        /* TO CONFIRM: is this possible? */
        snakes.push_back( Snake() );
        continue;
      }

      Snake snake{ target_DFF_hash };
      uint32_t snake_len = snake.sections.size();

      /* only the DFF variables whose distance to the target T1 gate */
      /* is no more than 'n_phases' are of interest                  */
      while ( snake_len < n_phases )
      {
        uint32_t snake_len_new{ 0u };
        /* collect the DFF variables within the phases of interest   */
        /* in a breadth-first manner                                 */
        for ( auto i = 0u; i < snake.sections[snake_len - 1].size(); ++i )
        {
          /* notice that the upper bound on 'i' is dynamic, as it is */
          /* possible that a DFF and its parent are assigned to the  */
          /* same phase                                              */
          const uint64_t hash = snake.sections[snake_len - 1][i];
          DFF_var const& dff = DFF_REG.at( hash );
          if ( dff.parent_hashes.size() > 1 && ( static_cast<NodeData>( ntk.value( dff.fanin ) ).type == AA_GATE ) )
          {
            /* early termination of tracking, because a CB is encountered      */
            break;
          }

          /* TO CONFIRM: is it necessary to check the gate type of the fanin   */
          /* to make sure the fanin is a confluence buffer?                    */
          assert( dff.parent_hashes.size() <= 1 );

          for ( const uint64_t parent_hash : dff.parent_hashes )
          {
            uint32_t snake_len_tmp = snake.append( parent_hash, DFF_REG );
            snake_len_new = ( snake_len_tmp > snake_len_new ) ? snake_len_tmp : snake_len_new;
          }
        }
        /* if all the DFF variables belonging to the current phase   */
        /* are iterated, but none of their parents belong to a       */
        /* smaller phase, it means we have already reached a source  */
        if ( snake_len_new == snake_len )
        {
          /* create helper variables if the source is an AS gate     */
          DFF_var const& earliest_dff = DFF_REG.at( snake.sections.back().back() );
          NodeData fanin_data{ ntk.value( earliest_dff.fanin ) };
          if ( fanin_data.type == AS_GATE )
          {
            assert( fanin_data.sigma == earliest_dff.sigma - 1 );
            DFF_var helper{ earliest_dff.fanin, fanin_data.sigma };
            snake.append( helper );
            /* collect helper variables */
            helpers.push_back( helper );
          }
          break;
        }
        else
        {
          assert( snake_len_new > snake_len );
          snake_len = snake_len_new;
        }
      }

      snakes.push_back( snake );
    }

    t1_input_constraint.emplace( it->first, snakes );
  }

  return std::make_tuple( out_snakes, t1_input_constraint, helpers );
}

std::tuple<int, std::unordered_map<unsigned int, unsigned int>, std::string>  cpsat_macro_opt(const std::string & cfg_name, uint8_t n_phases) 
{
  std::string command = fmt::format("{} {} {} {}", PYTHON_EXECUTABLE, PYTHON_PHASE_ASSIGNMENT, n_phases, cfg_name);
  
  std::string pattern = "Objective value: (\\d+)";

  // Run the command and capture its output
  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe) 
  {
    std::cerr << "Error running the command." << std::endl;
    throw;
  }

  char buffer[128];
  std::string output;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) 
  {
    output += buffer;
  }
  fmt::print(output);

  int result = pclose(pipe);
  if (result == -1) 
  {
    std::cerr << "Error closing the command pipe." << std::endl;
    throw;
  }

  // Parse output
  std::istringstream iss(output);

  std::string line;
  std::string solve_status;
  int objective_value;
  std::unordered_map<unsigned int, unsigned int> key_value_pairs;

  while (std::getline(iss, line))
  {
    if (line.find("Solve status:") != std::string::npos) 
    {
      if (line.find("OPTIMAL") || line.find("FEASIBLE"))
      {
        solve_status = "SUCCESS";
        break;
      }
      else
      {
        solve_status = "UNKNOWN";
        return {0, {}, ""};
      }
    }
  }

  // Parse the second line (Objective value)
  std::getline(iss, line);
  if (line.find("Objective value: ") != std::string::npos) 
  {
      objective_value = std::stoi(line.substr(17));
  } 
  else 
  {
      // Handle missing or incorrect format for objective value
      std::cerr << "Error: Objective value not found or invalid format." << std::endl;
      return {0, {}, ""};
  }

  // Parse the key-value pairs in subsequent lines
  while (std::getline(iss, line)) 
  {
      std::istringstream line_stream(line);
      unsigned int key, value;
      char colon;
      if (line_stream >> key >> colon >> value && colon == ':') 
      {
          key_value_pairs[key] = value;
      } 
      else 
      {
          // Handle incorrect format for key-value pairs
          std::cerr << "Error: Invalid format for key-value pairs." << std::endl;
          return {0, {}, ""};
      }
  }

  return {objective_value, key_value_pairs, solve_status};
}

  // TODO : record the timing constraints for the ILP
  // TODO : record the mapping from src to tgt network
  // TODO : record the data pertaining to each node :
  //        - whether the element is AA, AS, or SA
  //          - AA elements are placed at the phase of the latest input
  //          - SA elements tie the preceding AS elements to itself to ensure simultaneous arrival of pulses  
  //        - anything else???

// Function to read unordered_map from CSV file
std::unordered_map<std::string, int> readCSV(const std::string& filename) 
{
    std::ifstream infile(filename);             // Open the input file stream
    std::unordered_map<std::string, int> map;   // Create the unordered_map
    
    std::string line;
    std::getline(infile, line);                 // Ignore the header row

    // Read each subsequent row and add the key-value pair to the unordered_map
    while (std::getline(infile, line)) 
    {
        std::stringstream ss(line);
        std::string key;
        int value;
        std::getline(ss, key, ',');
        ss >> value;
        map[key] = value;
    }
    infile.close(); // Close the input file stream
    return map;
}

int cpsat_ortools(const std::string & cfg_name) 
{
  std::string command = fmt::format("{} {} {}", PYTHON_EXECUTABLE, PYTHON_DFF_PLACEMENT, cfg_name);
  std::string pattern = "Objective value: (\\d+)";

  // Run the command and capture its output
  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe) 
  {
    std::cerr << "Error running the command." << std::endl;
    return -1;
  }

  char buffer[128];
  std::string output;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) 
  {
    output += buffer;
  }
  fmt::print(output);

  int result = pclose(pipe);
  if (result == -1) 
  {
    std::cerr << "Error closing the command pipe." << std::endl;
    return -1;
  }

  // Use regex to find the objective value in the output
  std::regex regex(pattern);
  std::smatch match;
  if (std::regex_search(output, match, regex) && match.size() > 1) 
  {
    std::string value_str = match[1];
    return std::stoi(value_str);
  } 
  else 
  {
    std::cerr << "Objective value not found in the output." << std::endl;
    return -1;
  }
}


int cpsat_ortools_union(const std::string & cfg_name, const uint8_t n_phases) 
{
  std::string command = fmt::format("{} {} {} {}", PYTHON_EXECUTABLE, PYTHON_DFF_PLACEMENT_UNION, cfg_name, n_phases);
  fmt::print("Executing command:\n{}\n", command);
  std::string pattern = "Objective value: (\\d+)";

  // Run the command and capture its output
  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe) 
  {
    std::cerr << "Error running the command." << std::endl;
    return -1;
  }

  char buffer[128];
  std::string output;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) 
  {
    output += buffer;
  }
  fmt::print(output);

  int result = pclose(pipe);
  if (result == -1) 
  {
    std::cerr << "Error closing the command pipe." << std::endl;
    return -1;
  }

  // Use regex to find the objective value in the output
  std::regex regex(pattern);
  std::smatch match;
  if (std::regex_search(output, match, regex) && match.size() > 1) 
  {
    std::string value_str = match[1];
    return std::stoi(value_str);
  } 
  else 
  {
    std::cerr << "Objective value not found in the output." << std::endl;
    return -1;
  }
}

uint32_t get_node_cost( const uint32_t gate_type )
{
  switch( gate_type )
  {
  case 3:
    return COSTS_MAP[fNOT];
  case 4:
    return COSTS_MAP[fAND];
  case 6:
    return COSTS_MAP[fOR];
  case 12:
    return COSTS_MAP[fXOR];
  default:
    std::cerr << "[e] detected unsupported gate type in the decomposed 2-LUT\n";
    return 0u;
  }
}

uint32_t deref_node( klut& ntk, const std::array<klut::node, 3> leaves, const klut::node n, std::vector<klut::node>& has_overlap )
{
  /* return the total cost of nodes in the original cut that can be      */
  /* removed, if implemented using an T1 cell                            */
  if ( std::find( leaves.begin(), leaves.end(), n ) != leaves.end() )
  {
    return 0u;
  }

  uint32_t gain = get_node_cost( ntk.func_lit( n ) );
  ntk.foreach_fanin( n, [&]( auto const& ni ) {
    if ( ntk.fanout_size( ni ) == 0 )
    {
      /* there is an overlap between the current cut and a previously    */
      /* checked cut                                                     */
      has_overlap.push_back( ni );
    }
    /* skip fanins that are leaves */
    else if ( ( std::find( leaves.begin(), leaves.end(), ni ) == leaves.end() ) && 
              ( ntk.decr_fanout_size( ni ) == 0 ) )
    {
      gain += deref_node( ntk, leaves, ntk.node_to_index( ni ), has_overlap );
    }
    else
    {
      /* even if a node is not in the MFFC, reduce its fanout size by 1  */
      /* can save a splitter                                             */
      gain += COSTS_MAP[fSPL];
    }
  } );

  return gain;
}

void reref_node( klut& ntk, const std::array<klut::node, 3> leaves, const klut::node n )
{
  /* the inverse process of 'deref_root_node', invoked if the T1 cell    */
  /* based implementation turns out to be more expensive                 */
  if ( auto it_find = std::find( leaves.begin(), leaves.end(), n ); it_find != leaves.end() )
  {
    return;
  }

  ntk.foreach_fanin( n, [&]( auto const& ni ) {
    if ( ( std::find( leaves.begin(), leaves.end(), ni ) == leaves.end() ) && 
         ( ntk.incr_fanout_size( ni ) == 0 ) )
    {
      reref_node( ntk, leaves, ntk.node_to_index( ni ) );
    }
  } );
}

/* a special vertion of the 'reref_node' fuction, which has an extra param of 'has_overlap' */
void reref_node( klut& ntk, const std::array<klut::node, 3> leaves, const klut::node n, std::vector<klut::node> const& has_overlap )
{
  if ( auto it_find = std::find( leaves.begin(), leaves.end(), n ); it_find != leaves.end() )
  {
    return;
  }

  ntk.foreach_fanin( n, [&]( auto const& ni ) {
    if ( ( std::find( leaves.begin(), leaves.end(), ni ) == leaves.end() ) && 
         ( std::find( has_overlap.begin(), has_overlap.end(), ni ) == has_overlap.end() ) && 
         ( ntk.incr_fanout_size( ni ) == 0 ) )
    {
      reref_node( ntk, leaves, ntk.node_to_index( ni ), has_overlap );
    }
  } );
}

/* If an overlap if detected during the 'deref_node' process, i.e., a node whose fanout     */
/* is already 0, this does not mean such a node is in the MFFC, but there is a conflict     */
/* between current cut and a previously checked cut. Thus, to avoid conflict, the result of */
/* the sanity check on the current cut would be directly set to false.                      */
bool t1_usage_sanity_check( klut& ntk, std::pair<const std::array<klut::node, 3>, T1_OUTPUTS>& t1_candidate, int64_t& updated_area )
{
  int32_t gain{ 0 };
  const std::array<klut::node, 3> leaves = std::get<0>( t1_candidate );
  T1_OUTPUTS& t1_outputs = std::get<1>( t1_candidate );
  std::array<klut::node, 3> roots;
  roots[0] = ntk.node_to_index( t1_outputs.sum_to );
  roots[1] = std::max( ntk.node_to_index( t1_outputs.carry_to ), ntk.node_to_index( t1_outputs.inv_carry_to ) );
  roots[2] = std::max( ntk.node_to_index(  t1_outputs.cbar_to ), ntk.node_to_index(  t1_outputs.inv_cbar_to ) );

  std::vector<klut::node> has_overlap;

  for ( const klut::node root : roots )
  {
    if ( root != ntk.get_constant( false ) )
    {
      gain += static_cast<int32_t>( deref_node( ntk, leaves, root, has_overlap ) );
    }
  }
  gain -= static_cast<int32_t>( __builtin_popcount( t1_outputs.in_phase ) * ( COSTS_MAP[fNOT] - COSTS_MAP[fDFF] ) );
  gain -= static_cast<int32_t>( COSTS_SUNMAGNETICS_EXTENDED[fT1] );

  if ( !has_overlap.empty() )
  {
    for ( const auto root : roots )
    {
      reref_node( ntk, leaves, root, has_overlap );
    }
    return false;
  }
  
  if ( gain < 0 )
  {
    for ( const auto root : roots )
    {
      reref_node( ntk, leaves, root );
    }

    return false;
  }
  /* update gate type for the committed T1 cells */
  // if ( t1_outputs.has_sum ) { ntk.set_value( t1_outputs.sum_to, NodeData( static_cast<NodeData>( ntk.value( t1_outputs.sum_to ) ).sigma, T1_GATE ).value ); }
  // if ( t1_outputs.has_carry ) { ntk.set_value( t1_outputs.carry_to, NodeData( static_cast<NodeData>( ntk.value( t1_outputs.carry_to ) ).sigma, T1_GATE ).value ); }
  // if ( t1_outputs.has_carry_inverted ) { ntk.set_value( t1_outputs.inv_carry_to, NodeData( static_cast<NodeData>( ntk.value( t1_outputs.inv_carry_to ) ).sigma, T1_GATE ).value ); }
  // if ( t1_outputs.has_cbar ) { ntk.set_value( t1_outputs.cbar_to, NodeData( static_cast<NodeData>( ntk.value( t1_outputs.cbar_to ) ).sigma, T1_GATE ).value ); }
  // if ( t1_outputs.has_cbar_inverted ) { ntk.set_value( t1_outputs.inv_cbar_to, NodeData( static_cast<NodeData>( ntk.value( t1_outputs.inv_cbar_to ) ).sigma, T1_GATE ).value ); }

  updated_area -= gain;

  /* update the fanout size of the leaves */
  mockturtle::fanout_view<klut> ntk_fo{ ntk };
  for ( auto const& leaf : leaves )
  {
    uint32_t leaf_fanout_size{ 0u };
    ntk_fo.foreach_fanout( leaf, [&]( auto const& no ) {
      if ( !ntk.is_dangling( no ) )
      {
        ++leaf_fanout_size;
      }
    } );
    /* increase by one as it is an input to the current T1 gate */
    ++leaf_fanout_size;
    ntk._storage->nodes[leaf].data[0].h1 = leaf_fanout_size;
  }

  return true;
}

void substitute_node( klut& ntk, klut::node const& old_node, klut::signal const& new_signal )
{
  NodeData old_node_data = ntk.value( old_node );
  ntk.set_value( new_signal, NodeData( old_node_data.sigma, T1_GATE ).value );
  ntk.substitute_node( old_node, new_signal );
  fmt::print( "[i] Substitute {} with {}\n", old_node, new_signal );
}

void update_representative( klut const& ntk, klut::signal const& new_signal, klut::signal& repr, 
                            phmap::flat_hash_map<klut::signal, klut::signal>& representatives )
{
  /* the first non-dangling output port is selected as the representitive */
  if ( repr == ntk.get_constant( false ) )
  {
    repr = new_signal;
  }
  representatives.emplace( new_signal, repr );
}

void update_network( klut& ntk, array_map<3, T1_OUTPUTS> const& t1_candidates, 
                     phmap::flat_hash_map<klut::node, klut::node>& representatives, 
                     phmap::flat_hash_map<klut::node, klut::node>& symbol2real, 
                     phmap::flat_hash_map<klut::node, std::array<bool, 3>>& input_phases )
{
  for ( auto const& t1_candidate : t1_candidates )
  {
    auto const& leaves = std::get<0>( t1_candidate );
    auto const& t1_outputs = std::get<1>( t1_candidate );
    klut::node repr = ntk.get_constant( false );

    if ( t1_outputs.has_sum )
    {
      const auto new_signal = ntk.create_xor3( leaves[0], leaves[1], leaves[2] );
      fmt::print( "[i] Created SUM: {} = XOR3( {}, {}, {} )\n", new_signal, leaves[0], leaves[1], leaves[2] );
      update_representative( ntk, new_signal, repr, representatives );
      symbol2real.emplace( t1_outputs.sum_to, new_signal );
    }
    if ( t1_outputs.has_carry )
    {
      const auto new_signal = ntk.create_maj( leaves[0], leaves[1], leaves[2], 0 );
      fmt::print( "[i] Created CARRY: {} = MAJ3( {}, {}, {} )\n", new_signal, leaves[0], leaves[1], leaves[2] );
      update_representative( ntk, new_signal, repr, representatives  );
      symbol2real.emplace( t1_outputs.carry_to, new_signal );
    }
    if ( t1_outputs.has_carry_inverted )
    {
      const auto new_signal = ntk.create_maj( leaves[0], leaves[1], leaves[2], 1 );
      fmt::print( "[i] Created CARRY_INV: {} = MAJ3( {}, {}, {} )\n", new_signal, leaves[0], leaves[1], leaves[2] );
      update_representative( ntk, new_signal, repr, representatives );
      symbol2real.emplace( t1_outputs.inv_carry_to, new_signal );
    }
    if ( t1_outputs.has_cbar )
    {
      const auto new_signal = ntk.create_or3( leaves[0], leaves[1], leaves[2], 0 );
      fmt::print( "[i] Created CBAR: {} = OR3( {}, {}, {} )\n", new_signal, leaves[0], leaves[1], leaves[2] );
      update_representative( ntk, new_signal, repr, representatives );
      symbol2real.emplace( t1_outputs.cbar_to, new_signal );
    }
    if ( t1_outputs.has_cbar_inverted )
    {
      const auto new_signal = ntk.create_or3( leaves[0], leaves[1], leaves[2], 1 );
      fmt::print( "[i] Created CBAR_INV: {} = OR3( {}, {}, {} )\n", new_signal, leaves[0], leaves[1], leaves[2] );
      update_representative( ntk, new_signal, repr, representatives );
      symbol2real.emplace( t1_outputs.inv_cbar_to, new_signal );
    }

    auto input_phase = t1_outputs.in_phase;
    std::array<bool, 3> input_phase_bool{ { static_cast<bool>( input_phase >> 0 & 1 ), static_cast<bool>( input_phase >> 1 & 1 ), static_cast<bool>( input_phase >> 2 & 1 ) } };
    input_phases.emplace( repr, input_phase_bool );
  }

  /* since the enumeration of all T1 cells are performed in an unordered manner, */
  /* the nodes creation and substitution have to be handled seperately           */
  for ( auto const& t1_candidate : t1_candidates )
  {
    T1_OUTPUTS const& t1_outputs = std::get<1>( t1_candidate );

    if ( t1_outputs.has_sum )
    {
      substitute_node( ntk, t1_outputs.sum_to, symbol2real.at( t1_outputs.sum_to ) );
    }
    if ( t1_outputs.has_carry )
    {
      substitute_node( ntk, t1_outputs.carry_to, symbol2real.at( t1_outputs.carry_to ) );
    }
    if ( t1_outputs.has_carry_inverted )
    {
      substitute_node( ntk, t1_outputs.inv_carry_to, symbol2real.at( t1_outputs.inv_carry_to ) );
    }
    if ( t1_outputs.has_cbar )
    {
      substitute_node( ntk, t1_outputs.cbar_to, symbol2real.at( t1_outputs.cbar_to ) );
    }
    if ( t1_outputs.has_cbar_inverted )
    {
      substitute_node( ntk, t1_outputs.inv_cbar_to, symbol2real.at( t1_outputs.inv_cbar_to ) );
    }
  }

  //ntk = mockturtle::cleanup_dangling( ntk );
}

/* this function is merely for debugging and can be removed */
std::string get_gate_type( const uint32_t gate_type )
{
  switch( gate_type )
  {
  case 3:
    return "a NOT gate";
  case 4:
    return "an AND gate";
  case 6:
    return "an OR gate";
  case 12:
    return "an XOR gate";
  default:
    return "an unknown gate with the functional literal of " + std::to_string( gate_type );
  }
}

/* this function is merely for debugging and can be removed */
void profile_klut_node( klut const& ntk, klut::node const& n )
{
  mockturtle::fanout_view<klut, false> ntk_fanout{ ntk };
  fmt::print( "Profiling Node {} :\n", n );
  fmt::print( "\tGate type : {}\n", get_gate_type( ntk.func_lit( n ) ) );
  fmt::print( "\tFanins : " );
  /* notice that 'signal' and 'node' are the same concept in KLUTs */
  ntk.foreach_fanin( n, []( auto const& ni ) {
    fmt::print( "Node {}  ", ni );
  } );
  fmt::print( "\n" );
  fmt::print( "\tFanouts ( Size = {} ): ", ntk.fanout_size( n ) );
  ntk_fanout.foreach_fanout( n, []( auto const& no ) {
    fmt::print( "Node {}  ", no );
  } );
  fmt::print( "\n" );
}

void write_klut_specs_supporting_t1( klut const& ntk, array_map<3, T1_OUTPUTS> const& t1s, std::string const& filename )
{
  std::set<klut::signal> t1_inputs;
  for ( auto const& [leaves, t1_outputs] : t1s )
  {
    t1_inputs.insert(leaves.begin(), leaves.end());
  }

  std::ofstream spec_file( filename );

  spec_file << "PI";
  ntk.foreach_pi( [&] (const auto & node)
  {
    spec_file << "," << node;
  });
  spec_file << "\n";

  ntk.foreach_gate( [&]( auto const& n ) 
  {
    if ( ntk.is_dangling( n ) && t1_inputs.count( n ) == 0 && !ntk.is_po( n ) )
    {
      fmt::print("Node {} is dangling\n", n);
      /* a dangling node due to the usage of T1 cells */
      return true;
    }

    if ( static_cast<NodeData>( ntk.value( n ) ).type == T1_GATE )
    {
      fmt::print("Node {} is T1 output\n", n);
      /* T1 cells would be handled together later     */
      return true;
    }

    fmt::print("Node {} is a regular node, with fanout_size {}\n", n, ntk.fanout_size( n ) );

    std::vector<klut::node> n_fanins;
    ntk.foreach_fanin( n, [&n_fanins]( auto const& ni ) {
      /* notice that 'signal' and 'node' are equal    */
      /* in kluts                                     */
      // n_fanins.push_back( ntk.node_to_index( ni ) );
      n_fanins.push_back( ni );
    } );

    spec_file << fmt::format( "{0},{1},{2}\n", n, static_cast<NodeData>( ntk.value( n ) ).type, fmt::join( n_fanins, "|" ) );
    // spec_file << fmt::format( "{0},{1},{2}\n", ntk.node_to_index( n ), static_cast<NodeData>( ntk.value( n ) ).type, fmt::join( n_fanins, "|" ) );
    return true;
  } );

  /* write information of the committed T1 cells into  */
  /* the csv file                                      */
  for ( auto const& t1 : t1s )
  {
    /* the 5 output ports are in the order of: sum,    */
    /* carry, inverted carry, cbar, and inverted cbar  */
    std::vector<klut::node> output_ports( 5, 0u );
    output_ports[0] =       t1.second.sum_to;
    output_ports[1] =     t1.second.carry_to;
    output_ports[2] = t1.second.inv_carry_to;
    output_ports[3] =      t1.second.cbar_to;
    output_ports[4] =  t1.second.inv_cbar_to;

    /* combine input phases with input ports           */
    std::array<std::string, 3> input_ports;
    input_ports[0] = ( ( t1.second.in_phase >> 0 & 1 ) ? "~" : "" ) + std::to_string( t1.first[0] );
    input_ports[1] = ( ( t1.second.in_phase >> 1 & 1 ) ? "~" : "" ) + std::to_string( t1.first[1] );
    input_ports[2] = ( ( t1.second.in_phase >> 2 & 1 ) ? "~" : "" ) + std::to_string( t1.first[2] );

    spec_file << fmt::format( "{0},{1},{2}\n", fmt::join( output_ports, "|" ), 4u, fmt::join( input_ports, "|" ) );
  }
}

void write_klut_specs_supporting_t1_new( klut const& ntk, array_map<3, T1_OUTPUTS> const& t1s, std::string const& filename, 
                                         phmap::flat_hash_map<klut::signal, klut::signal> const& symbol2real )
{
  std::ofstream spec_file( filename );

  spec_file << "PI";
  ntk.foreach_pi( [&]( const auto & node ){
    spec_file << "," << node;
  } );
  spec_file << "\n";

  ntk.foreach_gate( [&]( auto const& n ) 
  {
    if ( ntk.is_dangling( n ) )
    {
      /* a dangling node due to the usage of T1 cells */
      fmt::print("Node {} is dangling\n", n);
      // profile_klut_node( ntk, n );
      return true;
    }

    if ( static_cast<NodeData>( ntk.value( n ) ).type == T1_GATE )
    {
      fmt::print("Node {} is T1 output\n", n);
      /* T1 cells would be handled together later     */
      return true;
    }

    fmt::print("Node {} is a regular node, with fanout_size {}\n", n, ntk.fanout_size( n ) );

    std::vector<klut::node> n_fanins;
    ntk.foreach_valid_fanin( n, [&n_fanins]( auto const& ni ) {
      /* notice that 'signal' and 'node' are equal    */
      /* in kluts                                     */

      n_fanins.push_back( ni );
    } );

    spec_file << fmt::format( "{0},{1},{2}\n", n, static_cast<NodeData>( ntk.value( n ) ).type, fmt::join( n_fanins, "|" ) );
    // spec_file << fmt::format( "{0},{1},{2}\n", ntk.node_to_index( n ), static_cast<NodeData>( ntk.value( n ) ).type, fmt::join( n_fanins, "|" ) );
    return true;
  } );

  /* write information of the committed T1 cells into  */
  /* the csv file                                      */
  for ( auto const& t1 : t1s )
  {
    auto const& leaves = std::get<0>( t1 );
    auto const& t1_outputs = std::get<1>( t1 );

    /* the 5 output ports are in the order of: sum,    */
    /* carry, inverted carry, cbar, and inverted cbar  */
    std::vector<klut::node> output_ports( 5, 0u );
    output_ports[0] = ntk.node_to_index( (       t1_outputs.sum_to == ntk.get_constant( false ) ) ? ntk.get_constant( false ) : symbol2real.at(       t1_outputs.sum_to ) );
    output_ports[1] = ntk.node_to_index( (     t1_outputs.carry_to == ntk.get_constant( false ) ) ? ntk.get_constant( false ) : symbol2real.at(     t1_outputs.carry_to ) );
    output_ports[2] = ntk.node_to_index( ( t1_outputs.inv_carry_to == ntk.get_constant( false ) ) ? ntk.get_constant( false ) : symbol2real.at( t1_outputs.inv_carry_to ) );
    output_ports[3] = ntk.node_to_index( (      t1_outputs.cbar_to == ntk.get_constant( false ) ) ? ntk.get_constant( false ) : symbol2real.at(      t1_outputs.cbar_to ) );
    output_ports[4] = ntk.node_to_index( (  t1_outputs.inv_cbar_to == ntk.get_constant( false ) ) ? ntk.get_constant( false ) : symbol2real.at(  t1_outputs.inv_cbar_to ) );

    /* combine input phases with input ports           */
    std::array<std::string, 3> input_ports;
    input_ports[0] = ( ( t1.second.in_phase >> 0 & 1 ) ? "~" : "" ) + std::to_string( ( symbol2real.count( leaves[0] ) ) ? symbol2real.at( leaves[0] ) : leaves[0] );
    input_ports[1] = ( ( t1.second.in_phase >> 1 & 1 ) ? "~" : "" ) + std::to_string( ( symbol2real.count( leaves[1] ) ) ? symbol2real.at( leaves[1] ) : leaves[1] );
    input_ports[2] = ( ( t1.second.in_phase >> 2 & 1 ) ? "~" : "" ) + std::to_string( ( symbol2real.count( leaves[2] ) ) ? symbol2real.at( leaves[2] ) : leaves[2] );

    spec_file << fmt::format( "{0},{1},{2}\n", fmt::join( output_ports, "|" ), 4u, fmt::join( input_ports, "|" ) );
  }
}

int main(int argc, char* argv[])  //
{
  using namespace experiments;
  using namespace mockturtle;


  TT3 XOR3, MAJ3, OR3;
  XOR3._bits = 0x96;
  MAJ3._bits = 0xe8;
  OR3._bits  = 0xfe;

  std::vector<TT3> xor3_tts;
  std::vector<TT3> maj3_tts;
  std::vector<TT3>  or3_tts;
  auto add_xor3 = [&xor3_tts](const TT3 & tt){xor3_tts.push_back(tt);};
  auto add_maj3 = [&maj3_tts](const TT3 & tt){maj3_tts.push_back(tt);};
  auto add_or3  =  [&or3_tts](const TT3 & tt){ or3_tts.push_back(tt);};
  kitty::exact_npn_canonization(XOR3, add_xor3);
  kitty::exact_npn_canonization(MAJ3, add_maj3);
  kitty::exact_npn_canonization( OR3, add_or3 );

  fmt::print("Compatible TTs:\n");
  for (auto i = 0u; i < xor3_tts.size(); ++i)
  {
    fmt::print("\t[{}]:\n", i);
    fmt::print("\t\tXOR3: {0:08b}={0:02x}={0:d}\n", xor3_tts[i]._bits);
    fmt::print("\t\tMAJ3: {0:08b}={0:02x}={0:d}\n", maj3_tts[i]._bits);
    fmt::print("\t\t OR3: {0:08b}={0:02x}={0:d}\n",  or3_tts[i]._bits);
  }
  // return 0;

  experiment<std::string, uint8_t, double, double, double, double> exp( "mapper_t1", "benchmark", "N_PHASES", "#DFF", "area", "delay", "#T1" );

  // uint8_t MIN_N_PHASES = std::stoi(argv[1]);
  // uint8_t MAX_N_PHASES = std::stoi(argv[2]);

  std::vector<uint8_t> PHASES;
  for (auto i = 1; i < argc; ++i)
  {
    PHASES.push_back(std::stoi(argv[i]));
  }
  if ( PHASES.empty() )
  {
    PHASES.push_back( 7 );
  }
  fmt::print("Phases to analyze: [{}]\n", fmt::join(PHASES, ", "));

  fmt::print( "[i] processing technology library\n" );

  // library to map to technology
  std::vector<gate> gates;
  std::ifstream inputFile( DATABASE_PATH );
  if ( lorina::read_genlib( inputFile, genlib_reader( gates ) ) != lorina::return_code::success )
  {
    return 1;
  }

  // std::unordered_map<std::string, int> nDFF_global = readCSV( NDFF_PATH );
  std::unordered_map<std::string, int> nDFF_global;

  mockturtle::tech_library_params tps; // tps.verbose = true;
  tech_library<NUM_VARS, mockturtle::classification_type::p_configurations> tech_lib( gates, tps );

  #pragma region benchmark_parsing
    // *** BENCHMARKS OF INTEREST ***
    // experiments::adder | experiments::div  | 
    auto benchmarks1 = epfl_benchmarks( experiments::square );//  | 
    // auto benchmarks1 = epfl_benchmarks( experiments::adder | experiments::bar  );// | experiments::max  | experiments::multiplier );
    // auto benchmarks1 = epfl_benchmarks( experiments::int2float | experiments::priority | experiments::voter);
    auto benchmarks2 = iscas_benchmarks( );
    // benchmarks1.insert(benchmarks1.end(), benchmarks2.begin(), benchmarks2.end());
    std::reverse(benchmarks1.begin(), benchmarks1.end());

    // *** OPENCORES BENCHMARKS (DO NOT LOOK GOOD) ***
    const std::vector<std::string> BEEREL_BENCHMARKS 
    {
      "simple_spi-gates",
      "des_area-gates",
      "pci_bridge32-gates",
      "spi-gates",
      "mem_ctrl-gates"
    };

    // *** ISCAS89 SEQUENTIAL BENCHMARKS (DO NOT LOOK GOOD) ***
    const std::vector<std::string> ISCAS89_BENCHMARKS {"s382.aig", "s5378.aig", "s13207.aig"};

    // benchmarks1.insert(benchmarks1.end(), ISCAS89_BENCHMARKS.begin(), ISCAS89_BENCHMARKS.end());
    // std::reverse(benchmarks1.begin(), benchmarks1.end());

    // *** LIST ALL CONSIDERED BENCHMARKS ***
    fmt::print("Benchmarks:\n\t{}\n", fmt::join(benchmarks1, "\n\t"));

    // *** READ COMPOUND GATE LIBRARY ***
    phmap::flat_hash_map<ULL, Node> GNM_global;
    bool status = LoadFromFile(GNM_global, NODEMAP_BINARY_PREFIX);
    assert(status);

    phmap::flat_hash_map<std::string, LibEntry> entries = read_LibEntry_map(LibEntry_file);

  #pragma endregion benchmark_parsing

  // *** START PROCESSING BECNHMARKS ***
  for ( auto const& benchmark : benchmarks1 )
  {
    fmt::print( "[i] processing {}\n", benchmark );

    #pragma region load network
    // *** LOAD NETWORK INTO MIG ***
    mig ntk_original;
    if (benchmark.find("-gates") != std::string::npos) 
    {
      fmt::print("USING THE BLIF READER\n");

      std::string abc_command = fmt::format("abc -c \"read_blif {}{}.blif\" -c strash -c \"write_aiger temp.aig\" ", OPENCORES_FOLDER, benchmark);

      std::system(abc_command.c_str());

      klut temp_klut;
      if ( lorina::read_aiger( "temp.aig", aiger_reader( ntk_original ) ) != lorina::return_code::success )
      {
        fmt::print("Failed to read {}\n", benchmark);
        continue;
      }
    }
    else if ( benchmark.find(".aig") != std::string::npos ) // ISCAS89 benchmark
    {
      fmt::print("USING THE BENCH READER\n");
      std::string path = fmt::format("{}{}", ISCAS89_FOLDER, benchmark);
      if ( lorina::read_aiger( path, aiger_reader( ntk_original ) ) != lorina::return_code::success )
      {
        fmt::print("Failed to read {}\n", benchmark);
        continue;
      }
      // convert_klut_to_graph<mig>(ntk_original, temp_klut);
    }
    else // regular benchmark
    {
      fmt::print( "USING THE AIGER READER\n" );
      if ( lorina::read_aiger( benchmark_path( benchmark ), aiger_reader( ntk_original ) ) != lorina::return_code::success )
      {
        fmt::print("Failed to read {}\n", benchmark);
        continue;
      }
      // convert_klut_to_graph<mig>(ntk_original, temp_klut);
    }
    #pragma endregion

    #pragma region mapping with compound gates 
    // *** MAP, NO NEED FOR RETIMING/PATH BALANCING ***
    fmt::print("Started mapping {}\n", benchmark);
    auto [res_wo_pb, st_wo_pb] = map_wo_pb(ntk_original, tech_lib, false); //benchmark, true, nDFF_global, total_ndff_w_pb, total_area_w_pb, cec_w_pb 
    fmt::print("Finished mapping {}\n", benchmark);
    #pragma endregion

    #pragma region decomposition of the mapped network into a klut
    // *** DECOMPOSE COMPOUND GATES INTO PRIMITIVES, REMOVE DFFS, REPLACE OR GATES WITH CB WHERE POSSIBLE ***
    auto _result = decompose_to_klut(res_wo_pb, GNM_global, entries, COSTS_MAP);
    auto klut_decomposed = std::get<0>(_result);
    auto raw_area = std::get<1>(_result);
    fmt::print("Decomposition complete\n");
    #pragma endregion

    #pragma region cut enumeration to find TTs that could be shared with a T1 cell
    // *** ENUMERATE 3-CUTS ***
    cut_enumeration_params ce_params; 
    ce_params.cut_size = 3u;
    const auto cuts = mockturtle::cut_enumeration<klut, true>( klut_decomposed, ce_params );

    /* print enumerated cuts */
    // klut_decomposed.foreach_node( [&]( auto node ) {
    //   auto idx = klut_decomposed.node_to_index( node );
    //   auto & node_cuts = cuts.cuts( idx );
    //   std::cout << node_cuts << "\n";
    // } );

    // *** FIND THOSE CUTS MATCHING THE XOR3/MAJ3/OR3 FUNCTIONS ***
    const auto xor3_cuts = match_cuts( xor3_tts, klut_decomposed, cuts);
    const auto maj3_cuts = match_cuts( maj3_tts, klut_decomposed, cuts);
    const auto  or3_cuts = match_cuts(  or3_tts, klut_decomposed, cuts);

    /* TODO: adopt the assumption that, it is a good deal if an implementation can make use of more than 2 out of the 3 outputs of T1,  */
    /* then we would need four rounds of matching: (1) 3 leaves using all 3 outputs; (2) ...using XOR and MAJ; (3) ...using MAJ and OR; */
    /* (4) ...using XOR and OR. For each 3 leaves found, check validity by deciding which of the 8 T1s to choose                        */

    array_map<3, T1_OUTPUTS> t1_candidates = find_t1_candidates(klut_decomposed, xor3_cuts, maj3_cuts, or3_cuts);

    #pragma endregion

    #pragma region leave only those cuts reducing area
    /* estimate the gain of implementing parts of the circuits using T1s instead */
    auto updated_area{ raw_area };
    // uint32_t num_t1_use_more_than_3{ 0u };
    uint32_t num_t1_cells{ 0u };

    /* Rewrote this using iterator output */
    for ( auto it_t1_cands{ t1_candidates.begin() }; it_t1_cands != t1_candidates.end(); )
    {
      if ( !t1_usage_sanity_check( klut_decomposed, *it_t1_cands, updated_area ) )
      {
        // update iterator after erasing
        it_t1_cands = t1_candidates.erase( it_t1_cands );
      }
      else
      {
        ++it_t1_cands;
        ++num_t1_cells;
      }
    }

    phmap::flat_hash_map<klut::signal, klut::signal> representatives;
    phmap::flat_hash_map<klut::signal, klut::signal> symbol2real;
    phmap::flat_hash_map<klut::node, std::array<bool, 3>> input_phases;
    update_network( klut_decomposed, t1_candidates, representatives, symbol2real, input_phases );

    #pragma endregion

    // start processing each possible number of phases
    for (const auto n_phases : PHASES)
    {
      fmt::print("[i] Mapping with {} phases\n", n_phases);
      // *** IF i = 0, we assign phases with the CP-SAT
      // *** IF i = 1, we assign phases greedily (currently unused)
      // for (auto i = 0; i < 2; ++i)
      for (auto i = 0; i < 1; ++i)
      {
        klut network { klut_decomposed.clone() };
        phmap::flat_hash_map<unsigned int, unsigned int> assignment;

        // *** IF i = 0, "assignment" has stages assigned by the CP-SAT
        // *** IF i = 1, "assignment" is empty
        if (i == 0)
        {
          const std::string ilp_cfg_filename = fmt::format("ilp_configs/{}.csv", benchmark);
          fmt::print("\tWriting config {}\n", ilp_cfg_filename);
          // write_klut_specs(network, ilp_cfg_filename);
          // write_klut_specs_supporting_t1( network, t1_candidates, ilp_cfg_filename );
          write_klut_specs_supporting_t1_new( network, t1_candidates, ilp_cfg_filename, symbol2real );

          continue;

          fmt::print("\tCalling OR-Tools\n");
          auto [obj_val, assignment_local, status] = cpsat_macro_opt(ilp_cfg_filename, n_phases);

          if (status == "SUCCESS") // (true) // 
          {
            assignment.insert(std::make_move_iterator(assignment_local.begin()), std::make_move_iterator(assignment_local.end()));
            fmt::print("[i] CP-SAT PHASE ASSIGNMENT: SUCCESS\n");
          }
          else
          {
            fmt::print("[i] CP-SAT PHASE ASSIGNMENT: FAIL\n");
            continue;
          }
        }
        
        // *** IF i = 0, "assignment" has stages assigned by the CP-SAT
        // *** IF i = 1, "assignment" is empty
        assign_sigma(network, assignment, false );

        // *** Greedily insert splitters
        splitter_ntk_insertion_t1( network, representatives, false );

        // network.foreach_node([&] ( const klut::signal & node ) {if ( network.fanout_size( node ) > 1 ){assert( network.node_function( node ) == 0x2 );};});

        fmt::print("[i] FINISHED PHASE ASSIGNMENT\n");

        fmt::print("[i] EXTRACTING PATHS\n");

        std::vector<Path> paths = extract_paths_t1( network, representatives, true );

        auto total_num_dff = 0u;

        auto cfg_file_ctr = 0u;
        for (const Path & path : paths)
        {
//           fmt::print("\tAnalyzing the path {} out of {}\n", ++path_ctr, paths.size());
//           // *** Create binary variables
//           phmap::flat_hash_map<klut::node, std::array<uint64_t, 3>> DFF_closest_to_t1s;
//           auto [DFF_REG, precalc_ndff, required_SA_DFFs] = dff_vars_single_paths_t1( path, network, n_phases, DFF_closest_to_t1s, true );
//           total_num_dff += precalc_ndff;
//           fmt::print("\t\t\t\t[i]: Precalculated {} DFFs, total #DFF = {}\n", precalc_ndff, total_num_dff);
          
//           // *** Generate constraints
//           auto const& [snakes, t1_input_constraint, helpers] = sectional_snake_t1( path, network, DFF_closest_to_t1s, DFF_REG, n_phases, true );

//           /* If the target gate is a T1 gate, extra constraints shall be added */

//           fmt::print("\tCreated {} snakes\n", snakes.size());
//           // *** If there's anything that needs optimization
//           if (!snakes.empty())
//           {
//             std::string cfg_file = fmt::format("ilp_configs/{}_cfgNR_{}.csv", benchmark, file_ctr++);
//             write_snakes_t1( snakes, t1_input_constraint, input_phases, helpers, DFF_REG, required_SA_DFFs, cfg_file, n_phases, true );


//             continue;


//             auto num_dff = cpsat_ortools(cfg_file);
//             // fmt::print("OR Tools optimized to {} DFF\n", num_dff);
//             total_num_dff += num_dff;
//             fmt::print("\t\t\t\t[i] total CPSAT #DFF = {}\n", total_num_dff);
//           }
          path.print_bfs(network);

          auto [gate_vars, sa_dff, stage_constraints, buffer_constraints, inverted_t1_input, merger_t1_input, truncated_t1_paths] = dff_from_threads(network, path, n_phases, input_phases);

          std::string cfg_file = fmt::format("{}_paths_{}.csv", benchmark, cfg_file_ctr);

          write_dff_cfg(network, cfg_file, gate_vars, sa_dff, stage_constraints, buffer_constraints, inverted_t1_input,  merger_t1_input, truncated_t1_paths  );

          auto num_dff = cpsat_ortools_union(cfg_file, n_phases);
          fmt::print("OR Tools optimized to {} DFF\n", num_dff);
          total_num_dff += num_dff;
          fmt::print("\t\t\t\t[i] total CPSAT #DFF = {}\n", total_num_dff);

          cfg_file_ctr++;
        }
        // continue;

        // auto [DFF_REG, precalc_ndff] = dff_vars(NR, paths, N_PHASES);

        // auto total_num_dff = 0u;
        #if false
          auto file_ctr = 0u;
          auto path_ctr = 0u;
          for (const Path & path : paths)
          {
            fmt::print("\tAnalyzing the path {} out of {}\n", ++path_ctr, paths.size());
            // *** Create binary variables
            phmap::flat_hash_map<klut::node, std::array<uint64_t, 3>> DFF_closest_to_t1s;
            auto [DFF_REG, precalc_ndff, required_SA_DFFs] = dff_vars_single_paths_t1( path, network, n_phases, DFF_closest_to_t1s );
            total_num_dff += precalc_ndff;
            fmt::print("\t\t\t\t[i]: Precalculated {} DFFs, total #DFF = {}\n", precalc_ndff, total_num_dff);
            
            // *** Generate constraints
            auto const& [snakes, t1_input_constraint, helpers] = sectional_snake_t1( path, network, DFF_closest_to_t1s, DFF_REG, n_phases, true );
            /* If the target gate is a T1 gate, extra constraints shall be added */

            fmt::print("\tCreated {} snakes\n", snakes.size());
            // *** If there's anything that needs optimization
            if (!snakes.empty())
            {
              std::string cfg_file = fmt::format("ilp_configs/{}_cfgNR_{}.csv", benchmark, file_ctr++);
              write_snakes_t1( snakes, t1_input_constraint, input_phases, helpers, DFF_REG, required_SA_DFFs, cfg_file, n_phases, true );
              
              continue;

              auto num_dff = cpsat_ortools(cfg_file);
              // fmt::print("OR Tools optimized to {} DFF\n", num_dff);
              total_num_dff += num_dff;
              fmt::print("\t\t\t\t[i] total CPSAT #DFF = {}\n", total_num_dff);
            }
          }
        #endif
        // *** Record maximum phase
        uint64_t max_phase = 0u;
        // *** Record number of splitters and total number of DFFs (not only path balancing DFFs)
        uint64_t total_num_spl = 0;
        network.foreach_gate([&](const klut::signal & node)
        {
          NodeData node_data { network.value(node) };
          fmt::print("[Node {}] old max_phase = {}\tnode_data = {}\t", node, max_phase, static_cast<int>(node_data.sigma));
          max_phase = generic_max(max_phase, node_data.sigma);
          fmt::print("new max_phase = {}\n", max_phase);

          auto fo_size = network.fanout_size(node);
          if (fo_size > 1)
          {
            total_num_spl += fo_size - 1;
          }
        });

        network.foreach_po([&](const klut::signal & node)
        {
          NodeData node_data { network.value(node) };
          fmt::print("[PO {}] max_phase = {}, sigma = {}, node #DFF = {}\n", node, max_phase, static_cast<int>(node_data.sigma), ( (max_phase - node_data.sigma) / n_phases ) );
          total_num_dff += (max_phase - node_data.sigma) / n_phases;
          fmt::print("\t\t\t\t[i] total #DFF = {}\n", total_num_dff);
        });

        fmt::print("{} PHASES: #DFF   for {} is {}\n", n_phases, benchmark, total_num_dff);
        int total_area = raw_area + total_num_dff * COSTS_MAP[fDFF] + total_num_spl * COSTS_MAP[fSPL];
        fmt::print("{} PHASES: #AREA  for {} is {}\n", n_phases, benchmark, total_area);
        fmt::print("{} PHASES: #MAX GLOB PHASE for {} is {}\n", n_phases, benchmark, max_phase);

        exp(fmt::format("{}_{}", benchmark, (i==0)?"CPSAT":"GREEDY"), n_phases, total_num_dff, total_area, ( (max_phase - 1) / n_phases + 1 ), num_t1_cells);
        exp.save();
        exp.table();
      }
    }
  }
  // TODO : now, count #DFFs in extracted paths. Perhaps, the function can be written within the Path object.
  return 0;
}

#if false
  int main() 
  {
    /*
    // *** READ COMPOUND GATE LIBRARY ***
    const std::vector<std::vector<UI>> sets_of_levels { { {0,0,0,0}, {0,0,0,1}, {0,0,0,2}, {0,0,1,1}, {0,0,1,2}, {0,1,1,1}, {0,1,1,2}, {0,1,2,2}, {0,1,2,3} } }; //  {0,1,1,3},

    auto start_time_csv = std::chrono::high_resolution_clock::now();

    phmap::flat_hash_map<ULL, Node> GNM_global = read_global_gnm( sets_of_levels, NODEMAP_PREFIX );

    auto end_time_csv = std::chrono::high_resolution_clock::now();

    // Calculate the duration in milliseconds
    auto ms_csv = std::chrono::duration_cast<std::chrono::milliseconds>(end_time_csv - start_time_csv);
    fmt::print("CSV Runtime: {}ms\n", ms_csv.count());

    const auto cost_it = std::max_element(GNM_global.begin(), GNM_global.end(), [&](const auto & pair1, const auto & pair2) { return pair1.second.cost < pair2.second.cost; });
    const auto & [hash_cost, node_cost] = *cost_it;
    fmt::print("The most expensive node: {} {}\n{}\n", hash_cost, node_cost.to_str(), node_cost.to_stack(GNM_global));

    const auto depth_it = std::max_element(GNM_global.begin(), GNM_global.end(), [&](const auto & pair1, const auto & pair2) 
    {
      return pair1.second.depth < pair2.second.depth; 
    });
    const auto & [hash_depth, node_depth] = *depth_it;
    fmt::print("The deepest node: {} {}\n{}\n", hash_depth, node_depth.to_str(), node_depth.to_stack(GNM_global));
    */
    const std::string DATABASE_PREFIX { "../GNM/GNM_global" };

    // // Dump the map to a binary file
    // SaveToFile(GNM_global, DATABASE_PREFIX);
    // fmt::print("Saved to {}\n", DATABASE_PREFIX);
    auto start_time_dat = std::chrono::high_resolution_clock::now();

    // Load the map from the binary file
    phmap::flat_hash_map<uint64_t, Node> loadedMap;
    bool status = LoadFromFile(loadedMap, DATABASE_PREFIX);
    if (!status)
    {
      fmt::print("READING FAILED");
      return 1;
    };

    auto end_time_dat = std::chrono::high_resolution_clock::now();

    auto ms_dat = std::chrono::duration_cast<std::chrono::milliseconds>(end_time_dat - start_time_dat);

    fmt::print("DAT Runtime: {}ms\n", ms_dat.count());
    /*

    for (const auto& [hash, old_node] : GNM_global) 
    {
      if (loadedMap.find(hash) == loadedMap.end())
      {
        fmt::print("Entry at {} is not found\n", hash);
        continue;
      };
      const Node & new_node = loadedMap.at(hash);
      if (old_node != new_node)
      {
        fmt::print("Entries at {} are not equivalent\n", hash);
        fmt::print("\tOld: {}\n", old_node.to_str());
        fmt::print("\tNew: {}\n", new_node.to_str());
        continue;
      };
    }

    */
   
    auto start_time_par = std::chrono::high_resolution_clock::now();

    // Load the map from the binary file
    phmap::flat_hash_map<uint64_t, Node> loadedMapPar = ParallelLoadFromFiles(DATABASE_PREFIX);

    auto end_time_par = std::chrono::high_resolution_clock::now();

    auto ms_par = std::chrono::duration_cast<std::chrono::milliseconds>(end_time_par - start_time_par);

    fmt::print("PAR Runtime: {}ms\n", ms_par.count());

    for (const auto& [hash, old_node] : loadedMap) 
    {
      if (loadedMapPar.find(hash) == loadedMapPar.end())
      {
        fmt::print("Entry at {} is not found\n", hash);
        continue;
      };
      const Node & new_node = loadedMapPar.at(hash);
      if (old_node != new_node)
      {
        fmt::print("Entries at {} are not equivalent\n", hash);
        fmt::print("\tOld: {}\n", old_node.to_str());
        fmt::print("\tNew: {}\n", new_node.to_str());
        continue;
      };
    }

    

    return 0;
  }

#endif