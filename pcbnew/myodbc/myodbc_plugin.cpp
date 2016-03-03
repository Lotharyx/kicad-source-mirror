/*
 *  MYODBC
 */


/*
 * 
 */


#ifndef WIN32_LEAN_AND_MEAN
// when WIN32_LEAN_AND_MEAN is defined, some useless includes in <window.h>
// are skipped, and this avoid some compil issues
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef WIN32
 // defines needed by avhttp
  // Minimal Windows version is XP: Google for _WIN32_WINNT
   #define _WIN32_WINNT   0x0501
    #define WINVER         0x0501
    #endif
    
    #include <sstream>
    #include <boost/ptr_container/ptr_map.hpp>
    #include <set>
    
    #include <fctsys.h>
    // Under Windows Mingw/msys, avhttp.hpp should be included after fctsys.h
    // in fact after wx/wx.h, included by fctsys.h,
    // to avoid issues (perhaps due to incompatible defines)
    
    #include <io_mgr.h>
    #include <richio.h>
    #include <pcb_parser.h>
    #include <class_board.h>
    #include <myodbc_plugin.h>
    #include <class_module.h>
    #include <macros.h>
    #include <fp_lib_table.h>       // ExpandSubstitutions()
    #include <wx/sstream.h>
    
    #include "mysql_adapter.h"
    #include "mysql_results.h"
    
    // Helper function; ideally this would be a static method on MYODBC_PLUGIN but there are...complexities.
    void unpack_parameters(MYSQL_ADAPTER::connection_parameters & parameters, const PROPERTIES* properties ) {
        PROPERTIES::const_iterator found_thing;
        found_thing = properties->find("mysql_server_name");
        if(found_thing != properties->end())
            parameters.host = found_thing->second;
        found_thing = properties->find("mysql_server_port");
        if(found_thing != properties->end())
            parameters.port = found_thing->second;
        found_thing = properties->find("mysql_user");
        if(found_thing != properties->end())
            parameters.username = found_thing->second;
        found_thing = properties->find("mysql_password");
        if(found_thing != properties->end())
            parameters.password = found_thing->second;
        found_thing = properties->find("mysql_database");
        if(found_thing != properties->end())
            parameters.db = found_thing->second;
    }
    
    
    
    MYODBC_PLUGIN::MYODBC_PLUGIN() :
        PCB_IO(),
            m_db(new MYSQL_ADAPTER)
            {
            }
            
            
            MYODBC_PLUGIN::~MYODBC_PLUGIN()
            {
                delete m_db;
            }
            
            
            const wxString MYODBC_PLUGIN::PluginName() const
            {
                return wxT( "mysql_odbc" );
            }
            
            
            const wxString MYODBC_PLUGIN::GetFileExtension() const
            {
                return wxEmptyString;
            }
            
            
            wxArrayString MYODBC_PLUGIN::FootprintEnumerate(
                const wxString& aLibraryPath, const PROPERTIES* aProperties )
            {
                wxArrayString ret;
                
                MYSQL_ADAPTER::connection_parameters parameters;
                unpack_parameters(parameters, aProperties);
                m_db->Disconnect(); // Just in case!
                if(m_db->Connect(parameters)) {
                    MYSQL_RESULTS * results = 0;
                    if(m_db->Query("SELECT DISTINCT `name` FROM `footprints` ORDER BY `name` ASC", &results)) {
                        while(results->Fetch()) {
                            std::string fpname;
                            if(results->GetData(1, fpname))
                                ret.Add(fpname);
                        }
                    } else {
                        std::string msg = StrPrintf( "Could not fetch footprint list.  Server said: %s", m_db->LastErrorString().c_str() );
                        THROW_IO_ERROR( msg );
                    }
                            delete results;
                            // issue a select
                            // disconnect
                            // iterate results
                            //     for( MYSET::const_iterator it = unique.begin();  it != unique.end();  ++it )
                            //     {
                            //         ret.Add( *it );
                            //     }
                } else {
                    std::string msg = StrPrintf( "Could not connect to MySQL.  Driver said: %s", m_db->LastErrorString().c_str() );
                    THROW_IO_ERROR( msg );
                }
                    return ret;
            }
            
            
            MODULE* MYODBC_PLUGIN::FootprintLoad( const wxString& aLibraryPath,
                                                  const wxString& aFootprintName, const PROPERTIES* aProperties )
            {
                MODULE * ret = NULL;
                MYSQL_ADAPTER::connection_parameters parameters;
                unpack_parameters(parameters, aProperties);
                m_db->Disconnect(); // Just in case!
                if(m_db->Connect(parameters)) {
                    MYSQL_RESULTS * results = 0;
                    std::string query = "select `data` from `footprints` where `name` LIKE '";
                    query += aFootprintName;
                    query += "' ORDER BY `revision_stamp` DESC LIMIT 1";
                    if(m_db->Query(query, &results)) {  
                        if(results->Fetch()) {
                            std::string module_data; // This guy gets the stuff
                                            results->GetData(1, module_data); 
                                            STRING_LINE_READER reader( module_data, aFootprintName );
                                            // I am a PCB_IO derivative with my own PCB_PARSER
                                            m_parser->SetLineReader( &reader );     // ownership not passed
                                            
                                            ret = (MODULE*) m_parser->Parse();
                                            ret->SetFPID( aFootprintName );
                        }
                    } else {
                        std::string msg = StrPrintf( "Could not fetch footprint data.  Server said: %s", m_db->LastErrorString().c_str() );
                        THROW_IO_ERROR( msg );
                    }
                            delete results;
                } else {
                    std::string msg = StrPrintf( "Could not connect to MySQL.  Driver said: %s", m_db->LastErrorString().c_str() );
                    THROW_IO_ERROR( msg );
                }
                
                    return ret;    // this API function returns NULL for "not found", per spec.
            }
            
            
            bool MYODBC_PLUGIN::IsFootprintLibWritable( const wxString& aLibraryPath )
            {
                return true;
            }
            
            
            void MYODBC_PLUGIN::FootprintSave( const wxString& aLibraryPath,
                                               const MODULE* aFootprint, const PROPERTIES* aProperties )
            {
                MYSQL_ADAPTER::connection_parameters parameters;
                unpack_parameters(parameters, aProperties);
                m_db->Disconnect(); // Just in case!
                if(m_db->Connect(parameters)) {
                    wxStringOutputStream serial;
                    STREAM_OUTPUTFORMATTER formatter(serial);
                    SetOutputFormatter(&formatter);
                    // Need a copy to format
                    MODULE* module = new MODULE( *aFootprint );
                    module->SetTimeStamp( 0 );
                    module->SetParent( 0 );
                    module->SetOrientation( 0 );
                    
                    if( module->GetLayer() != F_Cu )
                        module->Flip( module->GetPosition() );
                    Format(module);
                    
                    // We need binding now.
                    std::string query = "INSERT INTO `footprints` (`name`,`data`) VALUES (?, ?)";
                    std::string name = module->GetFPID().GetFootprintName();
                    std::string data = (const char *)serial.GetString().c_str();
                    
                    if(m_db->Prepare(query)) {
                        // Following lines: Casting away const should be safe for input parameters.
                        if(m_db->Bind(1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR, name.length(), 0, (void *)name.c_str(), name.length(), 0)) {
                            if(m_db->Bind(2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR, data.length(), 0, (void *)data.c_str(), data.length(), 0)) {
                                if(m_db->Execute()) {
                                    m_db->FreeStmt();
                                    return;
                                }
                            }
                        }
                    }
                } 
                    std::string msg = StrPrintf( "Could not connect to MySQL.  Driver said: %s", m_db->LastErrorString().c_str() );
                    THROW_IO_ERROR( msg );
                    
            } 
            
            
            void MYODBC_PLUGIN::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
                                                 const PROPERTIES* aProperties )
            {
                std::string msg = StrPrintf( "Footprint delete is not implemented in the myodbc plugin." );
                THROW_IO_ERROR( msg );
            }
            
            
            void MYODBC_PLUGIN::FootprintLibCreate( const wxString& aLibraryPath, const PROPERTIES* aProperties )
            {
                PCB_IO::FootprintLibCreate( aLibraryPath, aProperties );
            }
            
            
            bool MYODBC_PLUGIN::FootprintLibDelete( const wxString& aLibraryPath, const PROPERTIES* aProperties )
            {
                return PCB_IO::FootprintLibDelete( aLibraryPath, aProperties );
            }
            
            
            void MYODBC_PLUGIN::FootprintLibOptions( PROPERTIES* aListToAppendTo ) const
            {
                // inherit options supported by all PLUGINs.
                PLUGIN::FootprintLibOptions( aListToAppendTo );
                
                (*aListToAppendTo)[ "mysql_server_name" ] = UTF8( _(
                    "Where is the mysql server?"
                ));
                
                (*aListToAppendTo)[ "mysql_server_port" ] = UTF8( _(
                    "On what port is the mysql server listening?  Leave blank for the default of 3306."
                ));
                
                (*aListToAppendTo)[ "mysql_user" ] = UTF8( _(
                    "How shall kicad identify itself to mysql?"
                ));
                
                (*aListToAppendTo)[ "mysql_password" ] = UTF8( _(
                    "What password should kicad use when connecting?"
                ));
                
                (*aListToAppendTo)[ "mysql_database" ] = UTF8( _(
                    "What database on the server contains footprints?"
                ));
                
                (*aListToAppendTo)[ "mysql_table" ] = UTF8( _(
                    "What table contains footprints?"
                ));
                
                (*aListToAppendTo)[ "mysql_name_column" ] = UTF8( _(
                    "What column contains footprint names?"
                ));
                
                (*aListToAppendTo)[ "mysql_data_column" ] = UTF8( _(
                    "What column contains footprint data?"
                ));
            }
            
            
            