#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
#  Copyright (C) 2013 Kicad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, you may find one here:
#  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
#  or you may search the http://www.gnu.org website for the version 2 license,
#  or you may write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA



# Download avhttp and install the headers, not actually compiled
#################################################
# include( download_avhttp )
# 
# if( MINGW AND NOT OPENSSL_ROOT_DIR )
#     # download, compile and install to scratch dir a recent OPENSSL library and headers
#     include( download_openssl )
# else()
#     find_package( OpenSSL REQUIRED )
#     #message( STATUS "OPENSSL_FOUND:${OPENSSL_FOUND}  OPENSSL_LIBRARIES:${OPENSSL_LIBRARIES}" )
# 
#     # FindOpenSSL.cmake does not set this var into cache, so is not globally visible,
#     # do it here incase some other link image needs these libraries
#     set( OPENSSL_LIBRARIES   "${OPENSSL_LIBRARIES}"   CACHE FILEPATH "OpenSSL link libraries" )
#     set( OPENSSL_INCLUDE_DIR "${OPENSSL_INCLUDE_DIR}" CACHE FILEPATH "OpenSSL include dir" )
# endif()
# 
# 
# # These are additions to any inherited from pcbnew dir:
# include_directories( . ${OPENSSL_INCLUDE_DIR} ${AVHTTP_INCLUDE_DIR} )
# 
# # Tell AVHTTP we have SSL.
# add_definitions( -DAVHTTP_ENABLE_OPENSSL )

# find_package( unixodbc REQUIRED )

# tone down the compiler warnings for avhttp header library:
set( CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wno-sign-compare -Wno-reorder -Wno-unused-variable -Wno-unused-function -Wno-strict-aliasing" )

set( MYODBC_PLUGIN_SRCS
    myodbc_plugin.cpp mysql_adapter.cpp odbc_adapter.cpp mysql_results.cpp
    )
    
include_directories( . )

add_library(myodbc_plugin STATIC ${MYODBC_PLUGIN_SRCS} )

# No, you don't get github without boost and openssl. Boost_LIBRARIES now moved up
# into CMakeLists.txt for pcbnew and cvpcb:
target_link_libraries( myodbc_plugin
        odbc
    )

if( MINGW )
    target_link_libraries( myodbc_plugin
        ${wxWidgets_LIBRARIES}
        ws2_32
        )
endif()

