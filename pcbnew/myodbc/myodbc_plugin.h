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
    const wxString PluginName() const override;

    const wxString GetFileExtension() const override;

    void FootprintEnumerate( wxArrayString & aArrayString, const wxString& aLibraryPath,
            const PROPERTIES* aProperties = NULL ) override;

    MODULE* FootprintLoad( const wxString& aLibraryPath,
            const wxString& aFootprintName, const PROPERTIES* aProperties ) override;

    void FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint,
            const PROPERTIES* aProperties = NULL ) override;

    void FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
            const PROPERTIES* aProperties = NULL ) override;

    bool IsFootprintLibWritable( const wxString& aLibraryPath ) override;

    void FootprintLibOptions( PROPERTIES* aListToAppendTo ) const override;

    void FootprintLibCreate( const wxString& aLibraryPath, const PROPERTIES* aProperties ) override;

    bool FootprintLibDelete( const wxString& aLibraryPath, const PROPERTIES* aProperties ) override;

    //-----</PLUGIN API>---------------------------------------------------------

    MYODBC_PLUGIN();        // constructor, if any, must be zero arg
    ~MYODBC_PLUGIN();

protected:
    
    MYSQL_ADAPTER *  m_db;
    

};


#endif // MYODBC_PLUGIN_H_
