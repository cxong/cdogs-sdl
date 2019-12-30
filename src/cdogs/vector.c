/*
    Copyright (c) 2013-2014, 2016-2019 Cong Xu
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

struct vec2i svec2i_scale_divide(const struct vec2i v, const mint_t scale)
{
	return svec2i(v.x / scale, v.y / scale);
}

struct vec2i Vec2iToTile(struct vec2i v)
{
	return svec2i(v.x / TILE_WIDTH, v.y / TILE_HEIGHT);
}

struct vec2i Vec2iCenterOfTile(struct vec2i v)
{
	return svec2i(
		v.x * TILE_WIDTH + TILE_WIDTH / 2,
		v.y * TILE_HEIGHT + TILE_HEIGHT / 2);
}

struct vec2i Vec2ToTile(const struct vec2 v)
{
	return svec2i((int)(v.x / TILE_WIDTH), (int)(v.y / TILE_HEIGHT));
}

struct vec2 Vec2CenterOfTile(const struct vec2i v)
{
	return svec2_assign_vec2i(Vec2iCenterOfTile(v));
}

Rect2i Rect2iNew(const struct vec2i pos, const struct vec2i size)
{
	Rect2i r;
	r.Pos = pos;
	r.Size = size;
	return r;
}
Rect2i Rect2iZero(void)
{
	return Rect2iNew(svec2i_zero(), svec2i_zero());
}

bool Rect2iIsZero(const Rect2i r)
{
	return svec2i_is_zero(r.Pos) && svec2i_is_zero(r.Size);
}

bool Rect2iIsAtEdge(const Rect2i r, const struct vec2i v)
{
	return
		v.y == r.Pos.y || v.y == r.Pos.y + r.Size.y - 1 ||
		v.x == r.Pos.x || v.x == r.Pos.x + r.Size.x - 1;
}

bool Rect2iIsInside(const Rect2i r, const struct vec2i v)
{
	return !(
		v.y < r.Pos.y || v.y > r.Pos.y + r.Size.y - 1 ||
		v.x < r.Pos.x || v.x > r.Pos.x + r.Size.x - 1);
}

bool Rect2iOverlap(const Rect2i r1, const Rect2i r2)
{
	return
		r1.Pos.x < r2.Pos.x + r2.Size.x && r1.Pos.x + r1.Size.x > r2.Pos.x &&
		r1.Pos.y < r2.Pos.y + r2.Size.y && r1.Pos.y + r1.Size.y > r2.Pos.y;
}
