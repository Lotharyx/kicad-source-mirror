/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_cmp_graphic_properties.cpp
// Purpose:
// Author:      jean-pierre Charras
// Modified by:
// Created:     12/02/2006 11:38:02
// RCS-ID:
// Copyright:   License GNU
// Licence:
/////////////////////////////////////////////////////////////////////////////

// Generated by DialogBlocks (unregistered), 12/02/2006 11:38:02


////@begin includes
////@end includes

#include "dialog_cmp_graphic_properties.h"

////@begin XPM images
////@end XPM images

/*!
 * WinEDA_bodygraphics_PropertiesFrame type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WinEDA_bodygraphics_PropertiesFrame, wxDialog )

/*!
 * WinEDA_bodygraphics_PropertiesFrame event table definition
 */

BEGIN_EVENT_TABLE( WinEDA_bodygraphics_PropertiesFrame, wxDialog )

////@begin WinEDA_bodygraphics_PropertiesFrame event table entries
    EVT_BUTTON( wxID_OK, WinEDA_bodygraphics_PropertiesFrame::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, WinEDA_bodygraphics_PropertiesFrame::OnCancelClick )

////@end WinEDA_bodygraphics_PropertiesFrame event table entries

END_EVENT_TABLE()

/*!
 * WinEDA_bodygraphics_PropertiesFrame constructors
 */

WinEDA_bodygraphics_PropertiesFrame::WinEDA_bodygraphics_PropertiesFrame( )
{
}

WinEDA_bodygraphics_PropertiesFrame::WinEDA_bodygraphics_PropertiesFrame( WinEDA_LibeditFrame* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
LibEDA_BaseStruct * CurrentItem = CurrentDrawItem;

	m_Parent = parent;
    Create(parent, id, caption, pos, size, style);

	/* Set the dialog items: */
	if ( CurrentItem )
	{
		if ( CurrentItem->m_Unit == 0 ) m_CommonUnit->SetValue(TRUE);
	}
	else if ( ! g_FlDrawSpecificUnit ) m_CommonUnit->SetValue(TRUE);
	if ( CurrentItem )
	{
		if ( CurrentItem->m_Convert == 0 ) m_CommonConvert->SetValue(TRUE);
	}
	else if ( !g_FlDrawSpecificConvert ) m_CommonConvert->SetValue(TRUE);

bool show_fill_option = FALSE;
int fill_option = 0;
	if( CurrentItem )
        switch(CurrentItem->Type())
 		{
        case COMPONENT_ARC_DRAW_TYPE:
            show_fill_option = TRUE;
            fill_option = ((LibDrawArc*)CurrentItem)->m_Fill;
			m_GraphicShapeWidthCtrl->SetValue(((LibDrawArc*)CurrentItem)->m_Width);

            break;

		case COMPONENT_CIRCLE_DRAW_TYPE:
            show_fill_option = TRUE;
            fill_option = ((LibDrawCircle*)CurrentItem)->m_Fill;
			m_GraphicShapeWidthCtrl->SetValue(((LibDrawCircle*)CurrentItem)->m_Width);
            break;

		case COMPONENT_RECT_DRAW_TYPE:
            show_fill_option = TRUE;
            fill_option = ((LibDrawSquare *)CurrentItem)->m_Fill;
			m_GraphicShapeWidthCtrl->SetValue(((LibDrawSquare*)CurrentItem)->m_Width);
            break;

        case  COMPONENT_POLYLINE_DRAW_TYPE:
            show_fill_option = TRUE;
            fill_option = ((LibDrawPolyline*)CurrentItem)->m_Fill;
			m_GraphicShapeWidthCtrl->SetValue(((LibDrawPolyline*)CurrentItem)->m_Width);
            break;

        default: break;
        }

    if ( show_fill_option ) m_Filled->SetSelection(fill_option);
    else m_Filled->Enable(false);
}

/*!
 * WinEDA_bodygraphics_PropertiesFrame creator
 */

bool WinEDA_bodygraphics_PropertiesFrame::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WinEDA_bodygraphics_PropertiesFrame member initialisation
    m_CommonUnit = NULL;
    m_CommonConvert = NULL;
    m_ShapeWidthBoxSizer = NULL;
    m_Filled = NULL;
    m_btClose = NULL;
////@end WinEDA_bodygraphics_PropertiesFrame member initialisation

////@begin WinEDA_bodygraphics_PropertiesFrame creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end WinEDA_bodygraphics_PropertiesFrame creation
    return true;
}

/*!
 * Control creation for WinEDA_bodygraphics_PropertiesFrame
 */

void WinEDA_bodygraphics_PropertiesFrame::CreateControls()
{
////@begin WinEDA_bodygraphics_PropertiesFrame content construction
    // Generated by DialogBlocks, 24/04/2009 14:19:31 (unregistered)

    WinEDA_bodygraphics_PropertiesFrame* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticBox* itemStaticBoxSizer3Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Options :"));
    wxStaticBoxSizer* itemStaticBoxSizer3 = new wxStaticBoxSizer(itemStaticBoxSizer3Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer3, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_CommonUnit = new wxCheckBox( itemDialog1, ID_CHECKBOX, _("Common to Units"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    m_CommonUnit->SetValue(false);
    itemStaticBoxSizer3->Add(m_CommonUnit, 0, wxALIGN_LEFT|wxALL, 5);

    m_CommonConvert = new wxCheckBox( itemDialog1, ID_CHECKBOX1, _("Common to convert"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    m_CommonConvert->SetValue(false);
    itemStaticBoxSizer3->Add(m_CommonConvert, 0, wxALIGN_LEFT|wxALL, 5);

    m_ShapeWidthBoxSizer = new wxBoxSizer(wxVERTICAL);
    itemStaticBoxSizer3->Add(m_ShapeWidthBoxSizer, 0, wxGROW|wxTOP|wxBOTTOM, 5);

    wxArrayString m_FilledStrings;
    m_FilledStrings.Add(_("Void"));
    m_FilledStrings.Add(_("Filled"));
    m_FilledStrings.Add(_("BgFilled"));
    m_Filled = new wxRadioBox( itemDialog1, ID_RADIOBOX, _("Fill:"), wxDefaultPosition, wxDefaultSize, m_FilledStrings, 1, wxRA_SPECIFY_COLS );
    m_Filled->SetSelection(0);
    itemStaticBoxSizer3->Add(m_Filled, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton9 = new wxButton( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton9->SetDefault();
    itemBoxSizer8->Add(itemButton9, 0, wxGROW|wxALL, 5);

    m_btClose = new wxButton( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(m_btClose, 0, wxGROW|wxALL, 5);

////@end WinEDA_bodygraphics_PropertiesFrame content construction
	m_btClose->SetFocus();
	m_GraphicShapeWidthCtrl = new WinEDA_ValueCtrl(this, _("Width"), 0,
		g_UnitMetric,m_ShapeWidthBoxSizer, EESCHEMA_INTERNAL_UNIT);

}

/*!
 * Should we show tooltips?
 */

bool WinEDA_bodygraphics_PropertiesFrame::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WinEDA_bodygraphics_PropertiesFrame::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WinEDA_bodygraphics_PropertiesFrame bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WinEDA_bodygraphics_PropertiesFrame bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WinEDA_bodygraphics_PropertiesFrame::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WinEDA_bodygraphics_PropertiesFrame icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WinEDA_bodygraphics_PropertiesFrame icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void WinEDA_bodygraphics_PropertiesFrame::OnOkClick( wxCommandEvent& event )
{
	bodygraphics_PropertiesAccept(event);
	Close();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void WinEDA_bodygraphics_PropertiesFrame::OnCancelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in WinEDA_bodygraphics_PropertiesFrame.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in WinEDA_bodygraphics_PropertiesFrame.
}


