/*
	Copyright (c) 2013-2014, 2016-2020 Cong Xu
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

#define CHEBYSHEV_DISTANCE(x1, y1, x2, y2)                                    \
	MAX(fabsf((x1) - (x2)), fabsf((y1) - (y2)))

struct vec2i svec2i_scale_divide(const struct vec2i v, const mint_t scale);

struct vec2i Vec2iToTile(struct vec2i v);
struct vec2i Vec2iCenterOfTile(struct vec2i v);
struct vec2i Vec2ToTile(const struct vec2 v);
struct vec2 Vec2CenterOfTile(const struct vec2i v);

// Helper macros for positioning
#define CENTER_X(_pos, _size, _w) ((_pos).x + ((_size).x - (_w)) / 2)
#define CENTER_Y(_pos, _size, _h) ((_pos).y + ((_size).y - (_h)) / 2)

typedef struct
{
	struct vec2i Pos;
	struct vec2i Size;
} Rect2i;
// Convenience macro for looping through elements in rect
#define RECT_FOREACH(_r)                                                      \
	{                                                                         \
		struct vec2i _v;                                                      \
		for (_v.y = _r.Pos.y; _v.y < _r.Pos.y + _r.Size.y; _v.y++)            \
		{                                                                     \
			for (_v.x = _r.Pos.x; _v.x < _r.Pos.x + _r.Size.x; _v.x++)        \
			{                                                                 \
				const int _i = _v.x + _v.y * _r.Size.x;                       \
				UNUSED(_i);
#define RECT_FOREACH_END()                                                    \
	}                                                                         \
	}                                                                         \
	}

Rect2i Rect2iNew(const struct vec2i pos, const struct vec2i size);
Rect2i Rect2iZero(void);
bool Rect2iIsZero(const Rect2i r);
bool Rect2iIsAtEdge(const Rect2i r, const struct vec2i v);
bool Rect2iIsInside(const Rect2i r, const struct vec2i v);
bool Rect2iOverlap(const Rect2i r1, const Rect2i r2);
struct vec2i Rect2iCenter(const Rect2i r);
