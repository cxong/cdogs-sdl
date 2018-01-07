/*
    Copyright (c) 2013-2014, 2016-2018 Cong Xu
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

#include <math.h>

#include "mathc/mathc.h"
#include "utils.h"

#define CHEBYSHEV_DISTANCE(x1, y1, x2, y2) \
	MAX(fabsf((x1) - (x2)), fabsf((y1) - (y2)))

typedef struct
{
	int x;
	int y;
} Vec2i;

Vec2i Vec2iNew(int x, int y);
Vec2i Vec2iZero(void);
Vec2i Vec2iUnit(void);	// (1, 1)
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

Vec2i Vec2iToTile(Vec2i v);
Vec2i Vec2iCenterOfTile(Vec2i v);
Vec2i Vec2ToVec2i(const struct vec v);
Vec2i Vec2ToTile(const struct vec v);
struct vec Vec2iToVec2(const Vec2i v);
struct vec Vec2CenterOfTile(const Vec2i v);

int Vec2iSqrMagnitude(const Vec2i v);
int DistanceSquared(const Vec2i a, const Vec2i b);

// Helper macros for positioning
#define CENTER_X(_pos, _size, _w) ((_pos).x + ((_size).x - (_w)) / 2)
#define CENTER_Y(_pos, _size, _h) ((_pos).y + ((_size).y - (_h)) / 2)

typedef struct
{
	Vec2i Pos;
	Vec2i Size;
} Rect2i;
// Convenience macro for looping through elements in rect
#define RECT_FOREACH(_r)\
	{\
		Vec2i _v;\
		for (_v.y = _r.Pos.y; _v.y < _r.Pos.y + _r.Size.y; _v.y++)\
		{\
			for (_v.x = _r.Pos.x; _v.x < _r.Pos.x + _r.Size.x; _v.x++)\
			{
#define RECT_FOREACH_END() } } }

Rect2i Rect2iNew(const Vec2i pos, const Vec2i size);
bool Rect2iIsAtEdge(const Rect2i r, const Vec2i v);
bool Rect2iOverlap(const Rect2i r1, const Rect2i r2);
