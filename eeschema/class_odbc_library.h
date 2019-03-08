/**
 * \file ODBC symbol library loader
 */

#ifndef ODBC_SYMBOL_LIBRARY_H_
#define ODBC_SYMBOL_LIBRARY_H_

#include <sch_io_mgr.h>
#include <sqlext.h>
#include <memory>
#include <map>

class ODBC_SCH_READER;

class ODBC_SCH_PLUGIN : public SCH_PLUGIN {
public:
    ODBC_SCH_PLUGIN();
    ODBC_SCH_PLUGIN( int aType, const wxString& aConnectionFileName );
    virtual ~ODBC_SCH_PLUGIN();
    

    /**
     * New section: Overrides of selected portions of SCH_PLUGIN
     */
        //-----<PUBLIC SCH_PLUGIN API>-------------------------------------------------

    const wxString GetName() const override
    {
        return wxT( "MyODBC-Lib" );
    }

    const wxString GetFileExtension() const override
    {
        return wxT( "lib" );
    }

    // This class doesn't do caching, so always indicate change.  This may change
    // to never indicate change.
    virtual int GetModifyHash() const override { return m_dirty_count++; }

 
    virtual void SaveLibrary( const wxString& aFileName, const PROPERTIES* aProperties = NULL ) override;

    virtual size_t GetSymbolLibCount( const wxString&   aLibraryPath,
                                      const PROPERTIES* aProperties = NULL ) override;

    virtual void EnumerateSymbolLib( wxArrayString&    aAliasNameList,
                                     const wxString&   aLibraryPath,
                                     const PROPERTIES* aProperties = NULL ) override;

    virtual void EnumerateSymbolLib( std::vector<LIB_ALIAS*>& aAliasList,
                                     const wxString&   aLibraryPath,
                                     const PROPERTIES* aProperties = NULL ) override;

    virtual LIB_ALIAS* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                                   const PROPERTIES* aProperties = NULL ) override;

    virtual void SaveSymbol( const wxString& aLibraryName, const LIB_PART* aSymbol,
                             const PROPERTIES* aProperties = NULL ) override;

    virtual void DeleteAlias( const wxString& aLibraryPath, const wxString& aAliasName,
                              const PROPERTIES* aProperties = NULL ) override;

    virtual void DeleteSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                               const PROPERTIES* aProperties = NULL ) override;

    /**
     * Return true if the library at \a aLibraryPath is writable.  (Often
     * system libraries are read only because of where they are installed.)
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file,
     *                     or URL containing several symbols.
     *
     * @throw IO_ERROR if no library at aLibraryPath exists.
     */
    virtual bool IsSymbolLibWritable( const wxString& aLibraryPath ) override;

    /**
     * Append supported #SCH_PLUGIN options to \a aListToAppenTo along with internationalized
     * descriptions.  Options are typically appended so that a derived SCH_PLUGIN can call
     * its base class function by the same name first, thus inheriting options declared there.
     * (Some base class options could pertain to all Symbol*() functions in all derived
     * SCH_PLUGINs.)  Note that since aListToAppendTo is a PROPERTIES object, all options
     * will be unique and last guy wins.
     *
     * @param aListToAppendTo holds a tuple of
     * <dl>
     *   <dt>option</dt>
     *   <dd>This eventually is what shows up into the fp-lib-table "options"
     *       field, possibly combined with others.</dd>
     *   <dt>internationalized description</dt>
     *   <dd>The internationalized description is displayed in DIALOG_FP_SCH_PLUGIN_OPTIONS.
     *      It may be multi-line and be quite explanatory of the option.</dd>
     *  </dl>
     * <br>
     *  In the future perhaps \a aListToAppendTo evolves to something capable of also
     *  holding a wxValidator for the cells in said dialog:
     *  http://forums.wxwidgets.org/viewtopic.php?t=23277&p=104180.
     *   This would require a 3 column list, and introducing wx GUI knowledge to
     *   #SCH_PLUGIN, which has been avoided to date.
     */
    virtual void SymbolLibOptions( PROPERTIES* aListToAppendTo ) const override;

    virtual bool CheckHeader( const wxString& aFileName ) override;
    
    static bool ChkHeader( const wxString& aFileName);

    virtual const wxString& GetError() const override;

private:
    typedef std::shared_ptr<ODBC_SCH_READER> READER_PTR;
    // Not thread-safe.  Need to add a long-lived singleton element to this class
    READER_PTR getReaderByPath( const wxString & libPath );
    READER_PTR getReaderByName( const wxString & name ) const;
    static std::map<wxString, READER_PTR> m_libs_by_path;
    static std::map<wxString, READER_PTR> m_libs_by_name;
    wxString m_last_error_string;
    mutable int m_dirty_count;
};

class ENABLE_ODBC_TRACE {
public:
    ENABLE_ODBC_TRACE();
};


#endif
