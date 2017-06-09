/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2017, Cong Xu
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
#include "minkowski_hex.h"


static bool RectangleLineIntersect(
	const Vec2i rectPos, const Vec2i rectSize,
	const Vec2i lineStart, const Vec2i lineEnd, float *s, Vec2i *normal);
bool MinkowskiHexCollide(
	const Vec2i posA, const Vec2i velA, const Vec2i sizeA,
	const Vec2i posB, const Vec2i velB, const Vec2i sizeB,
	Vec2i *colA, Vec2i *colB, Vec2i *normal)
{
	// Find rectangle C by combining A and B
	const Vec2i sizeC = Vec2iAdd(sizeA, sizeB);

	// Subtract velA from vB
	const Vec2i velBA = Vec2iMinus(velB, velA);

	// Find the intersection between velBA and the rectangle C centered on posA
	float s;
	if (!RectangleLineIntersect(
		posA, sizeC, posB, Vec2iAdd(posB, velBA), &s, normal))
	{
		return false;
	}

	// If intersection is at the start, it means we were overlapping already
	if (s == 0)
	{
		*colA = posA;
		*colB = posB;
	}
	else
	{
		// Find the actual intersection points based on the result
		*colA = Vec2iAdd(posA, Vec2iScaleD(velA, s));
		*colB = Vec2iAdd(posB, Vec2iScaleD(velB, s));
	}

	return true;
}
static bool LinesIntersect(
	const Vec2i p1Start, const Vec2i p1End,
	const Vec2i p2Start, const Vec2i p2End,
	float *s);
static bool RectangleLineIntersect(
	const Vec2i rectPos, const Vec2i rectSize,
	const Vec2i lineStart, const Vec2i lineEnd, float *s, Vec2i *normal)
{
	// Find the closest point at which a line intersects a rectangle
	// Do this by finding intersections between the line and all four sides of
	// the rectangle, and returning the closest one
	const int left = rectPos.x - rectSize.x / 2;
	const int right = left + rectSize.x;
	const int top = rectPos.y - rectSize.y / 2;
	const int bottom = top + rectSize.y;
	const Vec2i topLeft = Vec2iNew(left, top);
	const Vec2i topRight = Vec2iNew(right, top);
	const Vec2i bottomRight = Vec2iNew(right, bottom);
	const Vec2i bottomLeft = Vec2iNew(left, bottom);

	// Border case: check if line start is entirely within rectangle
	if (lineStart.x > left && lineStart.x < right &&
		lineStart.y > top && lineStart.y < bottom)
	{
		*s = 0;
		*normal = Vec2iZero();
		return true;
	}

	// Find closest of four intersections
	*s = -1;
	float sPart;
#define _CHECK_MIN_DISTANCE(_p1, _p2, _n) \
	if (LinesIntersect(_p1, _p2, lineStart, lineEnd, &sPart) &&\
		(*s < 0 || sPart < *s))\
	{\
		*s = sPart;\
		*normal = _n;\
	}
	_CHECK_MIN_DISTANCE(topLeft, topRight, Vec2iNew(0, 1));
	_CHECK_MIN_DISTANCE(topRight, bottomRight, Vec2iNew(-1, 0));
	_CHECK_MIN_DISTANCE(bottomLeft, bottomRight, Vec2iNew(0, -1));
	_CHECK_MIN_DISTANCE(topLeft, bottomLeft, Vec2iNew(1, 0));

	return *s >= 0 && *s <= 1;
}
static bool LinesIntersect(
	const Vec2i p1Start, const Vec2i p1End,
	const Vec2i p2Start, const Vec2i p2End,
	float *s)
{
	// Return whether two line segments intersect, and at which fraction
	// http://stackoverflow.com/a/1968345/2038264
	const Vec2i v1 = Vec2iMinus(p1End, p1Start);
	const Vec2i v2 = Vec2iMinus(p2End, p2Start);

	const int divisor = -v2.x * v1.y + v1.x * v2.y;
	if (divisor == 0)
	{
		// Lines are parallel/collinear, just assume no collision
		return false;
	}
	const Vec2i p12Start = Vec2iMinus(p1Start, p2Start);
	*s = (float)(-v1.y * p12Start.x + v1.x * p12Start.y) / divisor;
	const float t = (float)(v2.x * p12Start.y - v2.y * p12Start.x) / divisor;

	return *s >= 0 && *s <= 1 && t >= 0 && t <= 1;
}
