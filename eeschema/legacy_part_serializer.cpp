#include "legacy_part_serializer.h"

#include <ctype.h>
#include <algorithm>

#include <wx/mstream.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <pgm_base.h>
#include <draw_graphic_text.h>
#include <kiway.h>
#include <kicad_string.h>
#include <richio.h>
#include <core/typeinfo.h>
#include <properties.h>
#include <trace_helpers.h>

#include <general.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_component.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_no_connect.h>
#include <sch_text.h>
#include <sch_sheet.h>
#include <sch_legacy_plugin.h>
#include <template_fieldnames.h>
#include <sch_screen.h>
#include <class_libentry.h>
#include <class_library.h>
#include <lib_arc.h>
#include <lib_bezier.h>
#include <lib_circle.h>
#include <lib_field.h>
#include <lib_pin.h>
#include <lib_polyline.h>
#include <lib_rectangle.h>
#include <lib_text.h>
#include <eeschema_id.h>       // for MAX_UNIT_COUNT_PER_PACKAGE definition
#include <symbol_lib_table.h>  // for PropPowerSymsOnly definintion.
#include <confirm.h>


// I have tried and tried, and cannot find a definition for MANDATORY_FIELDS ANYWHERE in the entire
// source tree, nor does it magically appear after #including everything that sch_legachy_plugin.cpp includes.
// So I am friggin' defining it here, making a completely wild guess as to what its value should be.
#define MANDATORY_FIELDS 4

/**
 * \file Most of the code here is shamelessly stolen from sch_legacy_plugin_cache, 
 * because I need it to be accessible from a static context outside that class
 */

#define SCH_PARSE_ERROR( text, reader, pos )                         \
    THROW_PARSE_ERROR( text, reader.GetSource(), reader.Line(),      \
                       reader.LineNumber(), pos - reader.Line() )


// Tokens to read/save graphic lines style
#define T_STYLE "style"
#define T_COLOR "rgb"          // cannot be modifed (used by wxWidgets)
#define T_COLORA "rgba"        // cannot be modifed (used by wxWidgets)
#define T_WIDTH "width"


static bool is_eol( char c )
{
    //        The default file eol character used internally by KiCad.
    //        |
    //        |            Possible eol if someone edited the file by hand on certain platforms.
    //        |            |
    //        |            |           May have gone past eol with strtok().
    //        |            |           |
    if( c == '\n' || c == '\r' || c == 0 )
        return true;

    return false;
}


/**
 * Compare \a aString to the string starting at \a aLine and advances the character point to
 * the end of \a String and returns the new pointer position in \a aOutput if it is not NULL.
 *
 * @param aString - A pointer to the string to compare.
 * @param aLine - A pointer to string to begin the comparison.
 * @param aOutput - A pointer to a string pointer to the end of the comparison if not NULL.
 * @return true if \a aString was found starting at \a aLine.  Otherwise false.
 */
static bool strCompare( const char* aString, const char* aLine, const char** aOutput = NULL )
{
    size_t len = strlen( aString );
    bool retv = ( strncasecmp( aLine, aString, len ) == 0 ) &&
                ( isspace( aLine[ len ] ) || aLine[ len ] == 0 );

    if( retv && aOutput )
    {
        const char* tmp = aLine;

        // Move past the end of the token.
        tmp += len;

        // Move to the beginning of the next token.
        while( *tmp && isspace( *tmp ) )
            tmp++;

        *aOutput = tmp;
    }

    return retv;
}


/**
 * Parse an ASCII integer string with possible leading whitespace into
 * an integer and updates the pointer at \a aOutput if it is not NULL, just
 * like "man strtol()".
 *
 * @param aReader - The line reader used to generate exception throw information.
 * @param aLine - A pointer the current position in a string.
 * @param aOutput - The pointer to a string pointer to copy the string pointer position when
 *                  the parsing is complete.
 * @return A valid integer value.
 * @throw An #IO_ERROR on an unexpected end of line.
 * @throw A #PARSE_ERROR if the parsed token is not a valid integer.
 */
static int parseInt( LINE_READER& aReader, const char* aLine, const char** aOutput = NULL )
{
    if( !*aLine )
        SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aLine );

    // Clear errno before calling strtol() in case some other crt call set it.
    errno = 0;

    long retv = strtol( aLine, (char**) aOutput, 10 );

    // Make sure no error occurred when calling strtol().
    if( errno == ERANGE )
        SCH_PARSE_ERROR( "invalid integer value", aReader, aLine );

    // strtol does not strip off whitespace before the next token.
    if( aOutput )
    {
        const char* next = *aOutput;

        while( *next && isspace( *next ) )
            next++;

        *aOutput = next;
    }

    return (int) retv;
}


/**
 * Parse a single ASCII character and updates the pointer at \a aOutput if it is not NULL.
 *
 * @param aReader - The line reader used to generate exception throw information.
 * @param aCurrentToken - A pointer the current position in a string.
 * @param aNextToken - The pointer to a string pointer to copy the string pointer position when
 *                     the parsing is complete.
 * @return A valid ASCII character.
 * @throw IO_ERROR on an unexpected end of line.
 * @throw PARSE_ERROR if the parsed token is not a a single character token.
 */
static char parseChar( LINE_READER& aReader, const char* aCurrentToken,
                       const char** aNextToken = NULL )
{
    while( *aCurrentToken && isspace( *aCurrentToken ) )
        aCurrentToken++;

    if( !*aCurrentToken )
        SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aCurrentToken );

    if( !isspace( *( aCurrentToken + 1 ) ) )
        SCH_PARSE_ERROR( "expected single character token", aReader, aCurrentToken );

    if( aNextToken )
    {
        const char* next = aCurrentToken + 2;

        while( *next && isspace( *next ) )
            next++;

        *aNextToken = next;
    }

    return *aCurrentToken;
}


/**
 * Parse an unquoted utf8 string and updates the pointer at \a aOutput if it is not NULL.
 *
 * The parsed string must be a continuous string with no white space.
 *
 * @param aString - A reference to the parsed string.
 * @param aReader - The line reader used to generate exception throw information.
 * @param aCurrentToken - A pointer the current position in a string.
 * @param aNextToken - The pointer to a string pointer to copy the string pointer position when
 *                     the parsing is complete.
 * @param aCanBeEmpty - True if the parsed string is optional.  False if it is mandatory.
 * @throw IO_ERROR on an unexpected end of line.
 * @throw PARSE_ERROR if the \a aCanBeEmpty is false and no string was parsed.
 */
static void parseUnquotedString( wxString& aString, LINE_READER& aReader,
                                 const char* aCurrentToken, const char** aNextToken = NULL,
                                 bool aCanBeEmpty = false )
{
    if( !*aCurrentToken )
    {
        if( aCanBeEmpty )
            return;
        else
            SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aCurrentToken );
    }

    const char* tmp = aCurrentToken;

    while( *tmp && isspace( *tmp ) )
        tmp++;

    if( !*tmp )
    {
        if( aCanBeEmpty )
            return;
        else
            SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aCurrentToken );
    }

    std::string utf8;

    while( *tmp && !isspace( *tmp ) )
        utf8 += *tmp++;

    aString = FROM_UTF8( utf8.c_str() );

    if( aString.IsEmpty() && !aCanBeEmpty )
        SCH_PARSE_ERROR( _( "expected unquoted string" ), aReader, aCurrentToken );

    if( aNextToken )
    {
        const char* next = tmp;

        while( *next && isspace( *next ) )
            next++;

        *aNextToken = next;
    }
}


/**
 * Parse an quoted ASCII utf8 and updates the pointer at \a aOutput if it is not NULL.
 *
 * The parsed string must be contained within a single line.  There are no multi-line
 * quoted strings in the legacy schematic file format.
 *
 * @param aString - A reference to the parsed string.
 * @param aReader - The line reader used to generate exception throw information.
 * @param aCurrentToken - A pointer the current position in a string.
 * @param aNextToken - The pointer to a string pointer to copy the string pointer position when
 *                     the parsing is complete.
 * @param aCanBeEmpty - True if the parsed string is optional.  False if it is mandatory.
 * @throw IO_ERROR on an unexpected end of line.
 * @throw PARSE_ERROR if the \a aCanBeEmpty is false and no string was parsed.
 */
static void parseQuotedString( wxString& aString, LINE_READER& aReader,
                               const char* aCurrentToken, const char** aNextToken = NULL,
                               bool aCanBeEmpty = false )
{
    if( !*aCurrentToken )
    {
        if( aCanBeEmpty )
            return;
        else
            SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aCurrentToken );
    }

    const char* tmp = aCurrentToken;

    while( *tmp && isspace( *tmp ) )
        tmp++;

    if( !*tmp )
    {
        if( aCanBeEmpty )
            return;
        else
            SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aCurrentToken );
    }

    // Verify opening quote.
    if( *tmp != '"' )
        SCH_PARSE_ERROR( "expecting opening quote", aReader, aCurrentToken );

    tmp++;

    std::string utf8;     // utf8 without escapes and quotes.

    // Fetch everything up to closing quote.
    while( *tmp )
    {
        if( *tmp == '\\' )
        {
            tmp++;

            if( !*tmp )
                SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aCurrentToken );

            // Do not copy the escape byte if it is followed by \ or "
            if( *tmp != '"' && *tmp != '\\' )
                    utf8 += '\\';

            utf8 += *tmp;
        }
        else if( *tmp == '"' )  // Closing double quote.
        {
            break;
        }
        else
        {
            utf8 += *tmp;
        }

        tmp++;
    }

    aString = FROM_UTF8( utf8.c_str() );

    if( aString.IsEmpty() && !aCanBeEmpty )
        SCH_PARSE_ERROR( "expected quoted string", aReader, aCurrentToken );

    if( *tmp && *tmp != '"' )
        SCH_PARSE_ERROR( "no closing quote for string found", aReader, tmp );

    // Move past the closing quote.
    tmp++;

    if( aNextToken )
    {
        const char* next = tmp;

        while( *next && *next == ' ' )
            next++;

        *aNextToken = next;
    }
}

FILL_T parseFillMode( LINE_READER& aReader, const char* aLine,
                                               const char** aOutput )
{
    switch( parseChar( aReader, aLine, aOutput ) )
    {
    case 'F': return FILLED_SHAPE;
    case 'f': return FILLED_WITH_BG_BODYCOLOR;
    case 'N': return NO_FILL;
    default: SCH_PARSE_ERROR( "invalid fill type, expected f, F, or N", aReader, aLine );
    }
}

void LEGACY_PART_SERIALIZER::ReadPart( LIB_PART * aPart,  LINE_READER& aReader )
{
    const char* line = aReader.Line();

    if( strCompare( "DEF", line, &line ) ) {

        long num;
        size_t pos = 4;                               // "DEF" plus the first space.
        wxString utf8Line = wxString::FromUTF8( line );
        wxStringTokenizer tokens( utf8Line, " \r\n\t" );

        if( tokens.CountTokens() < 8 )
            SCH_PARSE_ERROR( "invalid symbol definition", aReader, line );

        LIB_PART * part = aPart; // Just rename the variable rather than every reference below...

        wxString name, prefix, tmp;

        name = tokens.GetNextToken();
        pos += name.size() + 1;

        prefix = tokens.GetNextToken();
        pos += prefix.size() + 1;

        tmp = tokens.GetNextToken();
        pos += tmp.size() + 1;                        // NumOfPins, unused.

        tmp = tokens.GetNextToken();                  // Pin name offset.

        if( !tmp.ToLong( &num ) )
            THROW_PARSE_ERROR( "invalid pin offset", aReader.GetSource(), aReader.Line(),
                            aReader.LineNumber(), pos );

        pos += tmp.size() + 1;
        part->SetPinNameOffset( (int)num );

        tmp = tokens.GetNextToken();                  // Show pin numbers.

        if( !( tmp == "Y" || tmp == "N") )
            THROW_PARSE_ERROR( "expected Y or N", aReader.GetSource(), aReader.Line(),
                            aReader.LineNumber(), pos );

        pos += tmp.size() + 1;
        part->SetShowPinNumbers( ( tmp == "N" ) ? false : true );

        tmp = tokens.GetNextToken();                  // Show pin names.

        if( !( tmp == "Y" || tmp == "N") )
            THROW_PARSE_ERROR( "expected Y or N", aReader.GetSource(), aReader.Line(),
                            aReader.LineNumber(), pos );

        pos += tmp.size() + 1;
        part->SetShowPinNames( ( tmp == "N" ) ? false : true );

        tmp = tokens.GetNextToken();                  // Number of units.

        if( !tmp.ToLong( &num ) )
            THROW_PARSE_ERROR( "invalid unit count", aReader.GetSource(), aReader.Line(),
                            aReader.LineNumber(), pos );

        pos += tmp.size() + 1;
        part->SetUnitCount( (int)num );

        // Ensure m_unitCount is >= 1.  Could be read as 0 in old libraries.
        if( part->GetUnitCount() < 1 )
            part->SetUnitCount( 1 );

        // Copy part name and prefix.

        // The root alias is added to the alias list by SetName() which is called by SetText().
        if( name.IsEmpty() )
        {
            part->SetName( "~" );
        }
        else if( name[0] != '~' )
        {
            part->SetName( name );
        }
        else
        {
            part->SetName( name.Right( name.Length() - 1 ) );
            part->GetValueField().SetVisible( false );
        }

        // Don't set the library alias, this is determined by the symbol library table.
        part->SetLibId( LIB_ID( wxEmptyString, part->GetName() ) );

        // There are some code paths in SetText() that do not set the root alias to the
        // alias list so add it here if it didn't get added by SetText().
        if( !part->HasAlias( part->GetName() ) )
            part->AddAlias( part->GetName() );

        LIB_FIELD& reference = part->GetReferenceField();

        if( prefix == "~" )
        {
            reference.Empty();
            reference.SetVisible( false );
        }
        else
        {
            reference.SetText( prefix );
        }

        // Version 2.2 had a placeholder 0, and it's find to treat that 0 as if it
        // has a real meaning.
        tmp = tokens.GetNextToken();

        if( tmp == "L" )
            part->LockUnits( true );
        else if( tmp == "F" || tmp == "0" )
            part->LockUnits( false );
        else
            THROW_PARSE_ERROR( "expected L, F, or 0", aReader.GetSource(), aReader.Line(),
                                aReader.LineNumber(), pos );

        pos += tmp.size() + 1;

        // There is the optional power component flag.
        if( tokens.HasMoreTokens() )
        {
            tmp = tokens.GetNextToken();

            if( tmp == "P" )
                part->SetPower();
            else if( tmp == "N" )
                part->SetNormal();
            else
                THROW_PARSE_ERROR( "expected P or N", aReader.GetSource(), aReader.Line(),
                                aReader.LineNumber(), pos );
        }

        line = aReader.ReadLine();

        // Read lines until "ENDDEF" is found.
        while( line )
        {
            if( *line == '#' )                               // Comment
                ;
            else if( strCompare( "Ti", line, &line ) )       // Modification date is ignored.
                continue;
            else if( strCompare( "ALIAS", line, &line ) )    // Aliases
                loadAliases( part, aReader );
            else if( *line == 'F' )                          // Fields
                loadField( part, aReader );
            else if( strCompare( "DRAW", line, &line ) )     // Drawing objects.
                loadDrawEntries( part, aReader );
            else if( strCompare( "$FPLIST", line, &line ) )  // Footprint filter list
                loadFootprintFilters( part, aReader );
            else if( strCompare( "ENDDEF", line, &line ) )   // End of part description
            {
                // Add aliases -- should be moved to caller's context
    //             for( size_t ii = 0; ii < part->GetAliasCount(); ++ii )
    //             {
    //                 LIB_ALIAS* alias = part->GetAlias( ii );
    //                 const wxString& aliasName = alias->GetName();
    //                 auto it = m_aliases.find( aliasName );
    // 
    //                 if( it != m_aliases.end() )
    //                 {
    //                     // Find a new name for the alias
    //                     wxString newName;
    //                     int idx = 0;
    //                     LIB_ALIAS_MAP::const_iterator jt;
    // 
    //                     do
    //                     {
    //                         newName = wxString::Format( "%s_%d", aliasName, idx );
    //                         jt = m_aliases.find( newName );
    //                         ++idx;
    //                     }
    //                     while( jt != m_aliases.end() );
    // 
    //                     wxLogWarning( "Symbol name conflict in library:\n%s\n"
    //                                   "'%s' has been renamed to '%s'",
    //                                   m_fileName, aliasName, newName );
    // 
    //                     if( alias->IsRoot() )
    //                         part->SetName( newName );
    //                     else
    //                         alias->SetName( newName );
    // 
    //                     m_aliases[newName] = alias;
    //                 }
    //                 else
    //                 {
    //                     m_aliases[aliasName] = alias;
    //                 }
    //             }

                return;  // Successful exit point
            }

            line = aReader.ReadLine();
        }

        SCH_PARSE_ERROR( "missing ENDDEF", aReader, line );
    } 
    else 
    {
        SCH_PARSE_ERROR( "missing DEF", aReader, line );
    }
}

void LEGACY_PART_SERIALIZER::loadAliases( LIB_PART * aPart, LINE_READER & aReader )
{
    wxString newAlias;
    const char* line = aReader.Line();

    wxCHECK_RET( strCompare( "ALIAS", line, &line ), "Invalid ALIAS section" );

    wxString utf8Line = wxString::FromUTF8( line );
    wxStringTokenizer tokens( utf8Line, " \r\n\t" );

    // Parse the ALIAS list.
    while( tokens.HasMoreTokens() )
    {
        newAlias = tokens.GetNextToken();
        aPart->AddAlias( newAlias );
    }
}

void LEGACY_PART_SERIALIZER::loadField( LIB_PART * aPart, LINE_READER & aReader )
{
   const char* line = aReader.Line();

    wxCHECK_RET( *line == 'F', "Invalid field line" );

    int         id;

    if( sscanf( line + 1, "%d", &id ) != 1 || id < 0 )
        SCH_PARSE_ERROR( "invalid field ID", aReader, line + 1 );

    LIB_FIELD* field;

    if( (unsigned) id < MANDATORY_FIELDS )
    {
        field = aPart->GetField( id );

        // this will fire only if somebody broke a constructor or editor.
        // MANDATORY_FIELDS are always present in ram resident components, no
        // exceptions, and they always have their names set, even fixed fields.
        wxASSERT( field );
    }
    else
    {
        field = new LIB_FIELD( aPart, id );
        aPart->AddDrawItem( field );
    }

    // Skip to the first double quote.
    while( *line != '"' && *line != 0 )
        line++;

    if( *line == 0 )
        SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, line );

    parseQuotedString( field->m_Text, aReader, line, &line, true );

    // Doctor the *.lib file field which has a "~" in blank fields.  New saves will
    // not save like this.
    if( field->m_Text.size() == 1 && field->m_Text[0] == '~' )
        field->m_Text.clear();

    wxPoint pos;

    pos.x = parseInt( aReader, line, &line );
    pos.y = parseInt( aReader, line, &line );
    field->SetPosition( pos );

    wxSize textSize;

    textSize.x = textSize.y = parseInt( aReader, line, &line );
    field->SetTextSize( textSize );

    char textOrient = parseChar( aReader, line, &line );

    if( textOrient == 'H' )
        field->SetTextAngle( TEXT_ANGLE_HORIZ );
    else if( textOrient == 'V' )
        field->SetTextAngle( TEXT_ANGLE_VERT );
    else
        SCH_PARSE_ERROR( "invalid field text orientation parameter", aReader, line );

    char textVisible = parseChar( aReader, line, &line );

    if( textVisible == 'V' )
        field->SetVisible( true );
    else if ( textVisible == 'I' )
        field->SetVisible( false );
    else
        SCH_PARSE_ERROR( "invalid field text visibility parameter", aReader, line );

    // It may be technically correct to use the library version to determine if the field text
    // attributes are present.  If anyone knows if that is valid and what version that would be,
    // please change this to test the library version rather than an EOL or the quoted string
    // of the field name.
    if( *line != 0 && *line != '"' )
    {
        char textHJustify = parseChar( aReader, line, &line );

        if( textHJustify == 'C' )
            field->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        else if( textHJustify == 'L' )
            field->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        else if( textHJustify == 'R' )
            field->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        else
            SCH_PARSE_ERROR( "invalid field text horizontal justification", aReader, line );

        wxString attributes;

        parseUnquotedString( attributes, aReader, line, &line );

        size_t attrSize = attributes.size();

        if( !(attrSize == 3 || attrSize == 1 ) )
            SCH_PARSE_ERROR( "invalid field text attributes size", aReader, line );

        switch( (wxChar) attributes[0] )
        {
        case 'C': field->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER ); break;
        case 'B': field->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM ); break;
        case 'T': field->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );    break;
        default:  SCH_PARSE_ERROR( "invalid field text vertical justification", aReader, line );
        }

        if( attrSize == 3 )
        {
            wxChar attr_1 = attributes[1];
            wxChar attr_2 = attributes[2];

            if( attr_1 == 'I' )        // Italic
                field->SetItalic( true );
            else if( attr_1 != 'N' )   // No italics is default, check for error.
                SCH_PARSE_ERROR( "invalid field text italic parameter", aReader, line );

            if ( attr_2 == 'B' )       // Bold
                field->SetBold( true );
            else if( attr_2 != 'N' )   // No bold is default, check for error.
                SCH_PARSE_ERROR( "invalid field text bold parameter", aReader, line );
        }
    }

    // Fields in RAM must always have names.
    if( (unsigned) id < MANDATORY_FIELDS )
    {
        // Fields in RAM must always have names, because we are trying to get
        // less dependent on field ids and more dependent on names.
        // Plus assumptions are made in the field editors.
        field->m_name = TEMPLATE_FIELDNAME::GetDefaultFieldName( id );

        // Ensure the VALUE field = the part name (can be not the case
        // with malformed libraries: edited by hand, or converted from other tools)
        if( id == VALUE )
            field->m_Text = aPart->GetName();
    }
    else
    {
        parseQuotedString( field->m_name, aReader, line, &line, true );  // Optional.
    }
}

void LEGACY_PART_SERIALIZER::loadDrawEntries( LIB_PART * aPart, LINE_READER & aReader )
{
    const char* line = aReader.Line();

    wxCHECK_RET( strCompare( "DRAW", line, &line ), "Invalid DRAW section" );

    line = aReader.ReadLine();

    while( line )
    {
        if( strCompare( "ENDDRAW", line, &line ) )
            return;

        switch( line[0] )
        {
        case 'A':    // Arc
            aPart->AddDrawItem( loadArc( aPart, aReader ) );
            break;

        case 'C':    // Circle
            aPart->AddDrawItem( loadCircle( aPart, aReader ) );
            break;

        case 'T':    // Text
            aPart->AddDrawItem( loadText( aPart, aReader ) );
            break;

        case 'S':    // Square
            aPart->AddDrawItem( loadRectangle( aPart, aReader ) );
            break;

        case 'X':    // Pin Description
            aPart->AddDrawItem( loadPin( aPart, aReader ) );
            break;

        case 'P':    // Polyline
            aPart->AddDrawItem( loadPolyLine( aPart, aReader ) );
            break;

        case 'B':    // Bezier Curves
            aPart->AddDrawItem( loadBezier( aPart, aReader ) );
            break;

        case '#':    // Comment
        case '\n':   // Empty line
        case '\r':
        case 0:
            break;

        default:
            SCH_PARSE_ERROR( "undefined DRAW entry", aReader, line );
        }

        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "file ended prematurely loading component draw element", aReader, line );
}

void LEGACY_PART_SERIALIZER::loadFootprintFilters( LIB_PART * aPart, LINE_READER & aReader ) 
{
   const char* line = aReader.Line();

    wxCHECK_RET( strCompare( "$FPLIST", line, &line ), "Invalid footprint filter list" );

    line = aReader.ReadLine();

    while( line )
    {
        if( strCompare( "$ENDFPLIST", line, &line ) )
            return;

        wxString footprint;

        parseUnquotedString( footprint, aReader, line, &line );
        aPart->GetFootprints().Add( footprint );
        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "file ended prematurely while loading footprint filters", aReader, line );
}

LIB_ITEM * LEGACY_PART_SERIALIZER::loadArc( LIB_PART * aPart, LINE_READER & aReader ) 
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "A", line, &line ), NULL, "Invalid LIB_ARC definition" );

    LIB_ARC* arc = new LIB_ARC( aPart );

    wxPoint center;

    center.x = parseInt( aReader, line, &line );
    center.y = parseInt( aReader, line, &line );

    arc->SetPosition( center );
    arc->SetRadius( parseInt( aReader, line, &line ) );

    int angle1 = parseInt( aReader, line, &line );
    int angle2 = parseInt( aReader, line, &line );

    NORMALIZE_ANGLE_POS( angle1 );
    NORMALIZE_ANGLE_POS( angle2 );
    arc->SetFirstRadiusAngle( angle1 );
    arc->SetSecondRadiusAngle( angle2 );

    arc->SetUnit( parseInt( aReader, line, &line ) );
    arc->SetConvert( parseInt( aReader, line, &line ) );
    arc->SetWidth( parseInt( aReader, line, &line ) );

    // Old libraries (version <= 2.2) do not have always this FILL MODE param
    // when fill mode is no fill (default mode).
    if( *line != 0 )
        arc->SetFillMode( parseFillMode( aReader, line, &line ) );

    // Actual Coordinates of arc ends are read from file
    if( *line != 0 )
    {
        wxPoint arcStart, arcEnd;

        arcStart.x = parseInt( aReader, line, &line );
        arcStart.y = parseInt( aReader, line, &line );
        arcEnd.x = parseInt( aReader, line, &line );
        arcEnd.y = parseInt( aReader, line, &line );

        arc->SetStart( arcStart );
        arc->SetEnd( arcEnd );
    }
    else
    {
        // Actual Coordinates of arc ends are not read from file
        // (old library), calculate them
        wxPoint arcStart( arc->GetRadius(), 0 );
        wxPoint arcEnd( arc->GetRadius(), 0 );

        RotatePoint( &arcStart.x, &arcStart.y, -angle1 );
        arcStart += arc->GetPosition();
        arc->SetStart( arcStart );
        RotatePoint( &arcEnd.x, &arcEnd.y, -angle2 );
        arcEnd += arc->GetPosition();
        arc->SetEnd( arcEnd );
    }

    return arc;
}


LIB_ITEM* LEGACY_PART_SERIALIZER::loadCircle( LIB_PART * aPart, LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "C", line, &line ), NULL, "Invalid LIB_CIRCLE definition" );

    LIB_CIRCLE* circle = new LIB_CIRCLE( aPart );

    wxPoint center;

    center.x = parseInt( aReader, line, &line );
    center.y = parseInt( aReader, line, &line );

    circle->SetPosition( center );
    circle->SetRadius( parseInt( aReader, line, &line ) );
    circle->SetUnit( parseInt( aReader, line, &line ) );
    circle->SetConvert( parseInt( aReader, line, &line ) );
    circle->SetWidth( parseInt( aReader, line, &line ) );

    if( *line != 0 )
        circle->SetFillMode( parseFillMode( aReader, line, &line ) );

    return circle;
}


LIB_ITEM* LEGACY_PART_SERIALIZER::loadText( LIB_PART * aPart, LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "T", line, &line ), NULL, "Invalid LIB_TEXT definition" );

    LIB_TEXT* text = new LIB_TEXT( aPart );

    text->SetTextAngle( (double) parseInt( aReader, line, &line ) );

    wxPoint center;

    center.x = parseInt( aReader, line, &line );
    center.y = parseInt( aReader, line, &line );
    text->SetPosition( center );

    wxSize size;

    size.x = size.y = parseInt( aReader, line, &line );
    text->SetTextSize( size );
    text->SetVisible( !parseInt( aReader, line, &line ) );
    text->SetUnit( parseInt( aReader, line, &line ) );
    text->SetConvert( parseInt( aReader, line, &line ) );

    wxString str;

    // If quoted string loading fails, load as not quoted string.
    if( *line == '"' )
        parseQuotedString( str, aReader, line, &line );
    else
    {
        parseUnquotedString( str, aReader, line, &line );

        // In old libs, "spaces" are replaced by '~' in unquoted strings:
        str.Replace( "~", " " );
    }

    if( !str.IsEmpty() )
    {
        // convert two apostrophes back to double quote
        str.Replace( "''", "\"" );
    }

    text->SetText( str );

    // Here things are murky and not well defined.  At some point it appears the format
    // was changed to add text properties.  However rather than add the token to the end of
    // the text definition, it was added after the string and no mention if the file
    // verion was bumped or not so this code make break on very old component libraries.
    //
    // Update: apparently even in the latest version this can be different so added a test
    //         for end of line before checking for the text properties.
    //
    // BH - removed version check altogether, as EOL should be clue enough, with the 
    // already-given caveat.
    if( !is_eol( *line ) )
    {
        if( strCompare( "Italic", line, &line ) )
            text->SetItalic( true );
        else if( !strCompare( "Normal", line, &line ) )
            SCH_PARSE_ERROR( "invalid text stype, expected 'Normal' or 'Italic'", aReader, line );

        if( parseInt( aReader, line, &line ) > 0 )
            text->SetBold( true );

        // Some old libaries version > 2.0 do not have these options for text justification:
        if( !is_eol( *line ) )
        {
            switch( parseChar( aReader, line, &line ) )
            {
            case 'L': text->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );   break;
            case 'C': text->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER ); break;
            case 'R': text->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );  break;
            default: SCH_PARSE_ERROR( "invalid horizontal text justication; expected L, C, or R",
                                      aReader, line );
            }

            switch( parseChar( aReader, line, &line ) )
            {
            case 'T': text->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );    break;
            case 'C': text->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER ); break;
            case 'B': text->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM ); break;
            default: SCH_PARSE_ERROR( "invalid vertical text justication; expected T, C, or B",
                                      aReader, line );
            }
        }
    }

    return text;
}


LIB_ITEM* LEGACY_PART_SERIALIZER::loadRectangle( LIB_PART * aPart, LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "S", line, &line ), NULL, "Invalid LIB_RECTANGLE definition" );

    LIB_RECTANGLE* rectangle = new LIB_RECTANGLE( aPart );

    wxPoint pos;

    pos.x = parseInt( aReader, line, &line );
    pos.y = parseInt( aReader, line, &line );
    rectangle->SetPosition( pos );

    wxPoint end;

    end.x = parseInt( aReader, line, &line );
    end.y = parseInt( aReader, line, &line );
    rectangle->SetEnd( end );

    rectangle->SetUnit( parseInt( aReader, line, &line ) );
    rectangle->SetConvert( parseInt( aReader, line, &line ) );
    rectangle->SetWidth( parseInt( aReader, line, &line ) );

    if( *line != 0 )
        rectangle->SetFillMode( parseFillMode( aReader, line, &line ) );

    return rectangle;
}


LIB_ITEM* LEGACY_PART_SERIALIZER::loadPin( LIB_PART * aPart, LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "X", line, &line ), NULL, "Invalid LIB_PIN definition" );

    LIB_PIN* pin = new LIB_PIN( aPart );

    size_t pos = 2;                               // "X" plus ' ' space character.
    wxString tmp;
    wxString utf8Line = wxString::FromUTF8( line );
    wxStringTokenizer tokens( utf8Line, " \r\n\t" );

    if( tokens.CountTokens() < 11 )
        SCH_PARSE_ERROR( "invalid pin definition", aReader, line );

    pin->m_name = tokens.GetNextToken();
    pos += pin->m_name.size() + 1;
    pin->m_number = tokens.GetNextToken();
    pos += pin->m_number.size() + 1;

    long num;
    wxPoint position;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin X coordinate", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    position.x = (int) num;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin Y coordinate", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    position.y = (int) num;
    pin->m_position = position;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin length", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    pin->m_length = (int) num;


    tmp = tokens.GetNextToken();

    if( tmp.size() > 1 )
        THROW_PARSE_ERROR( "invalid pin orientation", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    pin->m_orientation = tmp[0];

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin number text size", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    pin->m_numTextSize = (int) num;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin name text size", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    pin->m_nameTextSize = (int) num;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin unit", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    pin->m_Unit = (int) num;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin alternate body type", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    pin->m_Convert = (int) num;

    tmp = tokens.GetNextToken();

    if( tmp.size() != 1 )
        THROW_PARSE_ERROR( "invalid pin type", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    char type = tmp[0];

    wxString attributes;

    switch( type )
    {
    case 'I': pin->m_type = PIN_INPUT;         break;
    case 'O': pin->m_type = PIN_OUTPUT;        break;
    case 'B': pin->m_type = PIN_BIDI;          break;
    case 'T': pin->m_type = PIN_TRISTATE;      break;
    case 'P': pin->m_type = PIN_PASSIVE;       break;
    case 'U': pin->m_type = PIN_UNSPECIFIED;   break;
    case 'W': pin->m_type = PIN_POWER_IN;      break;
    case 'w': pin->m_type = PIN_POWER_OUT;     break;
    case 'C': pin->m_type = PIN_OPENCOLLECTOR; break;
    case 'E': pin->m_type = PIN_OPENEMITTER;   break;
    case 'N': pin->m_type = PIN_NC;            break;
    default: THROW_PARSE_ERROR( "unknown pin type", aReader.GetSource(),
                                aReader.Line(), aReader.LineNumber(), pos );
    }

    // Optional
    if( tokens.HasMoreTokens() )       /* Special Symbol defined */
    {
        tmp = tokens.GetNextToken();

        enum
        {
            INVERTED        = 1 << 0,
            CLOCK           = 1 << 1,
            LOWLEVEL_IN     = 1 << 2,
            LOWLEVEL_OUT    = 1 << 3,
            FALLING_EDGE    = 1 << 4,
            NONLOGIC        = 1 << 5
        };

        int flags = 0;

        for( int j = tmp.size(); j > 0; )
        {
            switch( tmp[--j].GetValue() )
            {
            case '~': break;
            case 'N': pin->m_attributes |= PIN_INVISIBLE; break;
            case 'I': flags |= INVERTED;     break;
            case 'C': flags |= CLOCK;        break;
            case 'L': flags |= LOWLEVEL_IN;  break;
            case 'V': flags |= LOWLEVEL_OUT; break;
            case 'F': flags |= FALLING_EDGE; break;
            case 'X': flags |= NONLOGIC;     break;
            default: THROW_PARSE_ERROR( "invalid pin attribut", aReader.GetSource(),
                                        aReader.Line(), aReader.LineNumber(), pos );
            }

            pos += 1;
        }

        switch( flags )
        {
        case 0:                   pin->m_shape = PINSHAPE_LINE;               break;
        case INVERTED:            pin->m_shape = PINSHAPE_INVERTED;           break;
        case CLOCK:               pin->m_shape = PINSHAPE_CLOCK;              break;
        case INVERTED | CLOCK:    pin->m_shape = PINSHAPE_INVERTED_CLOCK;     break;
        case LOWLEVEL_IN:         pin->m_shape = PINSHAPE_INPUT_LOW;          break;
        case LOWLEVEL_IN | CLOCK: pin->m_shape = PINSHAPE_CLOCK_LOW;          break;
        case LOWLEVEL_OUT:        pin->m_shape = PINSHAPE_OUTPUT_LOW;         break;
        case FALLING_EDGE:        pin->m_shape = PINSHAPE_FALLING_EDGE_CLOCK; break;
        case NONLOGIC:            pin->m_shape = PINSHAPE_NONLOGIC;           break;
        default: SCH_PARSE_ERROR( "pin attributes do not define a valid pin shape", aReader, line );
        }
    }

    return pin;
}


LIB_ITEM* LEGACY_PART_SERIALIZER::loadPolyLine( LIB_PART * aPart, LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "P", line, &line ), NULL, "Invalid LIB_POLYLINE definition" );

    LIB_POLYLINE* polyLine = new LIB_POLYLINE( aPart );

    int points = parseInt( aReader, line, &line );
    polyLine->SetUnit( parseInt( aReader, line, &line ) );
    polyLine->SetConvert( parseInt( aReader, line, &line ) );
    polyLine->SetWidth( parseInt( aReader, line, &line ) );
    polyLine->Reserve( points );

    wxPoint pt;

    for( int i = 0; i < points; i++ )
    {
        pt.x = parseInt( aReader, line, &line );
        pt.y = parseInt( aReader, line, &line );
        polyLine->AddPoint( pt );
    }

    if( *line != 0 )
        polyLine->SetFillMode( parseFillMode( aReader, line, &line ) );

    return polyLine;
}


LIB_ITEM* LEGACY_PART_SERIALIZER::loadBezier( LIB_PART * aPart, LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "B", line, &line ), NULL, "Invalid LIB_BEZIER definition" );

    LIB_BEZIER* bezier = new LIB_BEZIER( aPart );

    int points = parseInt( aReader, line, &line );
    bezier->SetUnit( parseInt( aReader, line, &line ) );
    bezier->SetConvert( parseInt( aReader, line, &line ) );
    bezier->SetWidth( parseInt( aReader, line, &line ) );

    wxPoint pt;
    bezier->Reserve( points );

    for( int i = 0; i < points; i++ )
    {
        pt.x = parseInt( aReader, line, &line );
        pt.y = parseInt( aReader, line, &line );
        bezier->AddPoint( pt );
    }

    if( *line != 0 )
        bezier->SetFillMode( parseFillMode( aReader, line, &line ) );

    return bezier;
}

/****************************************************************************\
 *                                                                           *
 * Above was reading; below is writing.  I'm making this comment block       *
 * very tall so that it's visible in Kate's thumbnail-scrollbar thingy.      *
 *                                                                           *
 * Here's a kitty!                                                           *
 *                                                                           *
 *           ,.                 .,                                           *
 *          ,: ':.    .,.    .:' :,                                          *
 *          ,',   '.:'   ':.'   ,',                                          *
 *          : '.  '         '  .' :                                          *
 *          ', : '           ' : ,'                                          *
 *          '.' .,:,.   .,:,. '.'                                            *
 *           ,:    V '. .' V    :,                                           *
 *          ,:        / '        :,                                          *
 *          ,:                   :,                                          *
 *           ,:       =:=       :,                                           *
 *            ,: ,     :     , :,                                            *
 *             :' ',.,' ',.,:' ':                                            *
 *            :'      ':WW::'   '.                                           *
 *           .:'       '::::'   ':                                           *
 *           ,:        '::::'    :,                                          *
 *           :'         ':::'    ':                                          *
 *          ,:           ':''     :.                                         *
 *         .:'             '.     ',.                                        *
 *        ,:'               ''     '.                                        *
 *        .:'               .',    ':                                        *
 *       .:'               .'.,     :                                        *
 *       .:                .,''     :                                        *
 *       ::                .,''    ,:                                        *
 *       ::              .,'','   .:'                                        *
 *     .,::'.           .,','     ::::.                                      *
 *   .:'     ',.       ,:,       ,WWWWW,                                     *
 *   :'        :       :W:'     :WWWWWWW,          .,.                       *
 *   :         ',      WWW      WWWWWWWWW          '::,                      *
 *   '.         ',     WWW     :WWWWWWWWW            '::,                    *
 *    '.         :     WWW     :WWWWWWWW'             :::                    *
 *     '.       ,:     WWW     :WWWWWWW'             .:::                    *
 *      '.     .W:     WWW     :WWWWWW'           .,:::'                     *
 *       '.   :WW:     WWW     :WWWWW'      .,,:::::''                       *
 *      .,'   ''::     :W:     :WWWWW.  .,::::''                             *
 *   ,'        ''','',',','','''WWWWW::::''                                  *
 *    ':,,,,,,,':  :  : : :  :  :WWWW'''                                     *
 *                                                                           *
 *                                                                           *
 *                                                                           *
 *                                                                           *
 * **************************************************************************/

void LEGACY_PART_SERIALIZER::WritePart( LIB_PART * aSymbol, OUTPUTFORMATTER & aFormatter) 
{
    wxCHECK_RET( aSymbol, "Invalid LIB_PART pointer." );

    LIB_FIELD&  value = aSymbol->GetValueField();

    // First line: it s a comment (component name for readers)
    aFormatter.Print( 0, "#\n# %s\n#\n", TO_UTF8( value.GetText() ) );

    // Save data
    aFormatter.Print( 0, "DEF" );

    if( value.IsVisible() )
    {
        aFormatter.Print( 0, " %s", TO_UTF8( value.GetText() ) );
    }
    else
    {
        aFormatter.Print( 0, " ~%s", TO_UTF8( value.GetText() ) );
    }

    LIB_FIELD& reference = aSymbol->GetReferenceField();

    if( !reference.GetText().IsEmpty() )
    {
        aFormatter.Print( 0, " %s", TO_UTF8( reference.GetText() ) );
    }
    else
    {
        aFormatter.Print( 0, " ~" );
    }

    aFormatter.Print( 0, " %d %d %c %c %d %c %c\n",
                       0, aSymbol->GetPinNameOffset(),
                       aSymbol->ShowPinNumbers() ? 'Y' : 'N',
                       aSymbol->ShowPinNames() ? 'Y' : 'N',
                       aSymbol->GetUnitCount(), aSymbol->UnitsLocked() ? 'L' : 'F',
                       aSymbol->IsPower() ? 'P' : 'N' );

    timestamp_t dateModified = aSymbol->GetDateLastEdition();

    if( dateModified != 0 )
    {
        int sec  = dateModified & 63;
        int min  = ( dateModified >> 6 ) & 63;
        int hour = ( dateModified >> 12 ) & 31;
        int day  = ( dateModified >> 17 ) & 31;
        int mon  = ( dateModified >> 22 ) & 15;
        int year = ( dateModified >> 26 ) + 1990;

        aFormatter.Print( 0, "Ti %d/%d/%d %d:%d:%d\n", year, mon, day, hour, min, sec );
    }

    LIB_FIELDS fields;
    aSymbol->GetFields( fields );

    // Mandatory fields:
    // may have their own save policy so there is a separate loop for them.
    // Empty fields are saved, because the user may have set visibility,
    // size and orientation
    for( int i = 0;  i < MANDATORY_FIELDS;  ++i )
    {
        saveField( &fields[i], aFormatter );
    }

    // User defined fields:
    // may have their own save policy so there is a separate loop for them.

    int fieldId = MANDATORY_FIELDS;     // really wish this would go away.

    for( unsigned i = MANDATORY_FIELDS; i < fields.size(); ++i )
    {
        // There is no need to save empty fields, i.e. no reason to preserve field
        // names now that fields names come in dynamically through the template
        // fieldnames.
        if( !fields[i].GetText().IsEmpty() )
        {
            fields[i].SetId( fieldId++ );
            saveField( &fields[i], aFormatter );
        }
    }

    // Save the alias list: a line starting by "ALIAS".  The first alias is the root
    // and has the same name as the component.  In the old library file format this
    // alias does not get added to the alias list.
    if( aSymbol->GetAliasCount() > 1 )
    {
        wxArrayString aliases = aSymbol->GetAliasNames();

        aFormatter.Print( 0, "ALIAS" );

        for( unsigned i = 1; i < aliases.size(); i++ )
        {
            aFormatter.Print( 0, " %s", TO_UTF8( aliases[i] ) );
        }

        aFormatter.Print( 0, "\n" );
    }

    wxArrayString footprints = aSymbol->GetFootprints();

    // Write the footprint filter list
    if( footprints.GetCount() != 0 )
    {
        aFormatter.Print( 0, "$FPLIST\n" );

        for( unsigned i = 0; i < footprints.GetCount(); i++ )
        {
            aFormatter.Print( 0, " %s\n", TO_UTF8( footprints[i] ) );
        }

        aFormatter.Print( 0, "$ENDFPLIST\n" );
    }

    // Save graphics items (including pins)
    if( !aSymbol->GetDrawItems().empty() )
    {
        // Sort the draw items in order to editing a file editing by hand.
        aSymbol->GetDrawItems().sort();

        aFormatter.Print( 0, "DRAW\n" );

        for( LIB_ITEM& item : aSymbol->GetDrawItems() )
        {
            switch( item.Type() )
            {
            case LIB_FIELD_T:              // Fields have already been saved above.
                continue;

            case LIB_ARC_T:
                saveArc( &item, aFormatter );
                break;

            case LIB_BEZIER_T:
                saveBezier( &item, aFormatter );
                break;

            case LIB_CIRCLE_T:
                saveCircle( &item, aFormatter );
                break;

            case LIB_PIN_T:
                savePin( &item, aFormatter );
                break;

            case LIB_POLYLINE_T:
                savePolyLine( &item, aFormatter );
                break;

            case LIB_RECTANGLE_T:
                saveRectangle( &item, aFormatter );
                break;

            case LIB_TEXT_T:
                saveText( &item, aFormatter );
                break;

            default:
                ;
            }
        }

        aFormatter.Print( 0, "ENDDRAW\n" );
    }

    aFormatter.Print( 0, "ENDDEF\n" );
}

void LEGACY_PART_SERIALIZER::saveField( LIB_FIELD * aField, OUTPUTFORMATTER & aFormatter )
{
    wxCHECK_RET( aField && aField->Type() == LIB_FIELD_T, "Invalid LIB_FIELD object." );

    int      hjustify, vjustify;
    int      id = aField->GetId();
    wxString text = aField->m_Text;

    hjustify = 'C';

    if( aField->GetHorizJustify() == GR_TEXT_HJUSTIFY_LEFT )
        hjustify = 'L';
    else if( aField->GetHorizJustify() == GR_TEXT_HJUSTIFY_RIGHT )
        hjustify = 'R';

    vjustify = 'C';

    if( aField->GetVertJustify() == GR_TEXT_VJUSTIFY_BOTTOM )
        vjustify = 'B';
    else if( aField->GetVertJustify() == GR_TEXT_VJUSTIFY_TOP )
        vjustify = 'T';

    aFormatter.Print( 0, "F%d %s %d %d %d %c %c %c %c%c%c",
                       id,
                       EscapedUTF8( text ).c_str(),       // wraps in quotes
                       aField->GetTextPos().x, aField->GetTextPos().y, aField->GetTextWidth(),
                       aField->GetTextAngle() == 0 ? 'H' : 'V',
                       aField->IsVisible() ? 'V' : 'I',
                       hjustify, vjustify,
                       aField->IsItalic() ? 'I' : 'N',
                       aField->IsBold() ? 'B' : 'N' );

    /* Save field name, if necessary
     * Field name is saved only if it is not the default name.
     * Just because default name depends on the language and can change from
     * a country to another
     */
    wxString defName = TEMPLATE_FIELDNAME::GetDefaultFieldName( id );

    if( id >= FIELD1 && !aField->m_name.IsEmpty() && aField->m_name != defName )
        aFormatter.Print( 0, " %s", EscapedUTF8( aField->m_name ).c_str() );
    
    aFormatter.Print(0, "\n");

}

void LEGACY_PART_SERIALIZER::saveArc( LIB_ITEM * anItem, OUTPUTFORMATTER & aFormatter )
{
    wxCHECK_RET( anItem && anItem->Type() == LIB_ARC_T, "Invalid LIB_ARC object." );
    
    LIB_ARC * aArc = ( LIB_ARC * ) anItem;

    int x1 = aArc->GetFirstRadiusAngle();

    if( x1 > 1800 )
        x1 -= 3600;

    int x2 = aArc->GetSecondRadiusAngle();

    if( x2 > 1800 )
        x2 -= 3600;

    aFormatter.Print( 0, "A %d %d %d %d %d %d %d %d %c %d %d %d %d\n",
                       aArc->GetPosition().x, aArc->GetPosition().y,
                       aArc->GetRadius(), x1, x2, aArc->GetUnit(), aArc->GetConvert(),
                       aArc->GetWidth(), fill_tab[aArc->GetFillMode()],
                       aArc->GetStart().x, aArc->GetStart().y,
                       aArc->GetEnd().x, aArc->GetEnd().y );
}

void LEGACY_PART_SERIALIZER::saveCircle( LIB_ITEM * anItem, OUTPUTFORMATTER & aFormatter )
{
    wxCHECK_RET( anItem && anItem->Type() == LIB_CIRCLE_T, "Invalid LIB_CIRCLE object." );

    LIB_CIRCLE * aCircle = ( LIB_CIRCLE * ) anItem;
    
    aFormatter.Print( 0, "C %d %d %d %d %d %d %c\n",
                       aCircle->GetPosition().x, aCircle->GetPosition().y,
                       aCircle->GetRadius(), aCircle->GetUnit(), aCircle->GetConvert(),
                       aCircle->GetWidth(), fill_tab[aCircle->GetFillMode()] );
}

void LEGACY_PART_SERIALIZER::saveText( LIB_ITEM * anItem, OUTPUTFORMATTER & aFormatter )
{
    wxCHECK_RET( anItem && anItem->Type() == LIB_TEXT_T, "Invalid LIB_TEXT object." );
    
    LIB_TEXT * aText = ( LIB_TEXT * ) anItem;

    wxString text = aText->GetText();

    if( text.Contains( wxT( " " ) ) || text.Contains( wxT( "~" ) ) || text.Contains( wxT( "\"" ) ) )
    {
        // convert double quote to similar-looking two apostrophes
        text.Replace( wxT( "\"" ), wxT( "''" ) );
        text = wxT( "\"" ) + text + wxT( "\"" );
    }

    aFormatter.Print( 0, "T %g %d %d %d %d %d %d %s", aText->GetTextAngle(),
                       aText->GetTextPos().x, aText->GetTextPos().y,
                       aText->GetTextWidth(), !aText->IsVisible(),
                       aText->GetUnit(), aText->GetConvert(), TO_UTF8( text ) );

    aFormatter.Print( 0, " %s %d", aText->IsItalic() ? "Italic" : "Normal", aText->IsBold() );

    char hjustify = 'C';

    if( aText->GetHorizJustify() == GR_TEXT_HJUSTIFY_LEFT )
        hjustify = 'L';
    else if( aText->GetHorizJustify() == GR_TEXT_HJUSTIFY_RIGHT )
        hjustify = 'R';

    char vjustify = 'C';

    if( aText->GetVertJustify() == GR_TEXT_VJUSTIFY_BOTTOM )
        vjustify = 'B';
    else if( aText->GetVertJustify() == GR_TEXT_VJUSTIFY_TOP )
        vjustify = 'T';

    aFormatter.Print( 0, " %c %c\n", hjustify, vjustify );
}

void LEGACY_PART_SERIALIZER::saveRectangle( LIB_ITEM * anItem, OUTPUTFORMATTER & aFormatter )
{
    wxCHECK_RET( anItem && anItem->Type() == LIB_RECTANGLE_T,
                 "Invalid LIB_RECTANGLE object." );
    
    LIB_RECTANGLE * aRectangle = ( LIB_RECTANGLE * ) anItem;

    aFormatter.Print( 0, "S %d %d %d %d %d %d %d %c\n",
                       aRectangle->GetPosition().x, aRectangle->GetPosition().y,
                       aRectangle->GetEnd().x, aRectangle->GetEnd().y,
                       aRectangle->GetUnit(), aRectangle->GetConvert(),
                       aRectangle->GetWidth(), fill_tab[aRectangle->GetFillMode()] );
}

void LEGACY_PART_SERIALIZER::savePin( LIB_ITEM * anItem, OUTPUTFORMATTER & aFormatter )
{
    wxCHECK_RET( anItem && anItem->Type() == LIB_PIN_T, "Invalid LIB_PIN object." );

    LIB_PIN * aPin = ( LIB_PIN * ) anItem;
    
    int      Etype;

    switch( aPin->GetType() )
    {
    default:
    case PIN_INPUT:
        Etype = 'I';
        break;

    case PIN_OUTPUT:
        Etype = 'O';
        break;

    case PIN_BIDI:
        Etype = 'B';
        break;

    case PIN_TRISTATE:
        Etype = 'T';
        break;

    case PIN_PASSIVE:
        Etype = 'P';
        break;

    case PIN_UNSPECIFIED:
        Etype = 'U';
        break;

    case PIN_POWER_IN:
        Etype = 'W';
        break;

    case PIN_POWER_OUT:
        Etype = 'w';
        break;

    case PIN_OPENCOLLECTOR:
        Etype = 'C';
        break;

    case PIN_OPENEMITTER:
        Etype = 'E';
        break;

    case PIN_NC:
        Etype = 'N';
        break;
    }

    if( !aPin->GetName().IsEmpty() )
        aFormatter.Print( 0, "X %s", TO_UTF8( aPin->GetName() ) );
    else
        aFormatter.Print( 0, "X ~" );

    aFormatter.Print( 0, " %s %d %d %d %c %d %d %d %d %c",
                       aPin->GetNumber().IsEmpty() ? "~" : TO_UTF8( aPin->GetNumber() ),
                       aPin->GetPosition().x, aPin->GetPosition().y,
                       (int) aPin->GetLength(), (int) aPin->GetOrientation(),
                       aPin->GetNumberTextSize(), aPin->GetNameTextSize(),
                       aPin->GetUnit(), aPin->GetConvert(), Etype );

    if( aPin->GetShape() || !aPin->IsVisible() )
        aFormatter.Print( 0, " " );

    if( !aPin->IsVisible() )
        aFormatter.Print( 0, "N" );

    switch( aPin->GetShape() )
    {
    case PINSHAPE_LINE:
        break;

    case PINSHAPE_INVERTED:
        aFormatter.Print( 0, "I" );
        break;

    case PINSHAPE_CLOCK:
        aFormatter.Print( 0, "C" );
        break;

    case PINSHAPE_INVERTED_CLOCK:
        aFormatter.Print( 0, "IC" );
        break;

    case PINSHAPE_INPUT_LOW:
        aFormatter.Print( 0, "L" );
        break;

    case PINSHAPE_CLOCK_LOW:
        aFormatter.Print( 0, "CL" );
        break;

    case PINSHAPE_OUTPUT_LOW:
        aFormatter.Print( 0, "V" );
        break;

    case PINSHAPE_FALLING_EDGE_CLOCK:
        aFormatter.Print( 0, "F" );
        break;

    case PINSHAPE_NONLOGIC:
        aFormatter.Print( 0, "X" );
        break;

    default:
        assert( !"Invalid pin shape" );
    }

    aFormatter.Print( 0, "\n" );

    aPin->ClearFlags( IS_CHANGED );
}

void LEGACY_PART_SERIALIZER::savePolyLine( LIB_ITEM * anItem, OUTPUTFORMATTER & aFormatter )
{
    wxCHECK_RET( anItem && anItem->Type() == LIB_POLYLINE_T, "Invalid LIB_POLYLINE object." );

    LIB_POLYLINE * aPolyLine = ( LIB_POLYLINE * ) anItem;
    
    int ccount = aPolyLine->GetCornerCount();

    aFormatter.Print( 0, "P %d %d %d %d", ccount, aPolyLine->GetUnit(), aPolyLine->GetConvert(),
                       aPolyLine->GetWidth() );

    for( const auto& pt : aPolyLine->GetPolyPoints() )
    {
        aFormatter.Print( 0, " %d %d", pt.x, pt.y );
    }

    aFormatter.Print( 0, " %c\n", fill_tab[aPolyLine->GetFillMode()] );
}

void LEGACY_PART_SERIALIZER::saveBezier( LIB_ITEM * anItem, OUTPUTFORMATTER & aFormatter )
{
    wxCHECK_RET( anItem && anItem->Type() == LIB_BEZIER_T, "Invalid LIB_BEZIER object." );

    LIB_BEZIER * aBezier = ( LIB_BEZIER * )anItem;
    
    aFormatter.Print( 0, "B %u %d %d %d", (unsigned)aBezier->GetPoints().size(),
                       aBezier->GetUnit(), aBezier->GetConvert(), aBezier->GetWidth() );

    for( const auto& pt : aBezier->GetPoints() )
        aFormatter.Print( 0, " %d %d", pt.x, pt.y );

    aFormatter.Print( 0, " %c\n", fill_tab[aBezier->GetFillMode()] );
}







