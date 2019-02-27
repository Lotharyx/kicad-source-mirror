/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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

#ifndef CONVERT_BASIC_SHAPES_TO_POLYGON_H
#define CONVERT_BASIC_SHAPES_TO_POLYGON_H

/**
 * @file convert_basic_shapes_to_polygon.h
 */

#include <vector>

#include <fctsys.h>
#include <trigo.h>
#include <macros.h>

#include <geometry/shape_poly_set.h>
/**
 * Function TransformCircleToPolygon
 * convert a circle to a polygon, using multiple straight lines
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aCenter = the center of the circle
 * @param aRadius = the radius of the circle
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * Note: the polygon is inside the circle, so if you want to have the polygon
 * outside the circle, you should give aRadius calculated with a correction factor
 */
void TransformCircleToPolygon( SHAPE_POLY_SET&  aCornerBuffer,
                                                wxPoint aCenter, int aRadius,
                                                int aCircleToSegmentsCount );


/**
 * convert a oblong shape to a polygon, using multiple segments
 * It is similar to TransformRoundedEndsSegmentToPolygon, but the polygon
 * is outside the actual oblong shape (a segment with rounded ends)
 * It is suitable to create oblong clearance areas.
 * because multiple segments create a smaller area than the circle, the
 * radius of the circle to approximate must be bigger ( radius*aCorrectionFactor)
 * to create segments outside the circle.
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aStart = the first point of the segment
 * @param aEnd = the second point of the segment
 * @param aWidth = the width of the segment
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aCorrectionFactor = the coefficient to have segments outside the circle
 * if aCorrectionFactor = 1.0, the shape will be the same as
 * TransformRoundedEndsSegmentToPolygon
 */
void TransformOvalClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                wxPoint aStart, wxPoint aEnd, int aWidth,
                                int aCircleToSegmentsCount, double aCorrectionFactor );


/**
 * Helper function GetRoundRectCornerCenters
 * Has meaning only for rounded rect
 * Returns the centers of the rounded corners.
 * @param aCenters is the buffer to store the 4 coordinates.
 * @param aRadius = the radius of the of the rounded corners.
 * @param aPosition = position of the round rect
 * @param aSize = size of the of the round rect.
 * @param aRotation = rotation of the of the round rect
 */
void GetRoundRectCornerCenters( wxPoint aCenters[4], int aRadius,
            const wxPoint& aPosition, const wxSize& aSize, double aRotation );

/**
 * Function TransformRoundRectToPolygon
 * convert a rectangle with rounded corners to a polygon
 * Convert arcs to multiple straight lines
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aPosition = the coordinate of the center of the rectangle
 * @param aSize = the size of the rectangle
 * @param aCornerRadius = radius of rounded corners
 * @param aRotation = rotation in 0.1 degrees of the rectangle
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 */
void TransformRoundRectToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                  const wxPoint& aPosition, const wxSize& aSize,
                                  double aRotation, int aCornerRadius,
                                  int aCircleToSegmentsCount );

/**
 * Function TransformRoundedEndsSegmentToPolygon
 * convert a segment with rounded ends to a polygon
 * Convert arcs to multiple straight lines
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aStart = the segment start point coordinate
 * @param aEnd = the segment end point coordinate
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aWidth = the segment width
 * Note: the polygon is inside the arc ends, so if you want to have the polygon
 * outside the circle, you should give aStart and aEnd calculated with a correction factor
 */
void TransformRoundedEndsSegmentToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                           wxPoint aStart, wxPoint aEnd,
                                           int aCircleToSegmentsCount,
                                           int aWidth );


/**
 * Function TransformArcToPolygon
 * Creates a polygon from an Arc
 * Convert arcs to multiple straight segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aCentre = centre of the arc or circle
 * @param aStart = start point of the arc, or a point on the circle
 * @param aArcAngle = arc angle in 0.1 degrees. For a circle, aArcAngle = 3600
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aWidth = width (thickness) of the line
 */
void TransformArcToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                            wxPoint aCentre, wxPoint aStart, double aArcAngle,
                            int aCircleToSegmentsCount, int aWidth );

/**
 * Function TransformRingToPolygon
 * Creates a polygon from a ring
 * Convert arcs to multiple straight segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aCentre = centre of the arc or circle
 * @param aRadius = radius of the circle
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aWidth = width (thickness) of the ring
 */
void TransformRingToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                            wxPoint aCentre, int aRadius,
                            int aCircleToSegmentsCount, int aWidth );

#endif     // CONVERT_BASIC_SHAPES_TO_POLYGON_H
