/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <map>
#include <deque>

#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/range/adaptor/map.hpp>

#include <wx/event.h>

#include <view/view.h>

#include <tool/tool_base.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <tool/context_menu.h>
#include <tool/coroutine.h>

#include <wxPcbStruct.h>
#include <class_drawpanel_gal.h>

using boost::optional;
using namespace std;

/// Struct describing the current state of a TOOL
struct TOOL_MANAGER::TOOL_STATE
{
    /// The tool itself
	TOOL_BASE* theTool;
	
	/// Is the tool active or idle at the moment
	bool idle;

	/// Flag defining if the tool is waiting for any event
	bool pendingWait;

	/// Is there a context menu to be displayed
	bool pendingContextMenu;

	/// Context menu used by the tool
	CONTEXT_MENU* contextMenu;

	/// Defines when a context menu is opened
	CONTEXT_MENU_TRIGGER contextMenuTrigger;

	/// Coroutine launched upon an event trigger
	COROUTINE<int, TOOL_EVENT&>* cofunc;
	
	/// The event that triggered the coroutine
	TOOL_EVENT wakeupEvent;

	/// List of events that are triggering the coroutine
	TOOL_EVENT_LIST waitEvents;

	/// List of possible transitions (ie. association of events and functions that are executed
	/// upon the event reception
	std::vector<TRANSITION> transitions;

	bool operator==( const TOOL_MANAGER::TOOL_STATE& aRhs ) const
    {
	    return ( aRhs.theTool == this->theTool );
    }

	bool operator!=( const TOOL_MANAGER::TOOL_STATE& aRhs ) const
    {
	    return ( aRhs.theTool != this->theTool );
    }
};


TOOL_MANAGER::TOOL_MANAGER() : 
    m_actionMgr( this ), m_model( NULL ), m_view( NULL )
{
}


TOOL_MANAGER::~TOOL_MANAGER()
{
    std::map<TOOL_BASE*, TOOL_STATE*>::iterator it, it_end;

    for( it = m_toolState.begin(), it_end = m_toolState.end(); it != it_end; ++it )
    {
        delete it->second->cofunc;  // delete cofunction
        delete it->second;          // delete TOOL_STATE
        delete it->first;           // delete the tool itself
    }
}


void TOOL_MANAGER::RegisterTool( TOOL_BASE* aTool )
{
	TOOL_STATE* st = new TOOL_STATE;

	st->theTool = aTool;
	st->idle = true;
	st->pendingWait = false;
	st->pendingContextMenu = false;
	st->cofunc = NULL;
	st->contextMenuTrigger = CMENU_OFF;

	m_toolState[aTool] = st;
	m_toolNameIndex[aTool->GetName()] = st;
	m_toolIdIndex[aTool->GetId()] = st;
	
	aTool->m_toolMgr = this;
	
	if( aTool->GetType() == TOOL_Interactive )
		static_cast<TOOL_INTERACTIVE*>( aTool )->Reset();
}


bool TOOL_MANAGER::InvokeTool( TOOL_ID aToolId )
{
    TOOL_BASE* tool = FindTool( aToolId );

    if( tool && tool->GetType() == TOOL_Interactive )
        return invokeTool( tool );

    return false;
}


bool TOOL_MANAGER::InvokeTool( const std::string& aToolName )
{
    TOOL_BASE* tool = FindTool( aToolName );

    if( tool && tool->GetType() == TOOL_Interactive )
        return invokeTool( tool );

    return false;
}


bool TOOL_MANAGER::invokeTool( TOOL_BASE* aTool )
{
    wxASSERT( aTool != NULL );

    TOOL_EVENT evt( TC_Command, TA_Action, aTool->GetName() );
    ProcessEvent( evt );

    return true;
}


bool TOOL_MANAGER::runTool( TOOL_ID aToolId )
{
    TOOL_BASE* tool = FindTool( aToolId );

    if( tool && tool->GetType() == TOOL_Interactive )
        return runTool( tool );

    return false;
}


bool TOOL_MANAGER::runTool( const std::string& aToolName )
{
    TOOL_BASE* tool = FindTool( aToolName );

    if( tool && tool->GetType() == TOOL_Interactive )
        return runTool( tool );

    return false;
}


bool TOOL_MANAGER::runTool( TOOL_BASE* aTool )
{
    wxASSERT( aTool != NULL );

    if( !isRegistered( aTool ) )
        return false;

    TOOL_STATE* state = m_toolState[aTool];

    // If the tool is already active, do not invoke it again
    if( state->idle == false )
        return false;
    state->idle = false;

    static_cast<TOOL_INTERACTIVE*>( aTool )->Reset();

    // Add the tool on the front of the processing queue (it gets events first)
    m_activeTools.push_front( aTool->GetId() );

    return true;
}


TOOL_BASE* TOOL_MANAGER::FindTool( int aId ) const
{
    std::map<TOOL_ID, TOOL_STATE*>::const_iterator it = m_toolIdIndex.find( aId );

    if( it != m_toolIdIndex.end() )
        return it->second->theTool;

    return NULL;
}


TOOL_BASE* TOOL_MANAGER::FindTool( const std::string& aName ) const
{
    std::map<std::string, TOOL_STATE*>::const_iterator it = m_toolNameIndex.find( aName );

    if( it != m_toolNameIndex.end() )
        return it->second->theTool;

    return NULL;
}


void TOOL_MANAGER::ScheduleNextState( TOOL_BASE* aTool, TOOL_STATE_FUNC& aHandler,
                                      const TOOL_EVENT_LIST& aConditions )
{
	TOOL_STATE* st = m_toolState[aTool];
	st->transitions.push_back( TRANSITION( aConditions, aHandler ) );
}


optional<TOOL_EVENT> TOOL_MANAGER::ScheduleWait( TOOL_BASE* aTool,
                                                 const TOOL_EVENT_LIST& aConditions )
{
	TOOL_STATE* st = m_toolState[aTool];
	
	st->pendingWait = true;
	st->waitEvents = aConditions;
	st->cofunc->Yield();

	return st->wakeupEvent;
}


void TOOL_MANAGER::dispatchInternal( TOOL_EVENT& aEvent )
{
	// iterate over all registered tools
    BOOST_FOREACH( TOOL_ID toolId, m_activeTools )
    {
        TOOL_STATE* st = m_toolIdIndex[toolId];

		// the tool state handler is waiting for events (i.e. called Wait() method)
		if( st->pendingWait )
		{
			if( st->waitEvents.Matches( aEvent ) )
			{
			    // By default, already processed events are not passed further
			    m_passEvent = false;

				// got matching event? clear wait list and wake up the coroutine
				st->wakeupEvent = aEvent;
				st->pendingWait = false;
				st->waitEvents.clear();
				if( st->cofunc && !st->cofunc->Resume() )
				{
				    // The couroutine has finished
				    finishTool( st );
				}

				// If the tool did not request to propagate
				// the event to other tools, we should stop it now
				if( !m_passEvent )
				    break;
			}
		}
    }

    BOOST_FOREACH( TOOL_STATE* st, m_toolState | boost::adaptors::map_values )
    {
		if( !st->pendingWait )
		{
			// no state handler in progress - check if there are any transitions (defined by
			// Go() method that match the event.
			if( st->transitions.size() )
			{
				BOOST_FOREACH( TRANSITION tr, st->transitions )
				{
					if( tr.first.Matches( aEvent ) )
					{
						st->transitions.clear();

						if( !st->cofunc )
							st->cofunc = new COROUTINE<int, TOOL_EVENT&>( tr.second );
						else 
							st->cofunc->SetEntry( tr.second );
							
						// got match? Run the handler.
						st->cofunc->Call( aEvent );
						
						if( !st->cofunc->Running() )
						{
						    finishTool( st );
						}
					}
				}
			}
		}
	}	
}


bool TOOL_MANAGER::dispatchActivation( TOOL_EVENT& aEvent )
{
    BOOST_FOREACH( TOOL_STATE* st, m_toolState | boost::adaptors::map_values )
    {
        if( st->theTool->GetName() == aEvent.m_commandStr )
        {
            runTool( st->theTool );
            return true;
        }
    }

    return false;
}


void TOOL_MANAGER::finishTool( TOOL_STATE* aState )
{
    // Find the tool to be deactivated
    std::deque<TOOL_ID>::iterator it, it_end;
    for( it = m_activeTools.begin(), it_end = m_activeTools.end(); it != it_end; ++it )
    {
        if( aState == m_toolIdIndex[*it] )
            break;
    }

    if( it != m_activeTools.end() )
        m_activeTools.erase( it );
    else
        wxLogWarning( wxT( "Tried to finish not active tool" ) );

    aState->idle = true;
    delete aState->cofunc;
    aState->cofunc = NULL;
}


bool TOOL_MANAGER::ProcessEvent( TOOL_EVENT& aEvent )
{
//	wxLogDebug( "event: %s", aEvent.Format().c_str() );

	if( aEvent.Action() == TA_KeyUp )
	{
	    // Check if there is a hotkey associated
	    if( m_actionMgr.RunHotKey( aEvent.Modifier() | aEvent.KeyCode() ) )
	        return false;   // hotkey event was handled so it does not go any further
	} else if( aEvent.Category() == TC_Command )        // it may be a tool activation event
	{
	    dispatchActivation( aEvent );
	}

	dispatchInternal( aEvent );

    BOOST_FOREACH( TOOL_ID toolId, m_activeTools )
    {
        TOOL_STATE* st = m_toolIdIndex[toolId];

		if( st->contextMenuTrigger != CMENU_OFF )
		{
			if( st->contextMenuTrigger == CMENU_BUTTON && !aEvent.IsClick( MB_Right ) )
				break;

			st->pendingWait = true;
			st->waitEvents = TOOL_EVENT( TC_Any, TA_Any );

			if( st->contextMenuTrigger == CMENU_NOW )
				st->contextMenuTrigger = CMENU_OFF;

            boost::scoped_ptr<CONTEXT_MENU> menu( new CONTEXT_MENU( *st->contextMenu ) );
            GetEditFrame()->PopupMenu( menu->GetMenu() );

			TOOL_EVENT evt( TC_Command, TA_ContextMenuChoice );
			dispatchInternal( evt );

			break;
		}
	}

	if( m_view->IsDirty() )
	{
		PCB_EDIT_FRAME* f = static_cast<PCB_EDIT_FRAME*>( GetEditFrame() );
		f->GetGalCanvas()->Refresh(); // fixme: ugly hack, provide a method in TOOL_DISPATCHER.
	}

	return false;
}


void TOOL_MANAGER::ScheduleContextMenu( TOOL_BASE* aTool, CONTEXT_MENU* aMenu,
                                        CONTEXT_MENU_TRIGGER aTrigger )
{
	TOOL_STATE* st = m_toolState[aTool];

	st->contextMenu = aMenu;
	st->contextMenuTrigger = aTrigger;
	
	if( aTrigger == CMENU_NOW )
		st->cofunc->Yield();	
}


TOOL_ID TOOL_MANAGER::MakeToolId( const std::string& aToolName )
{
	static int currentId;
	return currentId++;
}


void TOOL_MANAGER::SetEnvironment( EDA_ITEM* aModel, KiGfx::VIEW* aView,
                                   KiGfx::VIEW_CONTROLS* aViewControls, wxWindow* aFrame )
{
	m_model = aModel;
	m_view = aView;
	m_viewControls = aViewControls;
	m_editFrame = aFrame;

	// Reset state of the registered tools
	BOOST_FOREACH( TOOL_ID toolId, m_activeTools )
	{
	    TOOL_BASE* tool = m_toolIdIndex[toolId]->theTool;

	    if( tool->GetType() == TOOL_Interactive )
	        static_cast<TOOL_INTERACTIVE*>( tool )->Reset();
	}
}


bool TOOL_MANAGER::isActive( TOOL_BASE* aTool )
{
    if( !isRegistered( aTool ) )
        return false;

    return !m_toolState[aTool]->idle;
}