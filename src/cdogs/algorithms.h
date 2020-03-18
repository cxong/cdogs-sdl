/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2014, 2017, 2020 Cong Xu
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

#include <stdbool.h>

#include "vector.h"

typedef struct
{
	bool (*IsBlocked)(void *, struct vec2i);
	void *data;
} HasClearLineData;
// Use Bresenham line algorithm to determine whether line is clear
bool HasClearLineBresenham(
	struct vec2i from, struct vec2i to, HasClearLineData *data);
bool HasClearLineJMRaytrace(
	const struct vec2i from, const struct vec2i to, HasClearLineData *data);

typedef struct
{
	void (*Draw)(void *, struct vec2i);
	void *data;
} AlgoLineDrawData;
void BresenhamLineDraw(
	struct vec2i from, struct vec2i to, AlgoLineDrawData *data);
void JMRaytraceLineDraw(
	const struct vec2i from, const struct vec2i to, AlgoLineDrawData *data);

typedef struct
{
	void (*Fill)(void *, struct vec2i);
	bool (*IsSame)(void *, struct vec2i);
	void *data;
} FloodFillData;
bool CFloodFill(const struct vec2i v, FloodFillData *data);
