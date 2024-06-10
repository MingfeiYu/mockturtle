#include <cstdint>
#include <optional>
#include <vector>

#include <bill/sat/interface/common.hpp>

#include <kitty/bit_operations.hpp>
#include <kitty/constructors.hpp>
#include <kitty/spectral.hpp>

#include "../networks/xag.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/cnf_view.hpp"

namespace mockturtle
{

struct exact_mc_mo_synthesis_params
{
	uint32_t conflict_limit{ 0u };
	std::optional<std::string> write_dimacs{};
	bool verbose{ false };
};

struct exact_mc_mo_synthesis_stats
{
	stopwatch<>::duration time_total{};

	void report() const
	{
		fmt::print( "[i] total time    = {:>5.2f} secs\n", to_seconds( time_total ) );
	}
};

namespace detail
{

template<class Ntk, bill::solvers Solver>
struct exact_mc_mo_synthesis_impl
{
	using problem_network_t = cnf_view<xag_network, false, Solver>;

public:
	exact_mc_mo_synthesis_impl( std::vector<kitty::dynamic_truth_table> const& funcs, uint32_t num_vars, exact_mc_mo_synthesis_params const& ps, exact_mc_mo_synthesis_stats& st )
		: num_vars_( num_vars ),
		  num_funcs_( funcs.size() ),
		  ps_( ps ),
		  st_( st )
	{
		funcs_.reserve( num_funcs_ );
		invert_.reserve( num_funcs_ );
		num_ands_min_ = num_ands_max_ = 0u;
		for ( kitty::dynamic_truth_table const func : funcs )
		{
			funcs_.emplace_back( kitty::get_bit( func, 0 ) ? ~func : func );
			invert_.emplace_back( kitty::get_bit( func, 0 ) );
			uint32_t mc{ kitty::get_mc( func ) };
			num_ands_min_ = std::max( num_ands_min_, mc );
			num_ands_max_ += mc;
		}
		num_ands_ = num_ands_min_;
	}

	std::optional<Ntk> run()
	{
		stopwatch<> t( st_.time_total );
		for ( ; num_ands_ < num_ands_max_; ++num_ands_ )
		{
			// fmt::print( "[m] try with {} AND gates...\n", num_ands_ );

			cnf_view_params cvps;
			cvps.write_dimacs = ps_.write_dimacs;
			problem_network_t pntk( cvps );
			reset( pntk );

			for ( uint32_t i{ 0u }; i < num_ands_; ++i )
			{
				add_gate( pntk );
			}

			for ( uint32_t i{ 0u }; i < num_funcs_; ++i )
			{
				add_output( pntk );
			}

			if ( const auto sol = solve_direct( pntk ); sol )
			{
				// fmt::print( "[m] managed to synthesize using {} AND gates!\n", num_ands_ );
				if ( ps_.verbose )
				{
					debug_solution( pntk );
				}
				return *sol;
			}
		}

		// fmt::print( "[m] failed to synthesize using no more than {} AND gates!\n", ( num_ands_max_ - 1 ) );
		return std::nullopt;
	}

private:
	std::optional<Ntk> solve_direct( problem_network_t& pntk )
	{
		prune_search_space( pntk );

		for ( uint32_t b = 1u; b < funcs_[0].num_bits(); ++b )
		{
			constrain_assignment( pntk, b );
		}

		if ( const auto sat = solve( pntk ); sat && *sat )
		{
			return extract_network( pntk );
		}
		else
		{
			return std::nullopt;
		}
	}

	std::optional<bool> solve( problem_network_t& pntk )
	{
		bill::result::clause_type assumptions;
		pntk.foreach_po( [&]( auto const& f ) {
			assumptions.emplace_back( pntk.lit( f ) );
		} );
		const auto sat = pntk.solve( assumptions, ps_.conflict_limit );

		return sat;
	}

	Ntk extract_network( problem_network_t& pntk )
	{
		Ntk res;
    std::vector<signal<Ntk>> nodes( num_vars_ );
    std::generate( nodes.begin(), nodes.end(), [&]() { return res.create_pi(); } );

    const auto extract_ltfi = [&]( std::vector<signal<problem_network_t>> const& ltfi_vars ) -> signal<Ntk> {
      std::vector<signal<Ntk>> ltfi;
      for ( auto j = 0u; j < ltfi_vars.size(); ++j )
      {
        if ( pntk.model_value( ltfi_vars[j] ) )
        {
          ltfi.emplace_back( nodes[j] );
        }
      }
      return res.create_nary_xor( ltfi );
    };

    for ( uint32_t i{ 0u }; i < num_ands_; ++i )
    {
    	nodes.emplace_back( res.create_and( extract_ltfi( ltfi_vars_[2 * i] ), extract_ltfi( ltfi_vars_[2 * i + 1] ) ) );
    }

    for ( uint32_t i{ 0u }; i < num_funcs_; ++i )
    {
    	const auto s = extract_ltfi( ltfi_vars_[2 * num_ands_ + i] );
    	res.create_po( invert_[i] ? res.create_not( s ) : s );
    }

    return res;
	}


	void constrain_assignment( problem_network_t& pntk, uint32_t b )
	{
		const auto create_xor_clause = [&]( std::vector<signal<problem_network_t>> const& ltfi_vars ) -> signal<problem_network_t> {
      std::vector<signal<problem_network_t>> ltfi( ltfi_vars.size() );
      for ( uint32_t j = 0u; j < ltfi.size(); ++j )
      {
        ltfi[j] = pntk.create_and( ltfi_vars[j], truth_vars_[b][j] );
      }
      return pntk.create_nary_xor( ltfi );
    };

    for ( uint32_t i{ 0u }; i < num_ands_; ++i )
    {
    	truth_vars_[b].emplace_back( pntk.create_and( create_xor_clause( ltfi_vars_[2 * i] ), create_xor_clause( ltfi_vars_[2 * i + 1] ) ) );
    }

    for ( uint32_t i{ 0u }; i < num_funcs_; ++i )
    {
    	const auto po_signal = create_xor_clause( ltfi_vars_[2 * num_ands_ + i] );
    	pntk.create_po( kitty::get_bit( funcs_[i], b ) ? po_signal : pntk.create_not( po_signal ) );
    }
	}

	void reset( problem_network_t const& pntk )
  {
  	/* initialization */
    ltfi_vars_.clear();
    truth_vars_.clear();
    truth_vars_.resize( funcs_[0].num_bits() );

    /* pre-assign truth_vars_ with primary inputs */
    for ( auto i = 0u; i < num_vars_; ++i )
    {
      const auto var_tt = kitty::nth_var<kitty::dynamic_truth_table>( num_vars_, i );
      for ( auto b = 0u; b < funcs_[0].num_bits(); ++b )
      {
      	truth_vars_[b].emplace_back( pntk.get_constant( kitty::get_bit( var_tt, b ) ) );
      }
    }
  }

  void add_gate( problem_network_t& pntk )
  {
    uint32_t gate_index = static_cast<uint32_t>( ltfi_vars_.size() ) / 2;

    // add select variables
    for ( auto j = 0u; j < 2u; ++j )
    {
      ltfi_vars_.emplace_back( std::vector<signal<problem_network_t>>( num_vars_ + gate_index ) );
      std::generate( ltfi_vars_.back().begin(), ltfi_vars_.back().end(), [&]() { return pntk.create_pi(); } );
    }
  }

  void add_output( problem_network_t& pntk )
  {
    ltfi_vars_.emplace_back( std::vector<signal<problem_network_t>>( num_vars_ + num_ands_ ) );
    std::generate( ltfi_vars_.back().begin(), ltfi_vars_.back().end(), [&]() { return pntk.create_pi(); } );
  }

  uint32_t count_xors( problem_network_t& pntk ) const
  {
    uint32_t ctr{ 0u };
    for ( auto const& ltfi : ltfi_vars_ )
    {
      for ( auto const& l : ltfi )
      {
        ctr += pntk.model_value( l ) ? 1u : 0u;
      }
    }
    return ctr;
  }

  void prune_search_space( problem_network_t& pntk )
  {
  	// At least one element in LTFI
    for ( auto const& ltfi : ltfi_vars_ )
    {
      pntk.add_clause( ltfi );
    }

    // ensure to use essential variables and gates
    for ( uint32_t i{ 0u }; i < num_vars_ + num_ands_; ++i )
    {
    	if ( i < num_vars_ )
    	{
    		bool trivial_var{ true };
    		for ( uint32_t j{ 0u }; j < num_funcs_; ++j )
    		{
    			if ( trivial_var && kitty::has_var( funcs_[j], i ) )
    			{
    				trivial_var = false;
    			}
    		}
    		if ( trivial_var )
    		{
    			continue;
    		}
    	}

    	std::vector<signal<problem_network_t>> clause;
    	for ( auto const& ltfi : ltfi_vars_ )
    	{
    		if ( i < ltfi.size() )
    		{
    			clause.emplace_back( ltfi[i] );
    		}
    	}
    	pntk.add_clause( clause );
    }
  }

  void debug_solution( problem_network_t& pntk ) const
  {
    const auto print_ltfi = [&]( std::vector<signal<problem_network_t>> const& ltfi ) {
      for ( auto const& f : ltfi )
      {
        fmt::print( "{} ", static_cast<uint32_t>( pntk.model_value( f ) ) );
      }
      if ( uint32_t padding = 2u * ( num_vars_ + num_ands_ - ltfi.size() ); padding > 0 )
      {
        fmt::print( "{}", std::string( padding, ' ' ) );
      }
    };

    for ( uint32_t i{ 0u }; i < num_ands_; ++i )
    {
      fmt::print( "{:>2} = ", ( i + 1 ) );
      print_ltfi( ltfi_vars_[2 * i] );
      fmt::print( "   " );
      print_ltfi( ltfi_vars_[2 * i + 1] );
      fmt::print( "\n" );
    }

    for ( uint32_t i{ 0u }; i < num_funcs_; ++i )
    {
    	fmt::print( " f{} = ", i );
    	print_ltfi( ltfi_vars_[2 * num_ands_ + i] );
    }

    fmt::print( "\n  XORs = {}\n\n", count_xors( pntk ) );
  }

private:
	uint32_t num_vars_;
	uint32_t num_funcs_;
	std::vector<kitty::dynamic_truth_table> funcs_;
	std::vector<bool> invert_;
	uint32_t num_ands_min_;
	uint32_t num_ands_max_;
	uint32_t num_ands_;

	std::vector<std::vector<signal<problem_network_t>>> ltfi_vars_;
	std::vector<std::vector<signal<problem_network_t>>> truth_vars_;

	exact_mc_mo_synthesis_params const& ps_;
	exact_mc_mo_synthesis_stats& st_;
};

} /* namespace detail */

template<class Ntk = xag_network, bill::solvers Solver = bill::solvers::bsat2>
std::optional<Ntk> exact_mc_mo_synthesis( std::vector<kitty::dynamic_truth_table> const& funcs, exact_mc_mo_synthesis_params const& ps = {}, exact_mc_mo_synthesis_stats* pst = nullptr )
{
	uint32_t num_vars{ funcs[0].num_vars() };
	for ( kitty::dynamic_truth_table const& func : funcs )
	{
		if ( func.num_vars() != num_vars )
		{
			/* TODO: Align input size */
			fmt::print( "[e] Inconsistent input size detected!\n" );
			abort();
		} 
	}

	exact_mc_mo_synthesis_stats st;
	const auto res = detail::exact_mc_mo_synthesis_impl<Ntk, Solver>{ funcs, num_vars, ps, st }.run();

	if ( ps.verbose )
	{
		st.report();
	}
	if ( pst )
	{
		*pst = st;
	}

	if ( res )
	{
		return *res;
	}
	else
	{
		return std::nullopt;
	}
}

} /* namespace mockturtle */