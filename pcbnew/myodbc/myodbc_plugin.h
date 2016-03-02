/*
 * myodbc plugin by cheetah
 */

#ifndef MYODBC_PLUGIN_H_
#define MYODBC_PLUGIN_H_

#include <kicad_plugin.h>


class MYSQL_ADAPTER;


/**
   \todo docs
 */
class MYODBC_PLUGIN : public PCB_IO
{
public:
    //-----<PLUGIN API>----------------------------------------------------------
    const wxString PluginName() const;

    const wxString GetFileExtension() const;

    wxArrayString FootprintEnumerate( const wxString& aLibraryPath,
            const PROPERTIES* aProperties = NULL );

    MODULE* FootprintLoad( const wxString& aLibraryPath,
            const wxString& aFootprintName, const PROPERTIES* aProperties );

    void FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint,
            const PROPERTIES* aProperties = NULL );

    void FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
            const PROPERTIES* aProperties = NULL );

    bool IsFootprintLibWritable( const wxString& aLibraryPath );

    void FootprintLibOptions( PROPERTIES* aListToAppendTo ) const;

    void FootprintLibCreate( const wxString& aLibraryPath, const PROPERTIES* aProperties );

    bool FootprintLibDelete( const wxString& aLibraryPath, const PROPERTIES* aProperties );

    //-----</PLUGIN API>---------------------------------------------------------

    MYODBC_PLUGIN();        // constructor, if any, must be zero arg
    ~MYODBC_PLUGIN();

protected:
    
    MYSQL_ADAPTER *  m_db;
    

};


#endif // MYODBC_PLUGIN_H_
