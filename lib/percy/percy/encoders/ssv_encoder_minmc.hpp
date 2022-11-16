#pragma once

#include "encoder.hpp"
#include "../misc.hpp"

#include <iostream>

namespace percy
{

class ssv_encoder_minmc: public encoder
{
public:
	ssv_encoder_minmc( solver_wrapper& solver )
	{
		vLits = pabc::Vec_IntAlloc( 128 );
		set_solver( solver );
	}

	~ssv_encoder_minmc()
	{
		pabc::Vec_IntFree( vLits );
	}

	uint32_t get_op_var( uint32_t step_idx, uint32_t var_idx ) const
	{
		assert( step_idx < nr_steps );
		assert( var_idx > 0 && var_idx <= nr_op_vars_per_step );

		return op_offset + nr_op_vars_per_step * step_idx + var_idx - 1;
	}

	uint32_t get_sel_var( uint32_t var_idx ) const
	{
		assert( var_idx < nr_sel_vars );

		return sel_offset + var_idx;
	}

	uint32_t get_sim_var( uint32_t step_idx, uint32_t bit_idx ) const
	{
		assert( step_idx < nr_steps );
		assert( bit_idx < tt_size ); // 'tt_size' is already with the LSB excluded

		return sim_offset + tt_size * step_idx + bit_idx;
	}

	uint32_t get_isnfree_var( uint32_t step_idx ) const
	{
		assert( step_idx < nr_steps );

		return isnfree_offset + step_idx;
	}

	uint32_t get_minmc_var( uint32_t stage_idx, uint32_t bit_idx ) const
	{
		assert( stage_idx < nr_steps - 1 );
		assert( bit_idx < nr_nfree );

		return minmc_offset + stage_idx * nr_nfree + bit_idx;
	}

	/*! \brief Calculate the number of variables required.
 	*/
	void calculate_variables( spec_minmc const& spec )
	{
 		pis = spec.nr_in;
 		fanin_size = spec.fanin_size;
		nr_steps = spec.nr_steps;
		verbose = spec.verbosity;
		nr_nfree = spec.nfree;
		tt_size = spec.tt_size;

		nr_op_vars_per_step = ( 1u << fanin_size ) - 1;
		nr_op_vars = nr_steps * nr_op_vars_per_step;
		nr_sim_vars = nr_steps * tt_size;
		nr_sel_vars = 0u;
		nr_isnfree_vars = nr_steps;
		nr_minmc_vars = ( nr_steps - 1 ) * nr_nfree;

		svar_map.clear();
		nr_svar_map.resize( nr_steps );

		std::vector<uint32_t> fanins_cand( fanin_size );

		if ( verbose )
		{
			std::cout << "There are " << nr_steps << " steps to synthesize, " << nr_nfree << " are non-XOR gates.\n";
		}

		for ( auto i = pis; i < pis + nr_steps; ++i )
		{
			if ( verbose > 2 )
			{
				std::cout << "[i] adding sel vars for step-" << i + 1 << std::endl;
			}

			/* the number of potential combination of fanins of each step */
			auto nr_svars_each_step = 0u;
			fanin_init( fanins_cand, fanin_size - 1 );
			do
			{
				if ( verbose > 4 )
				{
					print_fanin( fanins_cand );
				}

				svar_map.emplace_back( fanins_cand );
				++nr_svars_each_step;
			} while ( fanin_inc( fanins_cand, i - 1 ) );

			if ( verbose > 2 )
			{
				std::cout << "[i] added " << nr_svars_each_step << " sel vars" << std::endl;
			}

			nr_sel_vars += nr_svars_each_step;
			nr_svar_map[i - pis] = nr_svars_each_step;
			assert( nr_svars_each_step == binomial_coeff( i, fanin_size ) );
		}

		sel_offset = 0u;
		op_offset = sel_offset + nr_sel_vars;
		sim_offset = op_offset + nr_op_vars;
		isnfree_offset = sim_offset + nr_sim_vars;
		minmc_offset = isnfree_offset + nr_isnfree_vars;
		nr_vars_total = minmc_offset + nr_minmc_vars;

		if ( verbose > 1 )
		{
			std::cout << "Creating variables (SSV-" << fanin_size << "): " << std::endl;
			std::cout << "nr_sel_vars = " << nr_sel_vars << std::endl;
			std::cout << "nr_op_vars = " << nr_op_vars << std::endl;
			std::cout << "nr_sim_vars = " << nr_sim_vars << std::endl;
			std::cout << "nr_isnfree_vars = " << nr_isnfree_vars << std::endl;
			std::cout << "nr_minmc_vars = " << nr_minmc_vars << std::endl;
			std::cout << "IN TOTAL: " << nr_vars_total << " variables" << std::endl;
		}

		solver->set_nr_vars( nr_vars_total );
	}

	/*! \brief Constraints ensure that every step has specified number of inputs.
 	*
 	* This function generate clauses that guarantee: for each step to be 
 	* synthesized, one and only one of its potential combination of fanins would 
 	* be enabled, i.e., for a certain step i, clause ( s_i_0 \/ s_i_1 \/ ... ) is 
 	* generated.
 	*
 	*/
 	bool create_op_clauses()
 	{
 		bool status = true;

 		if ( verbose > 2 )
 		{
 			std::cout << "Creating op clauses (SSV-" << fanin_size << "): " << std::endl;
 		}

 		auto svar_offset = 0u;
 		for ( auto i = 0; i < nr_steps; ++i )
 		{
 			const auto nr_svars_for_i = nr_svar_map[i];

 			for ( auto j = 0; j < nr_svars_for_i; ++j )
 			{
 				pabc::Vec_IntSetEntry( vLits, j, 
 															 pabc::Abc_Var2Lit( get_sel_var( svar_offset + j ), 0 ) );
 			}

 			status &= solver->add_clause( pabc::Vec_IntArray( vLits ), 
 																		pabc::Vec_IntArray( vLits ) + nr_svars_for_i );

 			if ( verbose > 3 )
 			{
 				std::cout << "creating op clauses: (";
 				for ( auto j = 0; j < nr_svars_for_i; ++j )
 				{
 					std::cout << ( j > 0 ? " \\/ " : " " );
 					std::cout << "s_" << pis + i + 1 << "_" << svar_offset + j + 1;
 				}
 				std::cout << " ) ( STATUS = " << status << " )" << std::endl;
 			}

 			svar_offset += nr_svars_for_i;
 		}

 		if ( verbose > 2 )
 		{
 			std::cout << "#clauses = " << solver->nr_clauses() << " ( POST )" << std::endl;
 		}

 		return status;
 	}

 	bool create_cardinal_clauses()
 	{
 		bool status = true;
 		int pLits[2];

 		if ( verbose > 2 )
 		{
 			std::cout << "Creating cardinal clauses (SSV-" << fanin_size << "): " << std::endl;
 		}

 		auto svar_offset = 0u;
 		for ( auto i = 0u; i < nr_steps; ++i )
 		{
 			const auto nr_svars_for_i = nr_svar_map[i];

 			for ( auto j1 = 0; j1 < nr_svars_for_i - 1; ++j1 )
 			{
 				for ( auto j2 = j1 + 1; j2 < nr_svars_for_i; ++j2 )
 				{
 					pLits[0] = pabc::Abc_Var2Lit( get_sel_var( svar_offset + j1 ), 1 );
 					pLits[1] = pabc::Abc_Var2Lit( get_sel_var( svar_offset + j2 ), 1 );
 					status &= solver->add_clause( pLits, pLits + 2 );

 					if ( verbose > 3 )
		 			{
		 				std::cout << "creating cardinal clauses: ( ";
		 				std::cout << "!s_" << pis + i + 1 << "_" << svar_offset + j1 + 1;
		 				std::cout << " \\/ ";
		 				std::cout << "!s_" << pis + i + 1 << "_" << svar_offset + j2 + 1;
		 				std::cout << " ) ( STATUS = " << status << " )" << std::endl;
		 			}
 				}
 			}

 			svar_offset += nr_svars_for_i;
 		}

 		if ( verbose > 2 )
 		{
 			std::cout << "#clauses = " << solver->nr_clauses() << " ( POST )" << std::endl;
 		}

 		return status;
 	}

 	bool create_sim_clauses( uint32_t const bit_idx, 
 													 uint32_t const step_idx, uint32_t const svar_idx, 
 													 bool const func_bit, uint32_t const opvar_idx, 
 													 std::vector<uint32_t> const& fanins, 
 													 std::vector<bool> const& fanin_asgn )
 	{
 		auto status = true;
 		uint32_t lit_offset = 0u;

 		for ( int i = 0; i < fanin_size; ++i )
 		{
 			auto const child = fanins[i];
 			auto const assigned_bit = fanin_asgn[i];

 			if ( child < pis )
 			{
 				if ( ( ( ( bit_idx + 1 ) & ( 1 << child ) ) ? 1 : 0 ) != assigned_bit )
 				{
 					return true;
 				}
 			}
 			else
 			{
 				pabc::Vec_IntSetEntry( vLits, lit_offset++, 
 															 pabc::Abc_Var2Lit( get_sim_var( ( child - pis ), bit_idx ), 
 																 									assigned_bit ) );
 			}
 		}
 		pabc::Vec_IntSetEntry( vLits, lit_offset++, 
 													 pabc::Abc_Var2Lit( get_sim_var( step_idx, bit_idx ), func_bit ) );
 		pabc::Vec_IntSetEntry( vLits, lit_offset++, 
 													 pabc::Abc_Var2Lit( get_sel_var( svar_idx ), 1 ) );

 		if ( opvar_idx == 0u )
 		{
 			assert( func_bit );

 			status &= solver->add_clause( pabc::Vec_IntArray( vLits ), 
 																		pabc::Vec_IntArray( vLits ) + lit_offset );

 			if ( verbose > 3 )
 			{
 				std::cout << "creating tt clauses: ( ";
	 			std::cout << "!s_" << pis + step_idx + 1 << "_" << svar_idx + 1 << " \\/ ";
	 			std::cout << ( func_bit ? "!" : "" ) << "x_" << pis + step_idx + 1 << "_" << bit_idx + 2;

	 			for ( auto i = 0; i < fanin_size; ++i )
	 			{
	 				auto const child = fanins[i];
	 				auto const assigned_bit = fanin_asgn[i];

	 				if (child < pis)
	 				{
	 					continue;
	 				}

	 				std::cout << " \\/ ";
	 				std::cout << ( assigned_bit ? "!" : "" ) << "x_" << child + 1 << "_" << bit_idx + 2;

	 			}
	 			std::cout << " ) ( STATUS = " << status << " )" << std::endl;
 			}

 			return status;
 		}

 		pabc::Vec_IntSetEntry( vLits, lit_offset++, 
 														 pabc::Abc_Var2Lit( get_op_var( step_idx, opvar_idx ), 1 - func_bit ) );

 		status &= solver->add_clause( pabc::Vec_IntArray( vLits ), 
 																	pabc::Vec_IntArray( vLits ) + lit_offset );

 		if ( verbose > 3 )
 		{
 			std::cout << "creating tt clauses: ( ";
 			std::cout << "!s_" << pis + step_idx + 1 << "_" << svar_idx + 1 << " \\/ ";
 			std::cout << ( func_bit ? "!" : "" ) << "x_" << pis + step_idx + 1 << "_" << bit_idx + 2;

 			for ( auto i = 0; i < fanin_size; ++i )
 			{
 				auto const child = fanins[i];
 				auto const assigned_bit = fanin_asgn[i];

 				if (child < pis)
 				{
 					continue;
 				}

 				std::cout << " \\/ ";
 				std::cout << ( assigned_bit ? "!" : "" ) << "x_" << child + 1 << "_" << bit_idx + 2;
 			}

 			assert( opvar_idx > 0 );
 			std::cout << " \\/ ";
 			std::cout << ( func_bit ? "" : "!" ) << "f_" << pis + step_idx + 1 << "_" << opvar_idx + 1;

 			std::cout << " ) ( STATUS = " << status << " )" << std::endl;
 		}

 		return status;
 	}

 	bool create_tt_clauses_each_bit( uint32_t const bit_idx )
 	{
 		bool status = true;

 		std::vector<bool> fanin_asgn( fanin_size );
 		uint32_t svar_offset = 0u;

 		for ( auto i = 0; i < nr_steps; ++i )
 		{
 			const auto nr_svars_for_i = nr_svar_map[i];

 			for ( auto j = 0; j < nr_svars_for_i; ++j )
 			{
 				const auto svar_idx = j + svar_offset;
 				const auto& fanins = svar_map[svar_idx];

 				uint32_t opvar_idx = 0u;
 				bool func_bit = false;
 				clear_assignment( fanin_asgn );

 				while ( true )
 				{
 					next_assignment( fanin_asgn );
 					if ( is_zero( fanin_asgn ) )
 					{
 						break;
 					}

 					++opvar_idx;
 					status &= create_sim_clauses( bit_idx, i, svar_idx, func_bit, 
 																				opvar_idx, fanins, fanin_asgn );
 				}

 				opvar_idx = 0u;
 				func_bit = true;
 				clear_assignment( fanin_asgn );

 				status &= create_sim_clauses( bit_idx, i, svar_idx, func_bit, 
 																			opvar_idx, fanins, fanin_asgn );

 				while ( true )
 				{
 					next_assignment( fanin_asgn );
 					if ( is_zero( fanin_asgn ) )
 					{
 						break;
 					}

 					++opvar_idx;
 					status &= create_sim_clauses( bit_idx, i, svar_idx, func_bit, 
 																				opvar_idx, fanins, fanin_asgn );
 				}
 			}

 			svar_offset += nr_svars_for_i;
 		}

 		return status;
 	}

 	bool create_tt_clauses()
 	{
 		bool status = true;

 		if ( verbose > 2 )
 		{
 			std::cout << "Creating tt clauses (SSV-" << fanin_size << "): " << std::endl;
 		}

 		for ( auto bit_idx = 0; bit_idx < tt_size; ++bit_idx )
 		{
 			status &= create_tt_clauses_each_bit( bit_idx );
 		}

 		if ( verbose > 2 )
 		{
 			std::cout << "#clauses = " << solver->nr_clauses() << " ( POST )" << std::endl;
 		}

 		return status;
 	}

 	/* clauses to guarantee that output of the last step meets the targeted funciton */
 	bool create_out_clauses( spec_minmc const& spec )
 	{
 		bool status = true;

 		if ( verbose > 2 )
 		{
 			std::cout << "Creating out clauses (SSV-" << fanin_size << "): " << std::endl;
 		}

 		int pLit[2];
 		for ( auto bit_idx = 1u; bit_idx <= tt_size; ++bit_idx )
 		{
 			/*
 			if ( spec.is_dont_care( bit_idx ) )
 			{

 				std::cout << "The dc mask is: ";
        print_binary( spec.get_dc_mask() );
        std::cout << std::endl;



 				std::cout << "The " << bit_idx + 1 << "th bit(" << kitty::get_bit( spec.get_function(), bit_idx ) << ") is dont care\n";
 				continue;
 			}
 			*/

 			bool func_bit = kitty::get_bit( spec.get_function(), bit_idx );

 			pLit[0] = pabc::Abc_Var2Lit( get_sim_var( ( nr_steps - 1), bit_idx - 1 ), !func_bit );
 			status &= solver->add_clause( pLit, pLit + 1 );

 			if ( verbose > 3 )
 			{
 				std::cout << "creating out clauses: " << std::endl;
 				std::cout << "( " << (func_bit ? "" : "!") << "x_" << pis + nr_steps << "_" << bit_idx + 1 << " )";
 				std::cout << " ( STATUS = " << status << " )" << std::endl;
 			}
 		}

 		if ( verbose > 2 )
 		{
 			std::cout << "#clauses = " << solver->nr_clauses() << " ( POST )" << std::endl;
 		}

 		return status;
 	}

 	bool create_primitive_clauses( spec_minmc const& spec )
 	{
 		bool status = true;

 		const auto primitives = spec.get_compiled_primitives();
 		const auto primitives_free = spec.get_compiled_free_primitives();

 		if ( verbose > 2 )
 		{
 			std::cout << "Creating primitive clauses (SSV-" << fanin_size << "): " << std::endl;
 		}

 		kitty::dynamic_truth_table tt( fanin_size );
 		kitty::clear( tt );
 		do {
 			if ( !kitty::is_normal( tt ) )
 			{
 				kitty::next_inplace( tt );
 				continue;
 			}

 			bool is_primitive_operator = false;
 			bool is_free_primitive_operator = false;

 			for ( auto const primitive_free: primitives_free )
 			{
 				if ( tt == primitive_free )
 				{
 					is_primitive_operator = true;
 					is_free_primitive_operator = true;
 					break;
 				}
 			}

 			if ( !is_primitive_operator )
 			{
 				for ( auto const primitive: primitives )
 				{
 					if ( tt == primitive )
 					{
 						is_primitive_operator = true;
 						break;
 					}
 				}
 			}

			for ( auto i = 0u; i < nr_steps; ++i )
			{
				for ( auto j = 0u; j < nr_op_vars_per_step; ++j )
				{
					pabc::Vec_IntSetEntry( vLits, j, 
																 pabc::Abc_Var2Lit( get_op_var( i, j + 1 ), 
																 										kitty::get_bit( tt, j + 1 ) ) );
				}

				if ( !is_primitive_operator )
				{
					status &= solver->add_clause( pabc::Vec_IntArray( vLits ), 
																				pabc::Vec_IntArray( vLits ) + nr_op_vars_per_step );
					
					if ( verbose > 3 )
					{
						std::cout << "creating primitive clauses: (";
						for ( uint32_t j = 0u; j < nr_op_vars_per_step; ++j )
						{
							std::cout << ( ( j > 0 ) ? " \\/ " : " " );
							std::cout << ( kitty::get_bit( tt, j + 1 ) ? "!" : "" );
							std::cout << "f_" << pis + i + 1 << "_" << j + 2;
						}
						std::cout << " ) ( STATUS = " << status << " )" << std::endl;
					}
				}
				else
				{
					pabc::Vec_IntSetEntry( vLits, nr_op_vars_per_step, 
																 pabc::Abc_Var2Lit( get_isnfree_var( i ), 
																 										( is_free_primitive_operator ? 1 : 0 ) ) );
					status &= solver->add_clause( pabc::Vec_IntArray( vLits ), 
																				pabc::Vec_IntArray( vLits ) + nr_op_vars_per_step + 1 );

					if ( verbose > 3 )
					{
						std::cout << "creating primitive clauses: (";
						for ( auto j = 0; j < nr_op_vars_per_step; ++j )
						{
							std::cout << ( ( j > 0 ) ? " \\/ " : " " );
							std::cout << ( kitty::get_bit( tt, j + 1 ) ? "!" : "" );
							std::cout << "f_" << pis + i + 1 << "_" << j + 2;
						}
						std::cout << " \\/ " << ( is_free_primitive_operator ? "!" : "" );
						std::cout << "o_" << pis + i + 1;
						std::cout << " ) ( STATUS = " << status << " )" << std::endl;
					}
				}

			}

			kitty::next_inplace( tt );
 		} while ( !kitty::is_const0( tt ) );

 		if ( verbose > 2 )
 		{
 			std::cout << "#clauses = " << solver->nr_clauses() << " ( POST )" << std::endl;
 		}

 		return status;
 	}

 	bool create_minmc_clauses()
 	{
 		bool status = true;

 		if ( verbose > 2 )
 		{
 			std::cout << "Creating minmc clauses (SSV-" << fanin_size << "): " << std::endl;
 		}

 		int pLits[3];

 		if ( nr_nfree == 0u )
 		{
 			for ( auto i = 0u; i < nr_steps; ++i )
 			{
 				pLits[0] = pabc::Abc_Var2Lit( get_isnfree_var( i ), 1 );
 				status &= solver->add_clause( pLits, pLits + 1 );

 				if ( verbose > 3 )
 				{
 					std::cout << "creating minmc clauses: ";
 					std::cout << "( !o_" << pis + i + 1;
 					std::cout << " ) ( STATUS = " << status << " )" << std::endl;
 				}
 			}
 		}
 		else if ( nr_steps == nr_nfree )
 		{
 			for ( int i = 0u; i < nr_steps; ++i )
 			{
 				pLits[0] = pabc::Abc_Var2Lit( get_isnfree_var( i ), 0 );
	 			status &= solver->add_clause( pLits, pLits + 1 );

	 			if ( verbose > 3 )
	 			{
	 				std::cout << "creating minmc clauses: ";
	 				std::cout << "( o_" << pis + i + 1;
	 				std::cout << " ) ( STATUS = " << status << " )" << std::endl;
	 			}
 			}
 		}
 		else 
 		{
	 		for ( auto i = 0u; i < nr_steps; ++i )
	 		{
	 			if ( i == 0 )
	 			{
	 				pLits[0] = pabc::Abc_Var2Lit( get_isnfree_var( 0u ), 1 );
	 				pLits[1] = pabc::Abc_Var2Lit( get_minmc_var( 0u, 0u ), 0 );
	 				status &= solver->add_clause( pLits, pLits + 2 );

	 				if ( verbose > 3 )
	 				{
	 					std::cout << "creating minmc clauses: ";
	 					std::cout << "( !o_" << pis + 1 << " \\/ ";
	 					std::cout << "m_" << pis + 1 << "_1";
	 					std::cout << " ) ( STATUS = " << status << " )" << std::endl;
	 				}

	 				for ( auto j = 1; j < nr_nfree; ++j )
	 				{
	 					pLits[0] = pabc::Abc_Var2Lit( get_minmc_var( 0u, j ), 1 );
	 					status &= solver->add_clause( pLits, pLits + 1 );

	 					if ( verbose > 3 )
		 				{
		 					std::cout << "creating minmc clauses: ";
		 					std::cout << "( !m_" << pis + 1 << "_" << j + 1;
		 					std::cout << " ) ( STATUS = " << status << " )" << std::endl;
		 				}
	 				}	
	 			}
	 			else if ( i < nr_steps - 1 )
	 			{
	 				pLits[0] = pabc::Abc_Var2Lit( get_isnfree_var( i ), 1 );
	 				pLits[1] = pabc::Abc_Var2Lit( get_minmc_var( i, 0u ), 0 );
	 				status &= solver->add_clause( pLits, pLits + 2 );

	 				pLits[0] = pabc::Abc_Var2Lit( get_minmc_var( i - 1, 0u ), 1 );
	 				pLits[1] = pabc::Abc_Var2Lit( get_minmc_var( i, 0u ), 0 );
	 				status &= solver->add_clause( pLits, pLits + 2 );

	 				pLits[0] = pabc::Abc_Var2Lit( get_isnfree_var( i ), 1 );
	 				pLits[1] = pabc::Abc_Var2Lit( get_minmc_var( i - 1, nr_nfree - 1 ), 1 );
	 				status &= solver->add_clause( pLits, pLits + 2 );

	 				if ( verbose > 3 )
	 				{
	 					std::cout << "creating minmc clauses: ";
	 					std::cout << "( !o_" << pis + i + 1 << " \\/ ";
	 					std::cout << "m_" << pis + i + 1 << "_1";
	 					std::cout << " ) ( STATUS = " << status << " )" << std::endl;

	 					std::cout << "creating minmc clauses: ";
	 					std::cout << "( !m_" << pis + i << "_1" << " \\/ ";
	 					std::cout << "m_" << pis + i + 1 << "_1";
	 					std::cout << " ) ( STATUS = " << status << " )" << std::endl;

	 					std::cout << "creating minmc clauses: ";
	 					std::cout << "( !o_" << pis + i + 1 << " \\/ ";
	 					std::cout << "!m_" << pis + i << "_" << nr_nfree;
	 					std::cout << " ) ( STATUS = " << status << " )" << std::endl;
	 				}

	 				for ( auto j = 1; j < nr_nfree; ++j )
	 				{
	 					pLits[0] = pabc::Abc_Var2Lit( get_isnfree_var( i ), 1 );
	 					pLits[1] = pabc::Abc_Var2Lit( get_minmc_var( i - 1, j - 1 ), 1 );
	 					pLits[2] = pabc::Abc_Var2Lit( get_minmc_var( i, j ), 0 );
	 					status &= solver->add_clause( pLits, pLits + 3 );

	 					pLits[0] = pabc::Abc_Var2Lit( get_minmc_var( i - 1, j ), 1 );
	 					pLits[1] = pabc::Abc_Var2Lit( get_minmc_var( i, j ), 0 );
	 					status &= solver->add_clause( pLits, pLits + 2 );

	 					if ( verbose > 3 )
	 					{
	 						std::cout << "creating minmc clauses: ";
	 						std::cout << "( !o_" << pis + i + 1 << " \\/ ";
	 						std::cout << "!m_" << pis + i << "_" << j << " \\/ ";
	 						std::cout << "m_" << pis + i + 1 << "_" << j + 1;
	 						std::cout << " ) ( STATUS = " << status << " )" << std::endl;

	 						std::cout << "creating minmc clauses: ";
		 					std::cout << "( !m_" << pis + i << "_" << j + 1 << " \\/ ";
		 					std::cout << "m_" << pis + i + 1 << "_" << j + 1;
		 					std::cout << " ) ( STATUS = " << status << " )" << std::endl;
	 					}
	 				}
	 			}
	 			else
	 			{
	 				pLits[0] = pabc::Abc_Var2Lit( get_isnfree_var( i ), 1 );
	 				pLits[1] = pabc::Abc_Var2Lit( get_minmc_var( i - 1, nr_nfree - 1 ), 1 );
	 				status &= solver->add_clause( pLits, pLits + 2 );

	 				if ( verbose > 3 )
	 				{
	 					std::cout << "creating minmc clauses: ";
	 					std::cout << "( !o_" << pis + i + 1 << " \\/ ";
	 					std::cout << "!m_" << pis + i << "_" << nr_nfree;
	 					std::cout << " ) ( STATUS = " << status << " )" << std::endl;
	 				}
	 			}
	 		}
	 	}

 		if ( verbose > 2 )
 		{
 			std::cout << "#clauses = " << solver->nr_clauses() << " ( POST )" << std::endl;
 		}

 		return status;
 	}

 	bool create_contribution_clauses()
 	{
 		bool status = true;

 		if ( verbose > 2 )
 		{
 			std::cout << "Creating contribution clauses (SSV-" << fanin_size << "): " << std::endl;
 		}

 		uint32_t svar_offset_ext = 0u;

 		for ( auto i = 0; i < nr_steps - 1; ++i )
 		{
 			uint32_t lit_offset = 0u;
 			svar_offset_ext += nr_svar_map[i];

 			uint32_t svar_offset = svar_offset_ext;
 			for ( auto i_suc = i + 1; i_suc < nr_steps; ++i_suc )
 			{
 				auto const nr_svar_for_i = nr_svar_map[i_suc];
 				for ( auto j = 0; j < nr_svar_for_i; ++j )
 				{
 					auto const fanins = svar_map[svar_offset + j];
 					for ( auto const fanin: fanins )
 					{
 						if ( fanin > pis + i )
 						{
 							break;
 						}
 						else if ( fanin == pis + i )
 						{
 							pabc::Vec_IntSetEntry( vLits, lit_offset++, 
 																		 pabc::Abc_Var2Lit( get_sel_var( svar_offset + j ), 0 ) );

 							if ( verbose > 3 )
 							{
 								if ( lit_offset == 1u )
 								{
 									std::cout << "creating contribution clauses: ( ";
 								}
 								else
 								{
 									std::cout << ", ";
 								}
 								std::cout << "s_" << pis + i_suc + 1 << "_" << svar_offset + j + 1;
 							}

 							break;
 						}
 					}
 				}

 				svar_offset += nr_svar_for_i;
 			}

 			status &= solver->add_clause( pabc::Vec_IntArray( vLits ), 
 																		pabc::Vec_IntArray( vLits ) + lit_offset );


 			if ( verbose > 3 )
 			{
 				std::cout << " ) ( STATUS = " << status << " )\n";
 			}
 		}

 		if ( verbose > 2 )
 		{
 			std::cout << "#clauses = " << solver->nr_clauses() << " ( POST )" << std::endl;
 		}

 		return status;
 	}

 	bool encode( spec_minmc const& spec )
 	{
 		assert( spec.nr_steps <= MAX_STEPS );

 		calculate_variables( spec );

 		if ( !create_op_clauses() )
 		{
 			return false;
 		}

 		if ( !create_cardinal_clauses() )
 		{
 			return false;
 		}

 		if ( !create_tt_clauses() )
 		{
 			return false;
 		}

 		if ( !create_out_clauses( spec ) )
 		{
 			return false;
 		}

 		if ( !create_primitive_clauses( spec ) )
 		{
 			return false;
 		}

 		if ( !create_minmc_clauses() )
 		{
 			return false;
 		}

 		/* additional clauses for faster SAT solving */
 		if ( spec.use_contribution_clauses )
 		{
 			if ( !create_contribution_clauses() )
 			{
 				std::cout << "WARNING: FAILED to create contribution clauses" << std::endl;
 			}
 		}

 		return true;
 	}

 	void extract_chain( spec_minmc const& spec, chain_minmc & chain )
 	{
 		chain.reset( pis, nr_steps, fanin_size );
 		chain.set_compiled_functions( spec.get_compiled_primitives() );

 		if ( verbose )
 		{
 			std::cout << "Report of exact synthesis: " << std::endl;
 		}

 		uint32_t svar_offset = 0u;
 		for ( auto i = 0u; i < nr_steps; ++i )
 		{
 			kitty::dynamic_truth_table op( fanin_size );

 			for ( auto j = 1u; j <= nr_op_vars_per_step; ++j )
 			{
 				if ( solver->var_value( get_op_var( i, j ) ) )
 				{
 					kitty::set_bit( op, j );
 				}
 			}

 			if ( verbose )
 			{
 				std::cout << "Step x_" << pis + i + 1 << " performs operation ";
 				kitty::print_binary( op, std::cout );
 				std::cout << ", ";
 			}

 			const auto nr_svars_for_i = nr_svar_map[i];
 			for ( auto j = 0u; j < nr_svars_for_i; ++j )
 			{
 				const auto sel_var = get_sel_var( svar_offset + j );

 				if ( solver->var_value( sel_var ) )
 				{
 					const auto fanins = svar_map[svar_offset + j];

 					if ( verbose )
 					{
 						std::cout << "with operands ";
 						for ( auto fanins_each: fanins )
 						{
 							std::cout << "x_" << fanins_each + 1 << " ";
 						}
 						std::cout << std::endl;
 					}

 					chain.set_step( i, fanins, op );
 				}
 			}

 			svar_offset += nr_svars_for_i;
 		}

 		if ( verbose )
 		{
 			kitty::dynamic_truth_table po_gen( pis );
			for ( auto j = 0u; j < tt_size; ++j )
			{
				if ( solver->var_value( get_sim_var( nr_steps - 1, j ) ) )
				{
					kitty::set_bit( po_gen, j + 1 );
				}
			}
			std::cout << "Truth table of the generated steps are: \n";
			for ( auto i = 0u; i < nr_steps; ++i )
			{
				std::cout << "Step x_" << pis + i + 1 << ": ";
				kitty::dynamic_truth_table po_gen( pis );
				for ( auto j = 0u; j < tt_size; ++j )
				{
					if ( solver->var_value( get_sim_var( i, j ) ) )
					{
						kitty::set_bit( po_gen, j + 1 );
					}
				}
				kitty::print_binary( po_gen, std::cout );
				std::cout << std::endl;
			}

			std::cout << "Truth table of the target po is: ";
			kitty::print_binary( spec.get_function(), std::cout );
			std::cout << std::endl;
 		}

 		return;
 	}

private:
	uint32_t pis;
	uint32_t fanin_size;
	uint32_t nr_steps;
	uint32_t nr_nfree;
	uint32_t tt_size;
	uint32_t verbose;

	uint32_t nr_op_vars_per_step;
	uint32_t nr_op_vars;
	uint32_t nr_sim_vars;
	uint32_t nr_sel_vars;
	uint32_t nr_isnfree_vars;
	uint32_t nr_minmc_vars;
	uint32_t nr_vars_total;

	uint32_t sel_offset;
	uint32_t op_offset;
	uint32_t sim_offset;
	uint32_t isnfree_offset;
	uint32_t minmc_offset;

	pabc::Vec_Int_t* vLits;
	std::vector<std::vector<uint32_t>> svar_map;
	std::vector<uint32_t> nr_svar_map;
}; /* ssv_encoder_minmc */

} /* namespace percy */