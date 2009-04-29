/////////////////////////////////////////////////////////////////////////////
// Name:        annotate_dialog.cpp
// Purpose:
// Author:      jean-pierre Charras
// Modified by:
// Created:     16/04/2008 17:50:59
// RCS-ID:
// Copyright:   License GNU
// Licence:
/////////////////////////////////////////////////////////////////////////////

// Generated by DialogBlocks (unregistered), 16/04/2008 17:50:59

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "annotate_dialog.h"
#endif

////@begin includes
////@end includes

#include "fctsys.h"
#include "bitmaps.h"
#include "common.h"
#include "program.h"

#include "annotate_dialog.h"

extern void AnnotateComponents( WinEDA_SchematicFrame* parent,
                                bool annotateSchematic,
                                int sortOption,
                                bool resetAnnotation,
                                bool repairsTimestamps );

////@begin XPM images
////@end XPM images


/*!
 * WinEDA_AnnotateFrame type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WinEDA_AnnotateFrame, wxDialog )


/*!
 * WinEDA_AnnotateFrame event table definition
 */

BEGIN_EVENT_TABLE( WinEDA_AnnotateFrame, wxDialog )

////@begin WinEDA_AnnotateFrame event table entries
    EVT_BUTTON( wxID_CANCEL, WinEDA_AnnotateFrame::OnCancelClick )

    EVT_BUTTON( ID_CLEAR_ANNOTATION_CMP, WinEDA_AnnotateFrame::OnClearAnnotationCmpClick )

    EVT_BUTTON( wxID_APPLY, WinEDA_AnnotateFrame::OnApplyClick )

////@end WinEDA_AnnotateFrame event table entries

END_EVENT_TABLE()


/*!
 * WinEDA_AnnotateFrame constructors
 */

WinEDA_AnnotateFrame::WinEDA_AnnotateFrame()
{
    Init();
}

WinEDA_AnnotateFrame::WinEDA_AnnotateFrame( WinEDA_SchematicFrame* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    m_Parent = parent;
    Init();
    Create(parent, id, caption, pos, size, style);
}


/*!
 * WinEDA_AnnotateFrame creator
 */

bool WinEDA_AnnotateFrame::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WinEDA_AnnotateFrame creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end WinEDA_AnnotateFrame creation
    return true;
}


/*!
 * WinEDA_AnnotateFrame destructor
 */

WinEDA_AnnotateFrame::~WinEDA_AnnotateFrame()
{
////@begin WinEDA_AnnotateFrame destruction
////@end WinEDA_AnnotateFrame destruction
}


/*!
 * Member initialisation
 */

void WinEDA_AnnotateFrame::Init()
{
////@begin WinEDA_AnnotateFrame member initialisation
    m_rbEntireSchematic = NULL;
    m_rbKeepAnnotation = NULL;
    m_rbResetAnnotation = NULL;
    m_rbSortBy_X_Position = NULL;
    m_rbSortBy_Y_Position = NULL;
    rbSortByValue = NULL;
    sizerDialogButtons = NULL;
    m_btnClose = NULL;
    m_btnClear = NULL;
    m_btnApply = NULL;
////@end WinEDA_AnnotateFrame member initialisation
}


/*!
 * Control creation for WinEDA_AnnotateFrame
 */

void WinEDA_AnnotateFrame::CreateControls()
{
////@begin WinEDA_AnnotateFrame content construction
    // Generated by DialogBlocks, 29/04/2009 13:38:10 (unregistered)

    WinEDA_AnnotateFrame* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText4 = new wxStaticText( itemDialog1, wxID_STATIC, _("Scope"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText4->SetFont(wxFont(int(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize()*1.2), wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFamily(), wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetStyle(), wxBOLD, false, wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName()));
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer5, 0, wxGROW|wxLEFT, 25);

    m_rbEntireSchematic = new wxRadioButton( itemDialog1, ID_ENTIRE_SCHEMATIC, _("Use the &entire schematic"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    m_rbEntireSchematic->SetValue(true);
    itemBoxSizer5->Add(m_rbEntireSchematic, 0, wxGROW|wxALL, 5);

    wxRadioButton* itemRadioButton7 = new wxRadioButton( itemDialog1, ID_CURRENT_PAGE, _("Use the current &page only"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton7->SetValue(false);
    itemBoxSizer5->Add(itemRadioButton7, 0, wxGROW|wxALL, 5);

    wxStaticLine* itemStaticLine8 = new wxStaticLine( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer5->Add(itemStaticLine8, 0, wxGROW|wxALL, 5);

    m_rbKeepAnnotation = new wxRadioButton( itemDialog1, ID_KEEP_ANNOTATION, _("&Keep existing annotation"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    m_rbKeepAnnotation->SetValue(true);
    itemBoxSizer5->Add(m_rbKeepAnnotation, 0, wxGROW|wxALL, 5);

    m_rbResetAnnotation = new wxRadioButton( itemDialog1, ID_RESET_ANNOTATION, _("&Reset existing annotation"), wxDefaultPosition, wxDefaultSize, 0 );
    m_rbResetAnnotation->SetValue(false);
    itemBoxSizer5->Add(m_rbResetAnnotation, 0, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText11 = new wxStaticText( itemDialog1, wxID_STATIC, _("Order"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText11->SetFont(wxFont(int(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize()*1.2), wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFamily(), wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetStyle(), wxBOLD, false, wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName()));
    itemBoxSizer3->Add(itemStaticText11, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer12, 0, wxGROW|wxLEFT, 25);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer12->Add(itemBoxSizer13, 0, wxGROW, 5);

    wxStaticBitmap* itemStaticBitmap14 = new wxStaticBitmap( itemDialog1, wxID_STATIC, itemDialog1->GetBitmapResource(wxT("annotate_down_right_xpm")), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemStaticBitmap14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_rbSortBy_X_Position = new wxRadioButton( itemDialog1, ID_SORT_BY_X_POSITION, _("Sort components by &X position"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    m_rbSortBy_X_Position->SetValue(true);
    itemBoxSizer13->Add(m_rbSortBy_X_Position, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer12->Add(itemBoxSizer16, 0, wxGROW, 5);

    wxStaticBitmap* itemStaticBitmap17 = new wxStaticBitmap( itemDialog1, wxID_STATIC, itemDialog1->GetBitmapResource(wxT("annotate_right_down_xpm")), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add(itemStaticBitmap17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_rbSortBy_Y_Position = new wxRadioButton( itemDialog1, ID_SORT_BY_Y_POSITION, _("Sort components by &Y position"), wxDefaultPosition, wxDefaultSize, 0 );
    m_rbSortBy_Y_Position->SetValue(false);
    itemBoxSizer16->Add(m_rbSortBy_Y_Position, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer19 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer12->Add(itemBoxSizer19, 0, wxGROW, 5);

    wxStaticBitmap* itemStaticBitmap20 = new wxStaticBitmap( itemDialog1, wxID_STATIC, itemDialog1->GetBitmapResource(wxT("add_text_xpm")), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(itemStaticBitmap20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    rbSortByValue = new wxRadioButton( itemDialog1, ID_SORT_BY_VALUE, _("Sort components by &value"), wxDefaultPosition, wxDefaultSize, 0 );
    rbSortByValue->SetValue(false);
    itemBoxSizer19->Add(rbSortByValue, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    sizerDialogButtons = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(sizerDialogButtons, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    m_btnClose = new wxButton( itemDialog1, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    m_btnClose->SetDefault();
    sizerDialogButtons->Add(m_btnClose, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_btnClear = new wxButton( itemDialog1, ID_CLEAR_ANNOTATION_CMP, _("Clear Annotation"), wxDefaultPosition, wxDefaultSize, 0 );
    sizerDialogButtons->Add(m_btnClear, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_btnApply = new wxButton( itemDialog1, wxID_APPLY, _("Annotation"), wxDefaultPosition, wxDefaultSize, 0 );
    m_btnApply->SetDefault();
    sizerDialogButtons->Add(m_btnApply, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end WinEDA_AnnotateFrame content construction

	m_btnClose->SetFocus();	// needed to closes dialog by escape key
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_APPLY
 */

void WinEDA_AnnotateFrame::OnApplyClick( wxCommandEvent& event )
{
    int response;
    wxString message;

    if( GetResetItems() )
        message = _( "Clear and annotate all of the components " );
    else
        message = _( "Annotate only the unannotated components " );
    if( GetLevel() )
        message += _( "on the entire schematic?" );
    else
        message += _( "on the current sheet?" );

    message += _( "\n\nThis operation will change the current annotation and " \
                  "cannot be undone." );
    response = wxMessageBox( message, wxT( "" ),
                             wxICON_EXCLAMATION | wxOK | wxCANCEL );
    if (response == wxCANCEL)
        return;
    AnnotateComponents( m_Parent, GetLevel(), GetSortOrder(),
                        GetResetItems() , true );
    m_btnClear->Enable();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DEANNOTATE_CMP
 */

void WinEDA_AnnotateFrame::OnClearAnnotationCmpClick( wxCommandEvent& event )
{
    int response;

    wxString message = _( "Clear the existing annotation for " );
    if( GetLevel() )
        message += _( "the entire schematic?" );
    else
        message += _( "the current sheet?" );

    message += _( "\n\nThis operation will clear the existing annotation " \
                  "and cannot be undone." );
    response = wxMessageBox( message, wxT( "" ),
                             wxICON_EXCLAMATION | wxOK | wxCANCEL );
    if (response == wxCANCEL)
        return;
    m_Parent->DeleteAnnotation( GetLevel() ? false : true, true );
    m_btnClear->Enable(false);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void WinEDA_AnnotateFrame::OnCancelClick( wxCommandEvent& event )
{
    if( IsModal() )
        EndModal( wxID_CANCEL );
    else
    {
        SetReturnCode( wxID_CANCEL );
        this->Show( false );
    }
}


bool WinEDA_AnnotateFrame::GetLevel( void )
{
    return m_rbEntireSchematic->GetValue();
}

bool WinEDA_AnnotateFrame::GetResetItems( void )
{
    return m_rbResetAnnotation->GetValue();
}

int WinEDA_AnnotateFrame::GetSortOrder( void )
/**
 * @return 0 if annotation by X position,
 *         1 if annotation by Y position
 *         2 if annotation by value
 */
{
    if ( m_rbSortBy_X_Position->GetValue() )
        return 0;
    if ( m_rbSortBy_Y_Position->GetValue() )
        return 1;
    return 2;
}



/*!
 * Should we show tooltips?
 */

bool WinEDA_AnnotateFrame::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WinEDA_AnnotateFrame::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
    if (name == wxT("annotate_down_right_xpm"))
    {
        wxBitmap bitmap(annotate_down_right_xpm);
        return bitmap;
    }
    else if (name == wxT("annotate_right_down_xpm"))
    {
        wxBitmap bitmap(annotate_right_down_xpm);
        return bitmap;
    }
    else if (name == wxT("add_text_xpm"))
    {
        wxBitmap bitmap(add_text_xpm);
        return bitmap;
    }
    return wxNullBitmap;
}

/*!
 * Get icon resources
 */

wxIcon WinEDA_AnnotateFrame::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WinEDA_AnnotateFrame icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WinEDA_AnnotateFrame icon retrieval
}
