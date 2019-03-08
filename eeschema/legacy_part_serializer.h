#ifndef LEGACY_PART_SERIALIZER_H
#define LEGACY_PART_SERIALIZER_H

class LIB_ITEM;
class LIB_PART;
class LIB_FIELD;
class LINE_READER;
class OUTPUTFORMATTER;

/**
 * Encapsulates the legacy EESCHEMA symbol library text format
 * 
 * This code is almost verbatim stolen right out of SCH_LEGACY_PLUGIN_CACHE
 * so that I can expose it to ODBC_SCH_PLUGIN, which uses the same format.
 * Note that a few cases of version checking have been removed, as it seemed
 * that using the latest-version approach to earlier-version data is innocuous
 * (that is, latest-version parsing does a perfectly acceptible thing in the
 * presence of earlier-version data, rendering the version-checking 
 * unnecessary).
 * 
 * My plan is to present this to the upstream maintainers in the hopes that 
 * it will be incorporated, thereby making the job of maintaining my branch
 * much less of a headache.
 */
class LEGACY_PART_SERIALIZER {
public:
    /**
     * Reads a legacy LIB_PART descriptor from aReader to configure aPart
     */
    static void ReadPart( LIB_PART * aPart, LINE_READER & aReader );

    /**
     * Writes a legacy LIB_PART descriptor for aPart to aFormatter
     */
    static void WritePart( LIB_PART * aSymbol, OUTPUTFORMATTER & aFormatter );
    
private:
    static void loadAliases( LIB_PART * aPart, LINE_READER & aReader );
    static void loadField( LIB_PART * aPart, LINE_READER & aReader );
    static void loadDrawEntries( LIB_PART * aPart, LINE_READER & aReader );
    static void loadFootprintFilters( LIB_PART * aPart, LINE_READER & aReader );
    
    static LIB_ITEM * loadArc( LIB_PART * aPart, LINE_READER & aReader );
    static LIB_ITEM * loadCircle( LIB_PART * aPart, LINE_READER & aReader );
    static LIB_ITEM * loadText( LIB_PART * aPart, LINE_READER & aReader );
    static LIB_ITEM * loadRectangle( LIB_PART * aPart, LINE_READER & aReader );
    static LIB_ITEM * loadPin( LIB_PART * aPart, LINE_READER & aReader );
    static LIB_ITEM * loadPolyLine( LIB_PART * aPart, LINE_READER & aReader );
    static LIB_ITEM * loadBezier( LIB_PART * aPart, LINE_READER & aReader );
    
    static void saveField( LIB_FIELD * aField, OUTPUTFORMATTER & aFormatter );
    
    static void saveArc( LIB_ITEM * anItem, OUTPUTFORMATTER & aFormatter );
    static void saveCircle( LIB_ITEM * anItem, OUTPUTFORMATTER & aFormatter );
    static void saveText( LIB_ITEM * anItem, OUTPUTFORMATTER & aFormatter );
    static void saveRectangle( LIB_ITEM * anItem, OUTPUTFORMATTER & aFormatter );
    static void savePin( LIB_ITEM * anItem, OUTPUTFORMATTER & aFormatter );
    static void savePolyLine( LIB_ITEM * anItem, OUTPUTFORMATTER & aFormatter );
    static void saveBezier( LIB_ITEM * anItem, OUTPUTFORMATTER & aFormatter );
    
};

#endif // LEGACY_PART_SERIALIZER_H
