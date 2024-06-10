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

struct incr_mc_mo_synthesis_params
{
	uint32_t conflict_limit{ 0u };
	std::optional<std::string> write_dimacs{};
	bool verbose{ false };
};

struct incr_mc_mo_synthesis_stats
{
	stopwatch<>::duration time_total{};
	std::vector<float> time_each{};
	std::vector<uint32_t> num_ands{};

	void report() const
	{
		fmt::print( "[i] total time    = {:>5.2f} secs\n", to_seconds( time_total ) );
	}
};

namespace detail
{

template<class Ntk, bill::solvers Solver, bool Slim>
struct incr_mc_mo_synthesis_impl
{
	using problem_network_t = cnf_view<xag_network, false, Solver>;

public:
	incr_mc_mo_synthesis_impl( std::vector<kitty::dynamic_truth_table> const& funcs, uint32_t num_vars, uint32_t upper, incr_mc_mo_synthesis_params const& ps, incr_mc_mo_synthesis_stats& st )
		: num_vars_( num_vars ),
		  num_funcs_( funcs.size() ),
		  num_bits_( funcs[0].num_bits() ),
		  ps_( ps ),
		  st_( st ),
		  upper_( upper )
	{
		assert( num_funcs_ >= 2u );
		funcs_.reserve( num_funcs_ );
		invert_.reserve( num_funcs_ );
		mc_.reserve( num_funcs_ );
		for ( kitty::dynamic_truth_table const& func : funcs )
		{
			funcs_.emplace_back( kitty::get_bit( func, 0 ) ? ~func : func );
			invert_.emplace_back( kitty::get_bit( func, 0 ) );
			mc_.emplace_back( kitty::get_mc( func ) );
		}
		num_ands_min_ = std::max( mc_[0], mc_[1] );
		num_ands_max_ = mc_[0] + mc_[1];
		num_ands_ = num_ands_min_;
		num_ands_accum_ = 0u;
		func_cnt = 0u;
		ltfi_value_pre_.clear();

		st_.time_each.clear();
		st_.num_ands.clear();
	}

	std::optional<Ntk> exec()
	{
		if constexpr ( Slim )
		{
			if ( upper_ != 0u )
			{
				return limited_run_slim();
			}
			else
			{
				return run_slim();
			}
		}
		else
		{
			return run();
		}
	}

	std::optional<Ntk> run()
	{
		stopwatch<>	t( st_.time_total );

		bool success{ false };
		cnf_view_params cvps;
		cvps.write_dimacs = ps_.write_dimacs;

		{
			stopwatch<>::duration time_each{};
			stopwatch<>	t_each( time_each );
			for ( ; num_ands_ <= num_ands_max_; ++num_ands_ )
			{
				// std::cout << "[m] try with " << ( num_ands_accum_ + num_ands_ ) << " AND gates...\n";
				problem_network_t pntk( cvps );
				reset( pntk );

				for ( uint32_t i{ 0u }; i < ( num_ands_accum_ + num_ands_ ); ++i )
				{
					add_gate( pntk );
				}
				for ( uint32_t i{ 0u }; i < 2u; ++i )
				{
					add_output( pntk );
				}

				prune_search_space( pntk, true );
				constrain_assignment( pntk, true );

				if ( const auto sat = solve( pntk ); sat && *sat )
				{
					// std::cout << "[m] managed to synthesize using " << ( num_ands_accum_ + num_ands_ ) << " AND gates!\n";
					st_.time_each.emplace_back( to_seconds( time_each ) );
					st_.num_ands.emplace_back( num_ands_ );

					if ( ps_.verbose )
					{
						debug_solution( pntk, true );
					}

					if ( num_funcs_ == 2u )
					{
						return extract_network( pntk, true );
					}
					else
					{
						dump( pntk, true );
						// std::cout << "[m] current size of 'ltfi_value_pre_' is " << ltfi_value_pre_.size() << "\n";
						num_ands_accum_ += num_ands_;
						success = true;
						break;
					}
				}
			}

			st_.time_each.emplace_back( to_seconds( time_each ) );
			if ( !success )
			{
				// std::cout << "[m] failed to synthesize using " << ( num_ands_accum_ + num_ands_ ) << " AND gates!\n";
				return std::nullopt;
			}
			
		}

		func_cnt += 2u;

		while ( func_cnt < num_funcs_ )
		{
			success = false;
			/* update 'num_ands_min_', 'num_ands_', and 'num_ands_max_' */
			num_ands_max_ = mc_[func_cnt];
			if ( num_ands_max_ <= num_ands_accum_ )
			{
				num_ands_min_ = 0u;
			}
			else
			{
				num_ands_min_ = num_ands_max_ - num_ands_accum_;
			}
			num_ands_ = num_ands_min_;

			stopwatch<>::duration time_each{};
			stopwatch<>	t_each( time_each );
			for ( ; num_ands_ <= num_ands_max_; ++num_ands_ )
			{
				// std::cout << "[m] try with " << ( num_ands_accum_ + num_ands_ ) << " AND gates...\n";
				problem_network_t pntk( cvps );
				reset( pntk );

				for ( uint32_t i{ 0u }; i < ( num_ands_accum_ + num_ands_ ); ++i )
				{
					add_gate( pntk );
				}
				add_output( pntk );

				prune_search_space( pntk, false );
				constrain_assignment( pntk, false );
				restore( pntk );

				if ( const auto sat = solve( pntk ); sat && *sat )
				{
					// std::cout << "[m] managed to synthesize using " << ( num_ands_accum_ + num_ands_ ) << " AND gates!\n";
					st_.time_each.emplace_back( to_seconds( time_each ) );
					st_.num_ands.emplace_back( num_ands_ );

					if ( ps_.verbose )
					{
						debug_solution( pntk, false );
					}

					if ( func_cnt == ( num_funcs_ - 1u ) )
					{
						return extract_network( pntk, false );
					}
					else
					{
						dump( pntk, false );
						// std::cout << "[m] current size of 'ltfi_value_pre_' is " << ltfi_value_pre_.size() << "\n";
						num_ands_accum_ += num_ands_;
						success = true;
						break;
					}
				}
			}

			if ( !success )
			{
				// std::cout << "[m] failed to synthesize using " << ( num_ands_accum_ + num_ands_ ) << " AND gates!\n";
				st_.time_each.emplace_back( to_seconds( time_each ) );
				return std::nullopt;
			}
			
			++func_cnt;
		}

		return std::nullopt;
	}

	std::optional<Ntk> run_slim()
	{
		// po_supports_.clear();
		// po_supports_.resize( num_funcs_ );
		stopwatch<>	t( st_.time_total );

		bool success{ false };
		cnf_view_params cvps;
		cvps.write_dimacs = ps_.write_dimacs;
		bool first = true;
		num_ands_min_ = num_ands_ = num_ands_max_ = mc_[0];

		while ( true )
		{
			success = false;

			stopwatch<>::duration time_each{};
			stopwatch<>	t_each( time_each );
			for ( ; num_ands_ <= num_ands_max_; ++num_ands_ )
			{
				// std::cout << "[m] try with " << ( num_ands_accum_ + num_ands_ ) << " AND gates...\n";
				problem_network_t pntk( cvps );
				reset( pntk );

				for ( uint32_t i{ 0u }; i < ( num_ands_accum_ + num_ands_ ); ++i )
				{
					add_gate( pntk );
				}
				add_output( pntk );

				constrain_assignment( pntk, false );
				if ( !first )
				{
					prune_search_space( pntk, false );
					restore( pntk );
				}
				else
				{
					prune_search_space( pntk, true );
					first = false;
				}
			
				if ( const auto sat = solve( pntk ); sat && *sat )
				{
					// std::cout << "[m] managed to synthesize using " << ( num_ands_accum_ + num_ands_ ) << " AND gates!\n";
					st_.time_each.emplace_back( to_seconds( time_each ) );
					st_.num_ands.emplace_back( num_ands_ );

					if ( ps_.verbose )
					{
						debug_solution( pntk, false );
					}

					if ( func_cnt == ( num_funcs_ - 1u ) )
					{
						return extract_network( pntk, false );
					}
					else
					{
						dump( pntk, false );
						// std::cout << "[m] current size of 'ltfi_value_pre_' is " << ltfi_value_pre_.size() << "\n";
						num_ands_accum_ += num_ands_;
						success = true;
						break;
					}
				}
			}

			if ( !success )
			{
				// std::cout << "[m] failed to synthesize using " << ( num_ands_accum_ + num_ands_ ) << " AND gates!\n";
				st_.time_each.emplace_back( to_seconds( time_each ) );
				return std::nullopt;
			}
			
			/* update 'num_ands_min_', 'num_ands_', and 'num_ands_max_' */
			if ( ++func_cnt < num_funcs_ )
			{
				num_ands_max_ = mc_[func_cnt];
				if ( num_ands_max_ <= num_ands_accum_ )
				{
					num_ands_min_ = 0u;
				}
				else
				{
					num_ands_min_ = num_ands_max_ - num_ands_accum_;
				}
				num_ands_ = num_ands_min_;
			}
		}
	}

	std::optional<Ntk> limited_run_slim()
	{
		stopwatch<>	t( st_.time_total );

		bool success{ false };
		cnf_view_params cvps;
		cvps.write_dimacs = ps_.write_dimacs;
		bool first = true;
		num_ands_min_ = num_ands_ = num_ands_max_ = mc_[0];

		while ( true )
		{
			success = false;

			stopwatch<>::duration time_each{};
			stopwatch<>	t_each( time_each );
			for ( ; num_ands_ <= num_ands_max_; ++num_ands_ )
			{
				// std::cout << "[m] try with " << ( num_ands_accum_ + num_ands_ ) << " AND gates...\n";
				problem_network_t pntk( cvps );
				reset( pntk );

				for ( uint32_t i{ 0u }; i < ( num_ands_accum_ + num_ands_ ); ++i )
				{
					add_gate( pntk );
				}
				add_output( pntk );

				constrain_assignment( pntk, false );
				if ( !first )
				{
					prune_search_space( pntk, false );
					restore( pntk );
				}
				else
				{
					prune_search_space( pntk, true );
					first = false;
				}
			
				if ( const auto sat = solve( pntk ); sat && *sat )
				{
					// std::cout << "[m] managed to synthesize using " << ( num_ands_accum_ + num_ands_ ) << " AND gates!\n";
					st_.time_each.emplace_back( to_seconds( time_each ) );
					st_.num_ands.emplace_back( num_ands_ );
					std::cout << "[m] po size is now " << st_.num_ands.size() << "\n";

					if ( ps_.verbose )
					{
						debug_solution( pntk, false );
					}

					if ( func_cnt == ( num_funcs_ - 1u ) || ( ( num_ands_accum_ + num_ands_ ) >= upper_ ) )
					{
						return extract_network( pntk, false );
					}
					else
					{
						dump( pntk, false );
						// std::cout << "[m] current size of 'ltfi_value_pre_' is " << ltfi_value_pre_.size() << "\n";
						num_ands_accum_ += num_ands_;
						success = true;
						break;
					}
				}
			}

			if ( !success )
			{
				// std::cout << "[m] failed to synthesize using " << ( num_ands_accum_ + num_ands_ ) << " AND gates!\n";
				st_.time_each.emplace_back( to_seconds( time_each ) );
				return std::nullopt;
			}
			
			/* update 'num_ands_min_', 'num_ands_', and 'num_ands_max_' */
			if ( ++func_cnt < num_funcs_ )
			{
				num_ands_max_ = mc_[func_cnt];
				if ( num_ands_max_ <= num_ands_accum_ )
				{
					num_ands_min_ = 0u;
				}
				else
				{
					num_ands_min_ = num_ands_max_ - num_ands_accum_;
				}
				num_ands_ = num_ands_min_;
			}
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
    ltfi_vars_.emplace_back( std::vector<signal<problem_network_t>>( num_vars_ + num_ands_accum_ + num_ands_ ) );
    std::generate( ltfi_vars_.back().begin(), ltfi_vars_.back().end(), [&]() { return pntk.create_pi(); } );
  }

  void prune_search_space( problem_network_t& pntk, bool first = true )
  {
  	// At least one element in LTFI
    for ( auto i{ ltfi_value_pre_.size() }; i < ltfi_vars_.size(); ++i )
    {
      pntk.add_clause( ltfi_vars_[i] );
    }

    // ensure to use essential gates
    for ( auto i{ num_vars_ + num_ands_accum_ }; i < ( num_vars_ + num_ands_accum_ + num_ands_ ); ++i )
    {
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

    if ( first )
    {
    	// ensure to use essential variables
    	for ( auto i{ 0u }; i < num_vars_; ++i )
    	{
    		// skip trivial variable
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

    		std::vector<signal<problem_network_t>> clause;
    		for ( auto const& ltfi : ltfi_vars_ )
    		{
    			clause.emplace_back( ltfi[i] );
    		}
    		pntk.add_clause( clause );
    	}
    }
  }

  void constrain_assignment( problem_network_t& pntk, bool first = true )
	{
		uint32_t num_pos{ 0u };
		num_pos = first ? 2u : 1u;

		for ( auto b{ 0u }; b < num_bits_; ++b )
		{
			const auto create_xor_clause = [&]( std::vector<signal<problem_network_t>> const& ltfi_vars ) -> signal<problem_network_t> {
				std::vector<signal<problem_network_t>> ltfi( ltfi_vars.size() );
				for ( auto j{ 0u }; j < ltfi.size(); ++j )
				{
					ltfi[j] = pntk.create_and( ltfi_vars[j], truth_vars_[b][j] );
				}
				return pntk.create_nary_xor( ltfi );
			};

			for ( auto i{ 0u }; i < ( num_ands_accum_ + num_ands_ ); ++i )
			{
				truth_vars_[b].emplace_back( pntk.create_and( create_xor_clause( ltfi_vars_[2 * i] ), create_xor_clause( ltfi_vars_[2 * i + 1] ) ) );
			}

			for ( auto i{ 0u }; i < num_pos; ++i )
			{
				const auto po_signal = create_xor_clause( ltfi_vars_[2 * (num_ands_accum_ + num_ands_ ) + i] );
				pntk.create_po( kitty::get_bit( funcs_[func_cnt + i], b ) ? po_signal : pntk.create_not( po_signal ) );
			}
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

	void debug_solution( problem_network_t const& pntk, bool first = true ) const
  {
    const auto print_ltfi = [&]( std::vector<signal<problem_network_t>> const& ltfi ) {
      for ( auto const& f : ltfi )
      {
        fmt::print( "{} ", static_cast<uint32_t>( pntk.model_value( f ) ) );
      }
      if ( uint32_t padding = 2u * ( num_vars_ + num_ands_accum_ + num_ands_ - ltfi.size() ); padding > 0 )
      {
        fmt::print( "{}", std::string( padding, ' ' ) );
      }
    };

    for ( uint32_t i{ 0u }; i < ( num_ands_accum_ + num_ands_ ); ++i )
    {
      fmt::print( "{:>2} = ", ( i + 1 ) );
      print_ltfi( ltfi_vars_[2 * i] );
      fmt::print( "   " );
      print_ltfi( ltfi_vars_[2 * i + 1] );
      fmt::print( "\n" );
    }

    uint32_t num_pos{ 0u };
    num_pos = first ? 2u : 1u;
    for ( uint32_t i{ 0u }; i < num_pos; ++i )
    {
    	fmt::print( " f{} = ", i );
    	print_ltfi( ltfi_vars_[2 * ( num_ands_accum_ + num_ands_ ) + i] );
    }

    fmt::print( "\n  XORs = {}\n\n", count_xors( pntk ) );
  }

  uint32_t count_xors( problem_network_t const& pntk ) const
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

  Ntk extract_network( problem_network_t const& pntk, bool first = true )
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

    for ( uint32_t i{ 0u }; i < ( num_ands_accum_ + num_ands_ ); ++i )
    {
    	nodes.emplace_back( res.create_and( extract_ltfi( ltfi_vars_[2 * i] ), extract_ltfi( ltfi_vars_[2 * i + 1] ) ) );
    }

    uint32_t num_pos{ 0u };
    num_pos = first ? 2u : 1u;
    for ( uint32_t i{ 0u }; i < num_pos; ++i )
    {
    	const auto s = extract_ltfi( ltfi_vars_[2 * ( num_ands_accum_ + num_ands_ ) + i] );
    	res.create_po( invert_[func_cnt + i] ? res.create_not( s ) : s );
    }

    return res;
	}

  void dump( problem_network_t const& pntk, bool first = true )
  {
  	uint32_t ltfi_vars_size{ ltfi_vars_.size() };
  	if ( first )
  	{
  		ltfi_vars_size -= 2u;
  	}
  	else
  	{
  		--ltfi_vars_size;
  	}

  	uint32_t ltfi_value_pre_size{ ltfi_value_pre_.size() };
  	ltfi_value_pre_.resize( ltfi_vars_size );
  	for ( auto i{ ltfi_value_pre_size }; i < ltfi_vars_size; ++i )
  	{
  		std::vector<signal<problem_network_t>> const& ltfi = ltfi_vars_[i];
  		std::vector<bool>& ltfi_value_each = ltfi_value_pre_[i];
  		ltfi_value_each.reserve( ltfi.size() );
  		for ( auto const& ltfi_vars_each : ltfi )
  		{
  			ltfi_value_each.emplace_back( pntk.model_value( ltfi_vars_each ) );
  		}
  	}
  }

  void restore( problem_network_t& pntk )
  {
  	for ( auto i{ 0u }; i < ltfi_value_pre_.size(); ++i )
  	{
  		std::vector<bool>& ltfi_value = ltfi_value_pre_[i];
  		std::vector<signal<problem_network_t>>& ltfi = ltfi_vars_[i];
  		for ( auto j{ 0u }; j < ltfi_value.size(); ++j )
  		{
  			pntk.add_clause( pntk.create_not( ltfi[j] ^ ltfi_value[j] ) );
  		}
  	}
  }

  // void record_po_support( problem_network_t const& pntk )
  // {}

private:
	uint32_t num_vars_;
	uint32_t num_funcs_;
	uint32_t func_cnt{ 0u };
	uint32_t num_bits_;
	std::vector<kitty::dynamic_truth_table> funcs_;
	std::vector<bool> invert_;
	std::vector<uint32_t> mc_;
	uint32_t num_ands_min_;
	uint32_t num_ands_max_;
	uint32_t num_ands_;
	uint32_t num_ands_accum_;
	uint32_t upper_;

	std::vector<std::vector<signal<problem_network_t>>> ltfi_vars_;
	std::vector<std::vector<signal<problem_network_t>>> truth_vars_;
	std::vector<std::vector<bool>> ltfi_value_pre_;
	// std::vector<std::vector<uint32_t>> po_supports_;

	incr_mc_mo_synthesis_params const& ps_;
	incr_mc_mo_synthesis_stats& st_;
};

} /* namespace detail */

template<class Ntk = xag_network, bill::solvers Solver = bill::solvers::bsat2, bool Slim = true>
std::optional<Ntk> incr_mc_mo_synthesis( std::vector<kitty::dynamic_truth_table> const& funcs, uint32_t upper = 0u, incr_mc_mo_synthesis_params const& ps = {}, incr_mc_mo_synthesis_stats* pst = nullptr )
{
	uint32_t num_vars{ funcs[0].num_vars() };
	for ( kitty::dynamic_truth_table const& func : funcs )
	{
		if ( func.num_vars() != num_vars )
		{
			fmt::print( "[e] Inconsistent input size detected!\n" );
			abort();
		} 
	}

	incr_mc_mo_synthesis_stats st;

	const auto res = detail::incr_mc_mo_synthesis_impl<Ntk, Solver, Slim>{ funcs, num_vars, upper, ps, st }.exec();

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