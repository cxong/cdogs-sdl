/*
    Copyright (c) 2013, Cong Xu
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
#include "vector.h"

#include <math.h>

void CalcChebyshevDistanceAndBearing(
	Vector2i origin, Vector2i target, int *distance, int *bearing)
{
	// short circuit if origin and target same
	if (origin.x == target.x && origin.y == target.y)
	{
		*distance = 0;
		*bearing = 0;
	}
	else
	{
		double angle;
		*distance = CHEBYSHEV_DISTANCE(origin.x, origin.y, target.x, target.y);
		angle = ToDegrees(atan2(target.y - origin.y, target.x - origin.x));
		// convert angle to bearings
		// first rotate so 0 angle = 0 bearing
		angle -= 90.0;
		// then reflect about Y axis
		angle = 360 - angle;
		*bearing = (int)floor(angle + 0.5);
	}
}

static Vector2i Minus(Vector2i a, Vector2i b)
{
	Vector2i x;
	x.x = a.x - b.x;
	x.y = a.y - b.y;
	return x;
}

static int DistanceSquared(Vector2i a, Vector2i b)
{
	int dx = a.x - b.x;
	int dy = a.y - b.y;
	return dx*dx + dy*dy;
}

Vector2i CalcClosestPointOnLineSegmentToPoint(
	Vector2i l1, Vector2i l2, Vector2i p)
{
	// Using parametric representation, line l1->l2 is
	// P(t) = l1 + t(l2 - l1)
	// Projection of point p on line is
	// t = ((p.x - l1.x)(l2.x - l1.x) + (p.y - l1.y)(l2.y - l1.y)) / ||l2 - l1||^2
	int lineDistanceSquared = DistanceSquared(l1, l2);
	int numerator;
	double t;
	Vector2i closestPoint;
	// Early exit since same point means 0 distance, and div by 0
	if (lineDistanceSquared == 0)
	{
		return l1;
	}
	numerator = (p.x - l1.x)*(l2.x - l1.x) + (p.y - l1.y)*(l2.y - l1.y);
	t = CLAMP(numerator * 1.0 / lineDistanceSquared, 0, 1);
	closestPoint.x = (int)Round(l1.x + t*(l2.x - l1.x));
	closestPoint.y = (int)Round(l1.y + t*(l2.y - l1.y));
	return closestPoint;
}
