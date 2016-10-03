/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file lib_circle.h
 */

#ifndef _LIB_CIRCLE_H_
#define _LIB_CIRCLE_H_

#include <lib_draw_item.h>


class LIB_CIRCLE : public LIB_ITEM
{
    int     m_Radius;
    wxPoint m_Pos;            // Position or centre (Arc and Circle) or start point (segments).
    int     m_Width;          // Line width.

    void drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                      EDA_COLOR_T aColor, GR_DRAWMODE aDrawMode, void* aData,
                      const TRANSFORM& aTransform ) override;

    void calcEdit( const wxPoint& aPosition ) override;

public:
    LIB_CIRCLE( LIB_PART * aParent );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~LIB_CIRCLE() { }

    wxString GetClass() const override
    {
        return wxT( "LIB_CIRCLE" );
    }


    bool Save( OUTPUTFORMATTER& aFormatter ) override;

    bool Load( LINE_READER& aLineReader, wxString& aErrorMsg ) override;

    bool HitTest( const wxPoint& aPosition ) const override;

    bool HitTest( const wxPoint& aPosRef, int aThreshold, const TRANSFORM& aTransform ) const override;

    int GetPenSize( ) const override;

    const EDA_RECT GetBoundingBox() const override;

    void GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList ) override;

    void BeginEdit( STATUS_FLAGS aEditMode, const wxPoint aStartPoint = wxPoint( 0, 0 ) ) override;

    bool ContinueEdit( const wxPoint aNextPoint ) override;

    void EndEdit( const wxPoint& aPosition, bool aAbort = false ) override;

    void SetOffset( const wxPoint& aOffset ) override;

    bool Inside( EDA_RECT& aRect ) const override;

    void Move( const wxPoint& aPosition ) override;

    wxPoint GetPosition() const override { return m_Pos; }

    void MirrorHorizontal( const wxPoint& aCenter ) override;

    void MirrorVertical( const wxPoint& aCenter ) override;

    void Rotate( const wxPoint& aCenter, bool aRotateCCW = true ) override;

    void Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
               const TRANSFORM& aTransform ) override;

    int GetWidth() const override { return m_Width; }

    void SetWidth( int aWidth ) override { m_Width = aWidth; }

    void SetRadius( int aRadius ) { m_Radius = aRadius; }

    int GetRadius() const { return m_Radius; }

    wxString GetSelectMenuText() const override;

    BITMAP_DEF GetMenuImage() const override { return  add_circle_xpm; }

    EDA_ITEM* Clone() const override;

private:

    /**
     * @copydoc LIB_ITEM::compare()
     *
     * The circle specific sort order is as follows:
     *      - Circle horizontal (X) position.
     *      - Circle vertical (Y) position.
     *      - Circle radius.
     */
    int compare( const LIB_ITEM& aOther ) const override;
};


#endif    // _LIB_CIRCLE_H_
