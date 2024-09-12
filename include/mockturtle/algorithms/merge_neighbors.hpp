#include <fmt/format.h>

#include "collapse_mapped.hpp"
#include "cut_enumeration.hpp"
#include "../networks/klut.hpp"
#include "../utils/node_map.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/mapping_view.hpp"
#include "../views/topo_view.hpp"

#include <kitty/properties.hpp>

namespace mockturtle
{

struct merge_neighbor_params
{
	merge_neighbor_params()
	{
		cut_enum_ps.cut_size = 3u;
		cut_enum_ps.cut_limit = 16u;
		cut_enum_ps.minimize_truth_table = true;
	}

	cut_enumeration_params cut_enum_ps{};
	bool verbose{ false };
};

struct merge_neighbor_stats
{
	stopwatch<>::duration runtime{ 0u };

	void report() const
	{
		std::cout << fmt::format( "[i] Merging runtime           = {:>5.2f} secs\n", to_seconds( runtime ) );
	}
};

namespace detail
{

struct cut_enumeration_neighbor_cut
{
	uint32_t cost{ 0u };
};

template<class Ntk>
class merge_neighbor_impl
{
public:
  // static constexpr uint32_t max_cut_num = 32;
  // static constexpr uint32_t max_cut_size = 16;
  // using cut_t = cut<max_cut_size, cut_data<true, cut_enum_neighbor_cut>>;
  // using cut_set_t = lut_cut_set<cut_t, max_cut_num>;
  using node = typename Ntk::node;
  // using cut_merge_t = typename std::array<cut_set_t*, Ntk::max_fanin_size + 1>;
  // using tt_cache = truth_table_cache<TT>;
  // using cost_cache = std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>;
  // using cubes_queue_t = std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<uint32_t>>;
  // using lut_info = std::pair<kitty::dynamic_truth_table, std::vector<signal<klut_network>>>;

public:
	explicit merge_neighbor_impl( Ntk& ntk, merge_neighbor_params const& ps, merge_neighbor_stats& st )
		: ntk( ntk ),
		  ps( ps ),
		  st( st ),
		  cuts( cut_enumeration<Ntk, true, cut_enumeration_neighbor_cut>( ntk, ps.cut_enum_ps ) )
	{}

	klut_network run()
	{
		stopwatch t( st.runtime );

		topo_order.reserve( ntk.size() );
		topo_view<Ntk>( ntk ).foreach_node( [this]( auto const& n ) {
			topo_order.emplace_back( n );
		} );

		cover_network();

		return create_lut_network();
	}

	void cover_network()
	{
		best_cut_ind.clear();
		ntk.clear_values();
		ntk.foreach_co( [this]( auto const& f ) {
			ntk.incr_value( ntk.get_node( f ) );
		} );

		for ( auto it{ topo_order.rbegin() }; it != topo_order.rend(); ++it )
		{
			if ( ntk.is_constant( *it ) || ntk.is_ci( *it ) || ( !ntk.value( *it ) ) )
			{
				continue;
			}

			const uint32_t index = ntk.node_to_index( *it );
			/* determine the best cut of Node indexed 'index' */
			// uint8_t cut_index{ 0u };


			// for ( auto const& cut_p : cuts.cuts( index ) )
			// {
			// 	std::cout << "[m] the " << ( cut_index + 1 ) << "th cut of Node " << index << " is with " << cut_p->size() << " leaves\n";
			// 	++cut_index;
			// }


			// cut_index = 0u;
			// for ( auto const& cut_p : cuts.cuts( index ) )
			for ( auto i{ cuts.cuts( index ).size() - 2 }; i != 0u; --i )
			{
				/* TODO: check if the cuts are in size-decreasing order */
				// const auto tt = cuts.truth_table( *cut_p );
				const auto tt = cuts.truth_table( ( cuts.cuts( index ) )[i] );


				// std::cout << "[m] the " << ( cut_index + 1 ) << "th cut of Node " << index << " is with " << tt.num_vars() << " leaves\n";
				// std::cout << "[m] the " << ( i + 1 ) << "th cut of Node " << index << " is with " << tt.num_vars() << " leaves\n";


				/* set the first-encountered legal cut as the best cut */
				if ( tt.num_vars() == 3u )
				{
					if ( kitty::is_symmetric( tt ) || kitty::is_top_xor_decomposible( tt ) )
					{
						// best_cut_ind.insert( { index, cut_index } );
						best_cut_ind.insert( { index, i } );
						break;
					}

					if ( const auto sym_check = kitty::is_symmetric_n( tt ); std::get<0>( sym_check ) )
					{
						// best_cut_ind.insert( { index, cut_index } );
						best_cut_ind.insert( { index, i } );
						break;
					}
				}
				else
				{
					// best_cut_ind.insert( { index, cut_index } );
					best_cut_ind.insert( { index, i } );
					break;
				}
				
				// ++cut_index;
			}

			if ( best_cut_ind.find( index ) == best_cut_ind.end() )
			{
				best_cut_ind.insert( { index, 0u } );
			}

			assert( best_cut_ind.find( index ) != best_cut_ind.end() );
			auto const& best_cut = ( cuts.cuts( index ) )[best_cut_ind[index]];
			for ( uint32_t const& leaf : best_cut )
			{
				ntk.incr_value( ntk.index_to_node( leaf ) );
			}
		}
	}

	klut_network create_lut_network()
	{
		klut_network res;
		mapping_view<Ntk, true> mapping_ntk{ ntk };

		for ( auto const& n : topo_order )
		{
			if ( ntk.is_ci( n ) || ntk.is_constant( n ) || ( !ntk.value( n ) ) )
			{
				continue;
			}

			const uint32_t index = ntk.node_to_index( n );
			auto const& best_cut = ( cuts.cuts( index ) )[best_cut_ind[index]];

			std::vector<node> leaves;
			for ( uint32_t const& leaf : best_cut )
			{
				leaves.emplace_back( ntk.index_to_node( leaf ) );
			}
			mapping_ntk.add_to_mapping( n, leaves.begin(), leaves.end() );
			mapping_ntk.set_cell_function( n, cuts.truth_table( best_cut ) );
		}

		collapse_mapped_network( res, mapping_ntk );
		ntk.clear_values();

		return res;
	}

private:
	Ntk& ntk;
	merge_neighbor_params const& ps;
	merge_neighbor_stats& st;
	network_cuts<Ntk, true, cut_enumeration_neighbor_cut> cuts;
	std::vector<node> topo_order;
	std::unordered_map<uint32_t, uint8_t> best_cut_ind;
};


} /* namespace detail */

template<class Ntk>
klut_network merge_neighbors( Ntk& ntk, merge_neighbor_params ps = {}, merge_neighbor_stats* pst = nullptr )
{
	static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
  static_assert( has_is_ci_v<Ntk>, "Ntk does not implement the is_ci method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_foreach_ci_v<Ntk>, "Ntk does not implement the foreach_ci method" );
  static_assert( has_foreach_co_v<Ntk>, "Ntk does not implement the foreach_co method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

  merge_neighbor_params ps_{ ps };
  merge_neighbor_stats st_;
  klut_network res;

  detail::merge_neighbor_impl<Ntk> impl( ntk, ps_, st_ );
  res = impl.run();

  if ( ps.verbose )
  {
  	st_.report();
  }
  if ( pst )
  {
  	*pst = st_;
  }

  return res;
}

} /* namespace mockturtle */