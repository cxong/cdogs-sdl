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
	const struct vec2 rectPos, const struct vec2i rectSize,
	const struct vec2 lineStart, const struct vec2 lineEnd, float *s,
	struct vec2 *normal);
bool MinkowskiHexCollide(
	const struct vec2 posA, const struct vec2 velA, const struct vec2i sizeA,
	const struct vec2 posB, const struct vec2 velB, const struct vec2i sizeB,
	struct vec2 *colA, struct vec2 *colB, struct vec2 *normal)
{
	// Find rectangle C by combining A and B
	const struct vec2i sizeC = svec2i_add(sizeA, sizeB);

	// Subtract velA from vB
	const struct vec2 velBA = svec2_subtract(velB, velA);

	// Find the intersection between velBA and the rectangle C centered on posA
	float s;
	if (!RectangleLineIntersect(
		posA, sizeC, posB, svec2_add(posB, velBA), &s, normal))
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
		*colA = svec2_add(posA, svec2_scale(velA, s));
		*colB = svec2_add(posB, svec2_scale(velB, s));
	}

	return true;
}
static bool LinesIntersect(
	const struct vec2 p1Start, const struct vec2 p1End,
	const struct vec2 p2Start, const struct vec2 p2End,
	float *s);
static bool RectangleLineIntersect(
	const struct vec2 rectPos, const struct vec2i rectSize,
	const struct vec2 lineStart, const struct vec2 lineEnd, float *s,
	struct vec2 *normal)
{
	// Find the closest point at which a line intersects a rectangle
	// Do this by finding intersections between the line and all four sides of
	// the rectangle, and returning the closest one
	const float left = rectPos.x - rectSize.x / 2;
	const float right = left + rectSize.x;
	const float top = rectPos.y - rectSize.y / 2;
	const float bottom = top + rectSize.y;
	const struct vec2 topLeft = svec2(left, top);
	const struct vec2 topRight = svec2(right, top);
	const struct vec2 bottomRight = svec2(right, bottom);
	const struct vec2 bottomLeft = svec2(left, bottom);

	// Border case: check if line start is entirely within rectangle
	if (lineStart.x > left && lineStart.x < right &&
		lineStart.y > top && lineStart.y < bottom)
	{
		*s = 0;
		*normal = svec2_zero();
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
	_CHECK_MIN_DISTANCE(topLeft, topRight, svec2(0, 1));
	_CHECK_MIN_DISTANCE(topRight, bottomRight, svec2(-1, 0));
	_CHECK_MIN_DISTANCE(bottomLeft, bottomRight, svec2(0, -1));
	_CHECK_MIN_DISTANCE(topLeft, bottomLeft, svec2(1, 0));

	return *s >= 0 && *s <= 1;
}
static bool LinesIntersect(
	const struct vec2 p1Start, const struct vec2 p1End,
	const struct vec2 p2Start, const struct vec2 p2End,
	float *s)
{
	// Return whether two line segments intersect, and at which fraction
	// http://stackoverflow.com/a/1968345/2038264
	const struct vec2 v1 = svec2_subtract(p1End, p1Start);
	const struct vec2 v2 = svec2_subtract(p2End, p2Start);

	const float divisor = -v2.x * v1.y + v1.x * v2.y;
	if (divisor == 0)
	{
		// Lines are parallel/collinear, just assume no collision
		return false;
	}
	const struct vec2 p12Start = svec2_subtract(p1Start, p2Start);
	*s = (float)(-v1.y * p12Start.x + v1.x * p12Start.y) / divisor;
	const float t = (float)(v2.x * p12Start.y - v2.y * p12Start.x) / divisor;

	return *s >= 0 && *s <= 1 && t >= 0 && t <= 1;
}
