#include <fstream>
#include <fmt/format.h>

#include <kitty/constructors.hpp>
#include <kitty/print.hpp>

std::string genlib_n_ary_sym( uint8_t num_vars )
{
    std::string res{};
    /* output negation is considered */
    uint32_t num_func = ( 1 << num_vars );
    for ( uint32_t i{ 1u }; i < num_func; ++i )
    {
        uint32_t spec = ( i << 1 );
        kitty::dynamic_truth_table tt = kitty::create<kitty::dynamic_truth_table>( num_vars );
        for ( uint64_t j{ 0u }; j < tt.num_bits(); ++j )
        {
            if ( ( spec >> __builtin_popcount( static_cast<uint32_t>( j ) ) ) & 1 )
            {
                set_bit( tt, j );
            }
        }

        /* print gate */
        std::string func = kitty::to_sop_expression( tt );
        std::string gate = fmt::format( "GATE SYM_{}_{} {:0.2f} Y={};\n", num_vars, spec, 1.f, func );
        // TODO: associate gate cost to #zero/one segmentations
        for ( uint8_t j{ 0u }; j < num_vars; ++j )
        {
            /* PHASE; INPUT_LOAD; MAX_LOAD; RISE_BLOCK_DELAY; RISE_FANOUT_DELAY; FALL_BLOCK_DELAY; FALL_FANOUT_DELAY */
            std::string pin = fmt::format( "\tPIN {} UNKNOWN 1 999 1.00 0.00 1.00 0.00\n", static_cast<char>( 'A' + j ) );
            gate += pin;
        }
        res += gate;
    }
    return res;
}

std::string genlib_upto_n_ary_sym( uint8_t num_vars )
{
    std::string res{};
    res += "GATE CONST_0 0.00 Y=CONST0;\n";
    res += "GATE CONST_1 0.00 Y=CONST1;\n";
    for ( uint8_t i{ 2u }; i <= num_vars; ++i )
    {
        res += genlib_n_ary_sym( i );
    }
    return res;
}

int main()
{
    uint8_t num_vars_upper{ 3u };
    std::ofstream fo;
    std::string filename = "sym_" + std::to_string( num_vars_upper ) + ".genlib";
    fo.open( filename );
    fo << genlib_upto_n_ary_sym( num_vars_upper );
    fo.close();

    return 0;
}