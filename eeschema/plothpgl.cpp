/////////////////////////////////////////////////////////////////////////////

// Name:        plothpgl.cpp
// Purpose:
// Author:      jean-pierre Charras
// Modified by:
// Created:     04/02/2006 16:54:19
// RCS-ID:
// Copyright:   License GNU
// Licence:
/////////////////////////////////////////////////////////////////////////////

// Generated by DialogBlocks (unregistered), 04/02/2006 16:54:19

#if defined (__GNUG__) && !defined (NO_GCC_PRAGMA)
#pragma implementation "plothpgl.h"
#endif

////@begin includes
////@end includes

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "worksheet.h"
#include "plot_common.h"

#include "protos.h"

/* coeff de conversion dim en 1 mil -> dim en unite HPGL: */
#define SCALE_HPGL 1.02041

#include "plothpgl.h"

////@begin XPM images
////@end XPM images

extern void Move_Plume( wxPoint pos, int plume );
extern void Plume( int plume );

/* Variables locales : */
FILE*         PlotOutput; /* exportee dans printps.cc */
static double Scale_X = 1;
static double Scale_Y = 1;
int           HPGL_SizeSelect;

enum PageFormatReq {
    PAGE_DEFAULT = 0,
    PAGE_SIZE_A4,
    PAGE_SIZE_A3,
    PAGE_SIZE_A2,
    PAGE_SIZE_A1,
    PAGE_SIZE_A0,
    PAGE_SIZE_A,
    PAGE_SIZE_B,
    PAGE_SIZE_C,
    PAGE_SIZE_D,
    PAGE_SIZE_E
};

static Ki_PageDescr* Plot_sheet_list[] =
{
    NULL,
    &g_Sheet_A4,
    &g_Sheet_A3,
    &g_Sheet_A2,
    &g_Sheet_A1,
    &g_Sheet_A0,
    &g_Sheet_A,
    &g_Sheet_B,
    &g_Sheet_C,
    &g_Sheet_D,
    &g_Sheet_E,
    &g_Sheet_GERBER,
    &g_Sheet_user
};

/* Routines Locales */


/**************************************************************/
void WinEDA_SchematicFrame::ToPlot_HPGL( wxCommandEvent& event )
/**************************************************************/
{
    WinEDA_PlotHPGLFrame* HPGL_frame = new WinEDA_PlotHPGLFrame( this );

    HPGL_frame->ShowModal();
    HPGL_frame->Destroy();
}


/*!
 * WinEDA_PlotHPGLFrame type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WinEDA_PlotHPGLFrame, wxDialog )

/*!
 * WinEDA_PlotHPGLFrame event table definition
 */

BEGIN_EVENT_TABLE( WinEDA_PlotHPGLFrame, wxDialog )

////@begin WinEDA_PlotHPGLFrame event table entries
    EVT_RADIOBOX( ID_RADIOBOX, WinEDA_PlotHPGLFrame::OnRadioboxSelected )

    EVT_SPINCTRL( ID_PEN_WIDTH_UPDATED, WinEDA_PlotHPGLFrame::OnPenWidthUpdatedUpdated )

    EVT_SPINCTRL( ID_PEN_SPEED_UPDATED, WinEDA_PlotHPGLFrame::OnPenSpeedUpdatedUpdated )

    EVT_SPINCTRL( ID_PEN_NUMBER_UPDATED, WinEDA_PlotHPGLFrame::OnPenNumberUpdatedUpdated )

    EVT_BUTTON( ID_PLOT_HPGL_CURRENT_EXECUTE, WinEDA_PlotHPGLFrame::OnPlotHpglCurrentExecuteClick )

    EVT_BUTTON( ID_PLOT_HPGL_ALL_EXECUTE, WinEDA_PlotHPGLFrame::OnPlotHpglAllExecuteClick )

    EVT_BUTTON( wxID_CANCEL, WinEDA_PlotHPGLFrame::OnCancelClick )

    EVT_BUTTON( ID_PLOT_ACCEPT_OFFSET, WinEDA_PlotHPGLFrame::OnPlotAcceptOffsetClick )

////@end WinEDA_PlotHPGLFrame event table entries

END_EVENT_TABLE()

/*!
 * WinEDA_PlotHPGLFrame constructors
 */

WinEDA_PlotHPGLFrame::WinEDA_PlotHPGLFrame()
{
}


WinEDA_PlotHPGLFrame::WinEDA_PlotHPGLFrame( WinEDA_DrawFrame* parent,
                                            wxWindowID        id,
                                            const wxString&   caption,
                                            const wxPoint&    pos,
                                            const wxSize&     size,
                                            long              style )
{
    m_Parent = parent;
    Create( parent, id, caption, pos, size, style );
    SetPageOffsetValue();
}


/*!
 * WinEDA_PlotHPGLFrame creator
 */

bool WinEDA_PlotHPGLFrame::Create( wxWindow*       parent,
                                   wxWindowID      id,
                                   const wxString& caption,
                                   const wxPoint&  pos,
                                   const wxSize&   size,
                                   long            style )
{
////@begin WinEDA_PlotHPGLFrame member initialisation
    m_SizeOption = NULL;
    m_ButtPenWidth = NULL;
    m_ButtPenSpeed = NULL;
    m_ButtPenNum = NULL;
    m_PlotOrgPosition_X = NULL;
    m_PlotOrgPosition_Y = NULL;
    m_btClose = NULL;
    m_MsgBox = NULL;
////@end WinEDA_PlotHPGLFrame member initialisation

////@begin WinEDA_PlotHPGLFrame creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end WinEDA_PlotHPGLFrame creation
    return true;
}


/*!
 * Control creation for WinEDA_PlotHPGLFrame
 */

void WinEDA_PlotHPGLFrame::CreateControls()
{
////@begin WinEDA_PlotHPGLFrame content construction
    // Generated by DialogBlocks, 24/04/2009 14:24:58 (unregistered)

    WinEDA_PlotHPGLFrame* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer4, 0, wxGROW|wxALL, 5);

    wxArrayString m_SizeOptionStrings;
    m_SizeOptionStrings.Add(_("Sheet Size"));
    m_SizeOptionStrings.Add(_("Page Size A4"));
    m_SizeOptionStrings.Add(_("Page Size A3"));
    m_SizeOptionStrings.Add(_("Page Size A2"));
    m_SizeOptionStrings.Add(_("Page Size A1"));
    m_SizeOptionStrings.Add(_("Page Size A0"));
    m_SizeOptionStrings.Add(_("Page Size A"));
    m_SizeOptionStrings.Add(_("Page Size B"));
    m_SizeOptionStrings.Add(_("Page Size C"));
    m_SizeOptionStrings.Add(_("Page Size D"));
    m_SizeOptionStrings.Add(_("Page Size E"));
    m_SizeOption = new wxRadioBox( itemDialog1, ID_RADIOBOX, _("Plot page size:"), wxDefaultPosition, wxDefaultSize, m_SizeOptionStrings, 1, wxRA_SPECIFY_COLS );
    m_SizeOption->SetSelection(0);
    itemBoxSizer4->Add(m_SizeOption, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer6, 0, wxALIGN_TOP|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer7Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Pen control:"));
    wxStaticBoxSizer* itemStaticBoxSizer7 = new wxStaticBoxSizer(itemStaticBoxSizer7Static, wxVERTICAL);
    itemBoxSizer6->Add(itemStaticBoxSizer7, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxStaticText* itemStaticText8 = new wxStaticText( itemDialog1, wxID_STATIC, _("Pen Width ( mils )"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer7->Add(itemStaticText8, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_ButtPenWidth = new wxSpinCtrl( itemDialog1, ID_PEN_WIDTH_UPDATED, _T("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 1, 100, 1 );
    itemStaticBoxSizer7->Add(m_ButtPenWidth, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText10 = new wxStaticText( itemDialog1, wxID_STATIC, _("Pen Speed ( cm/s )"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer7->Add(itemStaticText10, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_ButtPenSpeed = new wxSpinCtrl( itemDialog1, ID_PEN_SPEED_UPDATED, _T("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 1 );
    itemStaticBoxSizer7->Add(m_ButtPenSpeed, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText12 = new wxStaticText( itemDialog1, wxID_STATIC, _("Pen Number"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer7->Add(itemStaticText12, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_ButtPenNum = new wxSpinCtrl( itemDialog1, ID_PEN_NUMBER_UPDATED, _T("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 8, 1 );
    itemStaticBoxSizer7->Add(m_ButtPenNum, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticBox* itemStaticBoxSizer14Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Page offset:"));
    wxStaticBoxSizer* itemStaticBoxSizer14 = new wxStaticBoxSizer(itemStaticBoxSizer14Static, wxVERTICAL);
    itemBoxSizer6->Add(itemStaticBoxSizer14, 0, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* itemStaticText15 = new wxStaticText( itemDialog1, wxID_STATIC, _("Plot Offset X"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer14->Add(itemStaticText15, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_PlotOrgPosition_X = new wxTextCtrl( itemDialog1, ID_TEXTCTRL1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer14->Add(m_PlotOrgPosition_X, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText17 = new wxStaticText( itemDialog1, wxID_STATIC, _("Plot Offset Y"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer14->Add(itemStaticText17, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_PlotOrgPosition_Y = new wxTextCtrl( itemDialog1, ID_TEXTCTRL2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer14->Add(m_PlotOrgPosition_Y, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemBoxSizer3->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer20 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer20, 0, wxALIGN_TOP|wxLEFT|wxTOP|wxBOTTOM, 5);

    wxButton* itemButton21 = new wxButton( itemDialog1, ID_PLOT_HPGL_CURRENT_EXECUTE, _("&Plot Page"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton21->SetDefault();
    itemBoxSizer20->Add(itemButton21, 0, wxGROW|wxALL, 5);

    wxButton* itemButton22 = new wxButton( itemDialog1, ID_PLOT_HPGL_ALL_EXECUTE, _("Plot A&LL"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer20->Add(itemButton22, 0, wxGROW|wxALL, 5);

    m_btClose = new wxButton( itemDialog1, wxID_CANCEL, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer20->Add(m_btClose, 0, wxGROW|wxALL, 5);

    itemBoxSizer20->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxButton* itemButton25 = new wxButton( itemDialog1, ID_PLOT_ACCEPT_OFFSET, _("&Accept Offset"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer20->Add(itemButton25, 0, wxGROW|wxALL, 5);

    m_MsgBox = new wxTextCtrl( itemDialog1, ID_TEXTCTRL, _T(""), wxDefaultPosition, wxSize(-1, 110), wxTE_MULTILINE );
    itemBoxSizer2->Add(m_MsgBox, 0, wxGROW|wxALL, 5);

    // Set validators
    m_SizeOption->SetValidator( wxGenericValidator(& HPGL_SizeSelect) );
    m_ButtPenWidth->SetValidator( wxGenericValidator(& g_HPGL_Pen_Descr.m_Pen_Diam) );
    m_ButtPenSpeed->SetValidator( wxGenericValidator(& g_HPGL_Pen_Descr.m_Pen_Speed) );
    m_ButtPenNum->SetValidator( wxGenericValidator(& g_HPGL_Pen_Descr.m_Pen_Num) );
////@end WinEDA_PlotHPGLFrame content construction
    SetFocus(); // Make ESC key working
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PLOT_HPGL_CURRENT_EXECUTE
 */

void WinEDA_PlotHPGLFrame::OnPlotHpglCurrentExecuteClick( wxCommandEvent& event )
{
    HPGL_Plot( event );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PLOT_HPGL_ALL_EXECUTE
 */

void WinEDA_PlotHPGLFrame::OnPlotHpglAllExecuteClick( wxCommandEvent& event )
{
    HPGL_Plot( event );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void WinEDA_PlotHPGLFrame::OnCancelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in WinEDA_PlotHPGLFrame.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in WinEDA_PlotHPGLFrame.
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PLOT_ACCEPT_OFFSET
 */

void WinEDA_PlotHPGLFrame::OnPlotAcceptOffsetClick( wxCommandEvent& event )
{
    AcceptPlotOffset( event );
}


/*!
 * Should we show tooltips?
 */

bool WinEDA_PlotHPGLFrame::ShowToolTips()
{
    return true;
}


/*!
 * Get bitmap resources
 */

wxBitmap WinEDA_PlotHPGLFrame::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WinEDA_PlotHPGLFrame bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WinEDA_PlotHPGLFrame bitmap retrieval
}


/*!
 * Get icon resources
 */

wxIcon WinEDA_PlotHPGLFrame::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WinEDA_PlotHPGLFrame icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WinEDA_PlotHPGLFrame icon retrieval
}


/***************************************************/
void WinEDA_PlotHPGLFrame::SetPageOffsetValue()
/***************************************************/
{
    wxString msg;

    if( HPGL_SizeSelect != PAGE_DEFAULT )
    {
        msg = ReturnStringFromValue( g_UnitMetric,
                                     Plot_sheet_list[HPGL_SizeSelect]->m_Offset.x,
                                     EESCHEMA_INTERNAL_UNIT );
        m_PlotOrgPosition_X->SetValue( msg );
        msg = ReturnStringFromValue( g_UnitMetric,
                                     Plot_sheet_list[HPGL_SizeSelect]->m_Offset.y,
                                     EESCHEMA_INTERNAL_UNIT );
        m_PlotOrgPosition_Y->SetValue( msg );

        m_PlotOrgPosition_X->Enable( TRUE );
        m_PlotOrgPosition_Y->Enable( TRUE );
    }
    else
    {
        m_PlotOrgPosition_X->Enable( FALSE );
        m_PlotOrgPosition_Y->Enable( FALSE );
    }
}


/*****************************************************************/
void WinEDA_PlotHPGLFrame::AcceptPlotOffset( wxCommandEvent& event )
/*****************************************************************/
{
    int ii = m_SizeOption->GetSelection();

    if( ii <= 0 )
        HPGL_SizeSelect = 0;
    else
        HPGL_SizeSelect = ii;

    if( HPGL_SizeSelect != PAGE_DEFAULT )
    {
        wxString msg = m_PlotOrgPosition_X->GetValue();
        Plot_sheet_list[HPGL_SizeSelect]->m_Offset.x =
            ReturnValueFromString( g_UnitMetric, msg, EESCHEMA_INTERNAL_UNIT );
        msg = m_PlotOrgPosition_Y->GetValue();
        Plot_sheet_list[HPGL_SizeSelect]->m_Offset.y =
            ReturnValueFromString( g_UnitMetric, msg, EESCHEMA_INTERNAL_UNIT );
    }
}


/************************************************************/
void WinEDA_PlotHPGLFrame::SetPenWidth( wxSpinEvent& event )
/************************************************************/
{
    g_HPGL_Pen_Descr.m_Pen_Diam = m_ButtPenWidth->GetValue();
    if( g_HPGL_Pen_Descr.m_Pen_Diam > 100 )
        g_HPGL_Pen_Descr.m_Pen_Diam = 100;
    if( g_HPGL_Pen_Descr.m_Pen_Diam < 1 )
        g_HPGL_Pen_Descr.m_Pen_Diam = 1;
}


/*********************************************************/
void WinEDA_PlotHPGLFrame::SetPenSpeed( wxSpinEvent& event )
/*********************************************************/
{
    g_HPGL_Pen_Descr.m_Pen_Speed = m_ButtPenSpeed->GetValue();
    if( g_HPGL_Pen_Descr.m_Pen_Speed > 40 )
        g_HPGL_Pen_Descr.m_Pen_Speed = 40;
    if( g_HPGL_Pen_Descr.m_Pen_Speed < 1 )
        g_HPGL_Pen_Descr.m_Pen_Speed = 1;
}


/*******************************************************/
void WinEDA_PlotHPGLFrame::SetPenNum( wxSpinEvent& event )
/*******************************************************/
{
    g_HPGL_Pen_Descr.m_Pen_Num = m_ButtPenNum->GetValue();
    if( g_HPGL_Pen_Descr.m_Pen_Num > 8 )
        g_HPGL_Pen_Descr.m_Pen_Num = 8;
    if( g_HPGL_Pen_Descr.m_Pen_Num < 1 )
        g_HPGL_Pen_Descr.m_Pen_Num = 1;
}


/***********************************************************/
void WinEDA_PlotHPGLFrame::HPGL_Plot( wxCommandEvent& event )
/***********************************************************/
{
    int Select_PlotAll = FALSE;

    if( event.GetId() == ID_PLOT_HPGL_ALL_EXECUTE )
        Select_PlotAll = TRUE;

    if( HPGL_SizeSelect )
    {
        wxString msg = m_PlotOrgPosition_X->GetValue();
        Plot_sheet_list[HPGL_SizeSelect]->m_Offset.x =
            ReturnValueFromString( g_UnitMetric, msg, EESCHEMA_INTERNAL_UNIT );
        msg = m_PlotOrgPosition_Y->GetValue();
        Plot_sheet_list[HPGL_SizeSelect]->m_Offset.y =
            ReturnValueFromString( g_UnitMetric, msg, EESCHEMA_INTERNAL_UNIT );
    }

    Plot_Schematic_HPGL( Select_PlotAll, HPGL_SizeSelect );
}


/*******************************************************************/
void WinEDA_PlotHPGLFrame::ReturnSheetDims( BASE_SCREEN* screen,
                                            wxSize& SheetSize, wxPoint& SheetOffset )
/*******************************************************************/

/* Fonction calculant les dims et offsets de trace de la feuille selectionnee
 * retourne:
 */
{
    Ki_PageDescr* PlotSheet;

    if( screen == NULL )
        screen = m_Parent->GetBaseScreen();

    PlotSheet = screen->m_CurrentSheetDesc;

    SheetSize   = PlotSheet->m_Size;
    SheetOffset = PlotSheet->m_Offset;
}


/***********************************************************************************/
void WinEDA_PlotHPGLFrame::Plot_Schematic_HPGL( int Select_PlotAll, int HPGL_SheetSize )
/***********************************************************************************/
{
    WinEDA_SchematicFrame* schframe = (WinEDA_SchematicFrame*) m_Parent;
    wxString PlotFileName;
    SCH_SCREEN*            screen    = schframe->GetScreen();
    SCH_SCREEN*            oldscreen = screen;
    DrawSheetPath*         sheetpath, * oldsheetpath = schframe->GetSheet();
    Ki_PageDescr*          PlotSheet;
    wxSize  SheetSize;
    wxPoint SheetOffset, PlotOffset;
    int     margin;

    g_PlotFormat = PLOT_FORMAT_HPGL;

    /* When printing all pages, the printed page is not the current page.
     *  In complex hierarchies, we must setup references and others parameters in the printed SCH_SCREEN
     *  because in complex hierarchies a SCH_SCREEN (a schematic drawings)
     *  is shared between many sheets
     */
    EDA_SheetList SheetList( NULL );
    sheetpath = SheetList.GetFirst();
    DrawSheetPath list;

    for( ; ;  )
    {
        if( Select_PlotAll )
        {
            if( sheetpath == NULL )
                break;
            list.Clear();
            if( list.BuildSheetPathInfoFromSheetPathValue( sheetpath->Path() ) )
            {
                schframe->m_CurrentSheet = &list;
                schframe->m_CurrentSheet->UpdateAllScreenReferences();
                schframe->SetSheetNumberAndCount();
                screen = schframe->m_CurrentSheet->LastScreen();
                ActiveScreen = screen;
            }
            else  // Should not occur
                return;
            sheetpath = SheetList.GetNext();
        }
        ReturnSheetDims( screen, SheetSize, SheetOffset );
        /* Calcul des echelles de conversion */
        g_PlotScaleX = Scale_X * SCALE_HPGL;
        g_PlotScaleY = Scale_Y * SCALE_HPGL;

        margin       = 400; // Margin in mils
        PlotSheet    = screen->m_CurrentSheetDesc;
        g_PlotScaleX = g_PlotScaleX * (SheetSize.x - 2 * margin) / PlotSheet->m_Size.x;
        g_PlotScaleY = g_PlotScaleY * (SheetSize.y - 2 * margin) / PlotSheet->m_Size.y;

        /* calcul des offsets */
        PlotOffset.x  = -(int) ( SheetOffset.x * SCALE_HPGL );
        PlotOffset.y  = (int) ( (SheetOffset.y + SheetSize.y) * SCALE_HPGL );
        PlotOffset.x -= (int) ( margin * SCALE_HPGL );
        PlotOffset.y += (int) ( margin * SCALE_HPGL );

        PlotFileName = schframe->GetUniqueFilenameForCurrentSheet() + wxT( ".plt" );

        SetLocaleTo_C_standard();
        InitPlotParametresHPGL( PlotOffset, g_PlotScaleX, g_PlotScaleY );
        Plot_1_Page_HPGL( PlotFileName, screen );
        SetLocaleTo_Default();

        if( !Select_PlotAll )
            break;
    }

    ActiveScreen = oldscreen;
    schframe->m_CurrentSheet = oldsheetpath;
    schframe->m_CurrentSheet->UpdateAllScreenReferences();
    schframe->SetSheetNumberAndCount();
}


/**************************************************************************/
void WinEDA_PlotHPGLFrame::Plot_1_Page_HPGL( const wxString& FullFileName,
                                             BASE_SCREEN*    screen )
/**************************************************************************/

/* Trace en format HPGL. d'une feuille de dessin
 * 1 unite HPGL = 0.98 mils ( 1 mil = 1.02041 unite HPGL ) .
 */
{
    EDA_BaseStruct* DrawList;
    SCH_COMPONENT*  DrawLibItem;
    int             x1 = 0, y1 = 0, x2 = 0, y2 = 0, layer;
    wxString        msg;

    PlotOutput = wxFopen( FullFileName, wxT( "wt" ) );
    if( PlotOutput == 0 )
    {
        msg = _( "Unable to create " ) + FullFileName;
        DisplayError( this, msg ); return;
    }

    msg = _( "Plot  " ) + FullFileName + wxT( "\n" );
    m_MsgBox->AppendText( msg );

    /* Init : */
    PrintHeaderHPGL( PlotOutput, g_HPGL_Pen_Descr.m_Pen_Speed, g_HPGL_Pen_Descr.m_Pen_Num );

    m_Parent->PlotWorkSheet( PLOT_FORMAT_HPGL, screen );

    DrawList = screen->EEDrawList;
    while( DrawList )  /* tracage */
    {
        Plume( 'U' );
        layer = LAYER_NOTES;

        switch( DrawList->Type() )
        {
        case DRAW_BUSENTRY_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (DrawBusEntryStruct*) DrawList )
            x1    = STRUCT->m_Pos.x; y1 = STRUCT->m_Pos.y;
            x2    = STRUCT->m_End().x; y2 = STRUCT->m_End().y;
            layer = STRUCT->GetLayer();

        case DRAW_SEGMENT_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (EDA_DrawLineStruct*) DrawList )
            if( DrawList->Type() == DRAW_SEGMENT_STRUCT_TYPE )
            {
                x1    = STRUCT->m_Start.x; y1 = STRUCT->m_Start.y;
                x2    = STRUCT->m_End.x; y2 = STRUCT->m_End.y;
                layer = STRUCT->GetLayer();
            }

            switch( layer )
            {
            case LAYER_NOTES:         /* Trace en pointilles */
                Move_Plume( wxPoint( x1, y1 ), 'U' );
                fprintf( PlotOutput, "LT 2;\n" );
                Move_Plume( wxPoint( x2, y2 ), 'D' );
                fprintf( PlotOutput, "LT;\n" );
                break;

            case LAYER_BUS:         /* Trait large */
            {
                int deltaX = 0, deltaY = 0; double angle;
                if( (x2 - x1) == 0 )
                    deltaX = 8;
                else if( (y2 - y1) == 0 )
                    deltaY = 8;
                else
                {
                    angle  = atan2( (double) ( x2 - x1 ), (double) ( y1 - y2 ) );
                    deltaX = (int) ( 8 * sin( angle ) );
                    deltaY = (int) ( 8 * cos( angle ) );
                }
                Move_Plume( wxPoint( x1 + deltaX, y1 - deltaY ), 'U' );
                Move_Plume( wxPoint( x1 - deltaX, y1 + deltaY ), 'D' );
                Move_Plume( wxPoint( x2 - deltaX, y2 + deltaY ), 'D' );
                Move_Plume( wxPoint( x2 + deltaX, y2 - deltaY ), 'D' );
                Move_Plume( wxPoint( x1 + deltaX, y1 - deltaY ), 'D' );
            }
            break;

            default:
                Move_Plume( wxPoint( x1, y1 ), 'U' );
                Move_Plume( wxPoint( x2, y2 ), 'D' );
                break;
            }

            break;

        case DRAW_JUNCTION_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (DrawJunctionStruct*) DrawList )
            x1 = STRUCT->m_Pos.x; y1 = STRUCT->m_Pos.y;
            PlotCercle( wxPoint( x1, y1 ), true, DRAWJUNCTION_SIZE * 2 );
            break;

        case TYPE_SCH_TEXT:
        case TYPE_SCH_LABEL:
        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
            PlotTextStruct( DrawList );
            break;

        case TYPE_SCH_COMPONENT:
            DrawLibItem = (SCH_COMPONENT*) DrawList;
            PlotLibPart( DrawLibItem );
            break;

        case DRAW_PICK_ITEM_STRUCT_TYPE:
            break;

        case DRAW_POLYLINE_STRUCT_TYPE:
            break;

        case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
            break;

        case DRAW_MARKER_STRUCT_TYPE:
            break;

        case DRAW_SHEET_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (DrawSheetStruct*) DrawList )
            PlotSheetStruct( STRUCT );
            break;

        case DRAW_NOCONNECT_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (DrawNoConnectStruct*) DrawList )
            PlotNoConnectStruct( STRUCT );
            break;

        default:
            break;
        }

        Plume( 'U' );
        DrawList = DrawList->Next();
    }

    /* fin */
    CloseFileHPGL( PlotOutput );
}


/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_RADIOBOX
 */

void WinEDA_PlotHPGLFrame::OnRadioboxSelected( wxCommandEvent& event )
{
    HPGL_SizeSelect = m_SizeOption->GetSelection();
    SetPageOffsetValue();
}


/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_PEN_WIDTH_UPDATED
 */

void WinEDA_PlotHPGLFrame::OnPenWidthUpdatedUpdated( wxSpinEvent& event )
{
    SetPenWidth( event );
}


/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_PEN_SPEED_UPDATED
 */

void WinEDA_PlotHPGLFrame::OnPenSpeedUpdatedUpdated( wxSpinEvent& event )
{
    SetPenSpeed( event );
}


/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_PEN_NUMBER_UPDATED
 */

void WinEDA_PlotHPGLFrame::OnPenNumberUpdatedUpdated( wxSpinEvent& event )
{
    SetPenNum( event );
}
