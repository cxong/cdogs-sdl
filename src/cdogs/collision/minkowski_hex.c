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
	const Vec2i lineStart, const Vec2i lineEnd,
	Vec2i *intersectionPoint);
bool MinkowskiHexCollide(
	const Vec2i posA, const Vec2i velA, const Vec2i sizeA,
	const Vec2i posB, const Vec2i velB, const Vec2i sizeB,
	Vec2i *collideA, Vec2i *collideB)
{
	// Find rectangle C by combining A and B
	const Vec2i sizeC = Vec2iAdd(sizeA, sizeB);

	// Subtract velA from vB
	const Vec2i velBA = Vec2iMinus(velB, velA);

	// Find the point of intersection between velBA and the rectangle C
	// centered on posA
	Vec2i intersect;
	if (!RectangleLineIntersect(
		posA, sizeC, posB, Vec2iAdd(posB, velBA), &intersect))
	{
		return false;
	}

	// We've found the intersection point but it's not the real one;
	// use its proportion to velBA and apply to velA and velB
	*collideA = Vec2iAdd(
		posA, Vec2iScaleDiv(Vec2iScale(velA, intersect.x), velBA.x));
	*collideB = Vec2iAdd(
		posB, Vec2iScaleDiv(Vec2iScale(velB, intersect.x), velBA.x));

	return true;
}
static bool LinesIntersect(
	const Vec2i p1Start, const Vec2i p1End,
	const Vec2i p2Start, const Vec2i p2End,
	Vec2i *collisionPoint);
static bool RectangleLineIntersect(
	const Vec2i rectPos, const Vec2i rectSize,
	const Vec2i lineStart, const Vec2i lineEnd,
	Vec2i *intersectionPoint)
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

	// Find four intersections
	Vec2i leftIntersect;
	const bool isLeft = LinesIntersect(
		topLeft, bottomLeft, lineStart, lineEnd, &leftIntersect);
	Vec2i rightIntersect;
	const bool isRight = LinesIntersect(
		topRight, bottomRight, lineStart, lineEnd, &rightIntersect);
	Vec2i topIntersect;
	const bool isTop = LinesIntersect(
		topLeft, topRight, lineStart, lineEnd, &topIntersect);
	Vec2i bottomIntersect;
	const bool isBottom = LinesIntersect(
		bottomLeft, bottomRight, lineStart, lineEnd, &bottomIntersect);

	// Find closest intersection
	int minDistanceSquared = -1;
#define _CHECK_MIN_DISTANCE(_isDir, _intersect) \
	if (_isDir)\
	{\
		const int distance = DistanceSquared(_intersect, lineStart);\
		if (minDistanceSquared < 0 || distance < minDistanceSquared)\
		{\
			minDistanceSquared = distance;\
			*intersectionPoint = _intersect;\
		}\
	}
	_CHECK_MIN_DISTANCE(isLeft, leftIntersect);
	_CHECK_MIN_DISTANCE(isRight, rightIntersect);
	_CHECK_MIN_DISTANCE(isTop, topIntersect);
	_CHECK_MIN_DISTANCE(isBottom, bottomIntersect);

	// Special case if line is entirely within rectangle
	if (minDistanceSquared < 0 &&
		left <= lineStart.x && lineStart.x <= right &&
		top <= lineStart.y && lineStart.y <= bottom)
	{
		*intersectionPoint = lineStart;
		return true;
	}

	return minDistanceSquared >= 0;
}
static bool LinesIntersect(
	const Vec2i p1Start, const Vec2i p1End,
	const Vec2i p2Start, const Vec2i p2End,
	Vec2i *collisionPoint)
{
	// Return whether two line segments intersect, and where
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
	const float s = (float)(-v1.y * p12Start.x + v1.x * p12Start.y) / divisor;
	const float t = (float)(v2.x * p12Start.y - v2.y * p12Start.x) / divisor;

	if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
	{
		// Collision detected
		*collisionPoint = Vec2iNew(
			(int)(p1Start.x + (t * v1.x)), (int)(p1End.y + (t * v1.y)));
		return true;
	}

	return false; // No collision
}