
/* Do not modify this file it was automatically generated by the
 * TokenList2DsnLexer CMake script.
 *
 * Include this file in your lexer class to provide the keywords for
 * your DSN lexer.
 */

#include <cmp_library_lexer.h>

using namespace TLIB_T;

#define TOKDEF(x)    { #x, T_##x }

const KEYWORD CMP_LIBRARY_LEXER::keywords[] = {
    TOKDEF( arc ),
    TOKDEF( author ),
    TOKDEF( center ),
    TOKDEF( circle ),
    TOKDEF( comment ),
    TOKDEF( component ),
    TOKDEF( copyright ),
    TOKDEF( docs ),
    TOKDEF( drawing ),
    TOKDEF( electical_type ),
    TOKDEF( end ),
    TOKDEF( field ),
    TOKDEF( fill_style ),
    TOKDEF( header ),
    TOKDEF( height ),
    TOKDEF( length ),
    TOKDEF( license ),
    TOKDEF( name ),
    TOKDEF( number ),
    TOKDEF( orientation ),
    TOKDEF( pin ),
    TOKDEF( polyline ),
    TOKDEF( position ),
    TOKDEF( radius ),
    TOKDEF( rectangle ),
    TOKDEF( start ),
    TOKDEF( style ),
    TOKDEF( symbol ),
    TOKDEF( tags ),
    TOKDEF( text ),
    TOKDEF( url ),
    TOKDEF( version ),
    TOKDEF( width )
};

const unsigned CMP_LIBRARY_LEXER::keyword_count = unsigned( sizeof( CMP_LIBRARY_LEXER::keywords )/sizeof( CMP_LIBRARY_LEXER::keywords[0] ) );


const char* CMP_LIBRARY_LEXER::TokenName( T aTok )
{
    const char* ret;

    if( aTok < 0 )
        ret = DSNLEXER::Syntax( aTok );
    else if( (unsigned) aTok < keyword_count )
        ret = keywords[aTok].name;
    else
        ret = "token too big";

    return ret;
}
