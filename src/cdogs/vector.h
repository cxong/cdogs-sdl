/*
    Copyright (c) 2013-2014, 2016-2017 Cong Xu
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include "utils.h"

#define CHEBYSHEV_DISTANCE(x1, y1, x2, y2)  MAX(abs((x1) - (x2)), abs((y1) - (y2)))

typedef struct
{
	int x;
	int y;
} Vec2i;

Vec2i Vec2iNew(int x, int y);
Vec2i Vec2iZero(void);
Vec2i Vec2iUnit(void);	// (1, 1)
// r radius, theta radians; result will be rounded
// Note: 0 radians is north, continuing clockwise
Vec2i Vec2iFromPolar(const double r, const double th);
Vec2i Vec2iAdd(Vec2i a, Vec2i b);
Vec2i Vec2iMinus(Vec2i a, Vec2i b);
// Multiply the components of two Vec2is together
Vec2i Vec2iMult(const Vec2i a, const Vec2i b);
Vec2i Vec2iScale(Vec2i v, int scalar);
Vec2i Vec2iScaleD(const Vec2i v, const double scalar);
Vec2i Vec2iScaleDiv(Vec2i v, int scaleDiv);
// TODO: due to rounding, this will always return unit component vectors
Vec2i Vec2iNorm(Vec2i v);
bool Vec2iEqual(const Vec2i a, const Vec2i b);
bool Vec2iIsZero(const Vec2i v);
Vec2i Vec2iMin(Vec2i a, Vec2i b);	// Get min x and y of both vectors
Vec2i Vec2iMax(Vec2i a, Vec2i b);	// Get max x and y of both vectors
Vec2i Vec2iClamp(Vec2i v, Vec2i lo, Vec2i hi);

// Convert to and from real (i.e. integral) coordinates and full (fractional)
Vec2i Vec2iFull2Real(Vec2i v);
Vec2i Vec2iReal2Full(Vec2i v);
Vec2i Vec2iReal2FullCentered(const Vec2i v);

Vec2i Vec2iToTile(Vec2i v);
Vec2i Vec2iCenterOfTile(Vec2i v);

int DistanceSquared(Vec2i a, Vec2i b);
void CalcChebyshevDistanceAndBearing(
	Vec2i origin, Vec2i target, int *distance, int *bearing);
Vec2i CalcClosestPointOnLineSegmentToPoint(
	Vec2i l1, Vec2i l2, Vec2i p);

// Helper macros for positioning
#define CENTER_X(_pos, _size, _w) ((_pos).x + ((_size).x - (_w)) / 2)
#define CENTER_Y(_pos, _size, _h) ((_pos).y + ((_size).y - (_h)) / 2)

typedef struct
{
	Vec2i Pos;
	Vec2i Size;
} Rect2i;
