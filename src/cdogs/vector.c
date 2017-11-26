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

#include "tile.h"


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

Vec2i Vec2ToVec2i(const struct vec v)
{
	return Vec2iNew(v.x, v.y);
}
Vec2i Vec2ToTile(const struct vec v)
{
	return Vec2iNew(v.x / TILE_WIDTH, v.y / TILE_HEIGHT);
}
struct vec Vec2iToVec2(const Vec2i v)
{
	return to_vector2(v.x, v.y);
}
struct vec Vec2CenterOfTile(const Vec2i v)
{
	return Vec2iToVec2(Vec2iCenterOfTile(v));
}

int Vec2iSqrMagnitude(const Vec2i v)
{
	return v.x * v.x + v.y * v.y;
}

int DistanceSquared(const Vec2i a, const Vec2i b)
{
	const Vec2i d = Vec2iMinus(a, b);
	return Vec2iSqrMagnitude(d);
}


bool Rect2iIsAtEdge(const Rect2i r, const Vec2i v)
{
	return
		v.y == r.Pos.y || v.y == r.Pos.y + r.Size.y - 1 ||
		v.x == r.Pos.x || v.x == r.Pos.x + r.Size.x - 1;
}

bool Rect2iOverlap(const Rect2i r1, const Rect2i r2)
{
	return
		r1.Pos.x < r2.Pos.x + r2.Size.x && r1.Pos.x + r1.Size.x > r2.Pos.x &&
		r1.Pos.y < r2.Pos.y + r2.Size.y && r1.Pos.y + r1.Size.y > r2.Pos.y;
}
