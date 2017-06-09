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
#include "vector.h"

#include <math.h>

#include "map.h"

Vec2i Vec2iNew(int x, int y)
{
	Vec2i v;
	v.x = x;
	v.y = y;
	return v;
}

Vec2i Vec2iZero(void)
{
	return Vec2iNew(0, 0);
}
Vec2i Vec2iUnit(void)
{
	return Vec2iNew(1, 1);
}
Vec2i Vec2iFromPolar(const double r, const double th)
{
	return Vec2iNew((int)Round(sin(th) * r), -(int)Round(cos(th) * r));
}

Vec2i Vec2iAdd(Vec2i a, Vec2i b)
{
	a.x += b.x;
	a.y += b.y;
	return a;
}
Vec2i Vec2iMinus(Vec2i a, Vec2i b)
{
	a.x -= b.x;
	a.y -= b.y;
	return a;
}

Vec2i Vec2iMult(const Vec2i a, const Vec2i b)
{
	return Vec2iNew(a.x * b.x, a.y * b.y);
}

Vec2i Vec2iScale(Vec2i v, int scalar)
{
	v.x *= scalar;
	v.y *= scalar;
	return v;
}

Vec2i Vec2iScaleD(const Vec2i v, const double scalar)
{
	return Vec2iNew((int)(v.x * scalar), (int)(v.y * scalar));
}

Vec2i Vec2iScaleDiv(Vec2i v, int scaleDiv)
{
	v.x /= scaleDiv;
	v.y /= scaleDiv;
	return v;
}

Vec2i Vec2iNorm(Vec2i v)
{
	double magnitude;
	if (Vec2iIsZero(v))
	{
		return v;
	}
	magnitude = sqrt(v.x*v.x + v.y*v.y);
	v.x = (int)floor(v.x / magnitude + 0.5);
	v.y = (int)floor(v.y / magnitude + 0.5);
	return v;
}

bool Vec2iEqual(const Vec2i a, const Vec2i b)
{
	return a.x == b.x && a.y == b.y;
}
bool Vec2iIsZero(const Vec2i v)
{
	return Vec2iEqual(v, Vec2iZero());
}

Vec2i Vec2iMin(Vec2i a, Vec2i b)
{
	return Vec2iNew(MIN(a.x, b.x), MIN(a.y, b.y));
}
Vec2i Vec2iMax(Vec2i a, Vec2i b)
{
	return Vec2iNew(MAX(a.x, b.x), MAX(a.y, b.y));
}
Vec2i Vec2iClamp(Vec2i v, Vec2i lo, Vec2i hi)
{
	v.x = CLAMP(v.x, lo.x, hi.x);
	v.y = CLAMP(v.y, lo.y, hi.y);
	return v;
}

Vec2i Vec2iFull2Real(Vec2i v)
{
	return Vec2iScaleDiv(v, 256);
}
Vec2i Vec2iReal2Full(Vec2i v)
{
	return Vec2iScale(v, 256);
}
Vec2i Vec2iReal2FullCentered(const Vec2i v)
{
	return Vec2iAdd(
		Vec2iScale(v, 256), Vec2iNew(128 * SIGN(v.x), 128 * SIGN(v.y)));
}

Vec2i Vec2iToTile(Vec2i v)
{
	return Vec2iNew(v.x / TILE_WIDTH, v.y / TILE_HEIGHT);
}
Vec2i Vec2iCenterOfTile(Vec2i v)
{
	return Vec2iNew(
		v.x * TILE_WIDTH + TILE_WIDTH / 2,
		v.y * TILE_HEIGHT + TILE_HEIGHT / 2);
}

void CalcChebyshevDistanceAndBearing(
	Vec2i origin, Vec2i target, int *distance, int *bearing)
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

int DistanceSquared(Vec2i a, Vec2i b)
{
	int dx = a.x - b.x;
	int dy = a.y - b.y;
	return dx*dx + dy*dy;
}

Vec2i CalcClosestPointOnLineSegmentToPoint(
	Vec2i l1, Vec2i l2, Vec2i p)
{
	// Using parametric representation, line l1->l2 is
	// P(t) = l1 + t(l2 - l1)
	// Projection of point p on line is
	// t = ((p.x - l1.x)(l2.x - l1.x) + (p.y - l1.y)(l2.y - l1.y)) / ||l2 - l1||^2
	int lineDistanceSquared = DistanceSquared(l1, l2);
	int numerator;
	double t;
	Vec2i closestPoint;
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
