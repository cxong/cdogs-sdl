/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2014, Cong Xu
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
#ifndef __ALGORITHMS
#define __ALGORITHMS

#include <stdbool.h>

#include "vector.h"

typedef struct
{
	bool (*IsBlocked)(void *, Vec2i);
	void *data;
} HasClearLineData;
// Use Bresenham line algorithm to determine whether line is clear
bool HasClearLineBresenham(Vec2i from, Vec2i to, HasClearLineData *data);
bool HasClearLineXiaolinWu(Vec2i from, Vec2i to, HasClearLineData *data);

typedef struct
{
	void (*Draw)(void *, Vec2i);
	void *data;
} AlgoLineDrawData;
void BresenhamLineDraw(Vec2i from, Vec2i to, AlgoLineDrawData *data);
void XiaolinWuLineDraw(Vec2i from, Vec2i to, AlgoLineDrawData *data);

typedef struct
{
	void (*Fill)(void *, Vec2i);
	bool (*IsSame)(void *, Vec2i);
	void *data;
} FloodFillData;
bool CFloodFill(Vec2i v, FloodFillData *data);

#endif
