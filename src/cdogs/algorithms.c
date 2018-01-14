/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2014, 2017 Cong Xu
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
#include "algorithms.h"

#include <math.h>


typedef struct
{
	// Whether to use the IsBlocked func to check visibility,
	// and also whether to early-terminate if the line is blocked
	bool CheckBlockedAndEarlyTerminate;
	bool (*IsBlocked)(void *, struct vec2i);
	void (*OnPoint)(void *, struct vec2i);
	void *data;
} AlgoLineData;
static bool BresenhamLine(struct vec2i from, struct vec2i to, AlgoLineData *data)
{
	struct vec2i d = svec2i(abs(to.x - from.x), abs(to.y - from.y));
	struct vec2i s = svec2i(from.x < to.x ? 1 : -1, from.y < to.y ? 1 : -1);
	int err = d.x - d.y;
	struct vec2i v = from;
	for (;;)
	{
		int e2 = 2 * err;
		if (svec2i_is_equal(v, to))
		{
			break;
		}
		if (data->CheckBlockedAndEarlyTerminate)
		{
			if (data->IsBlocked(data->data, v))
			{
				return false;
			}
		}
		else
		{
			data->OnPoint(data->data, v);
		}
		if (e2 > -d.y)
		{
			err -= d.y;
			v.x += s.x;
		}
		if (svec2i_is_equal(v, to))
		{
			break;
		}
		if (e2 < d.x)
		{
			err += d.x;
			v.y += s.y;
		}
	}
	if (data->CheckBlockedAndEarlyTerminate)
	{
		return !data->IsBlocked(data->data, v);
	}
	else
	{
		data->OnPoint(data->data, v);
	}
	return true;
}

// From "Raytracing on a grid" by James McNeill
// http://playtechs.blogspot.com/2007/03/raytracing-on-grid.html
// "The code snippets are free to use as you see fit and no attribution is
// necessary"
static bool JMRaytrace(int x0, int y0, int x1, int y1, AlgoLineData *data)
{
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int x = x0;
	int y = y0;
	int n = 1 + dx + dy;
	int x_inc = (x1 > x0) ? 1 : -1;
	int y_inc = (y1 > y0) ? 1 : -1;
	int error = dx - dy;
	dx *= 2;
	dy *= 2;

	for (; n > 0; --n)
	{
		const struct vec2i v = svec2i(x, y);
		if (data->CheckBlockedAndEarlyTerminate)
		{
			if (data->IsBlocked(data->data, v))
			{
				return false;
			}
		}
		else
		{
			data->OnPoint(data->data, v);
		}

		if (error > 0)
		{
			x += x_inc;
			error -= dy;
		}
		else
		{
			y += y_inc;
			error += dx;
		}
	}
	return true;
}

bool HasClearLineBresenham(struct vec2i from, struct vec2i to, HasClearLineData *data)
{
	AlgoLineData bData;
	bData.CheckBlockedAndEarlyTerminate = true;
	bData.IsBlocked = data->IsBlocked;
	bData.data = data->data;
	return BresenhamLine(from, to, &bData);
}
bool HasClearLineJMRaytrace(
	const struct vec2i from, const struct vec2i to, HasClearLineData *data)
{
	AlgoLineData bData;
	bData.CheckBlockedAndEarlyTerminate = true;
	bData.IsBlocked = data->IsBlocked;
	bData.data = data->data;
	return JMRaytrace(from.x, from.y, to.x, to.y, &bData);
}


void BresenhamLineDraw(struct vec2i from, struct vec2i to, AlgoLineDrawData *data)
{
	AlgoLineData bData;
	bData.CheckBlockedAndEarlyTerminate = false;
	bData.OnPoint = data->Draw;
	bData.data = data->data;
	BresenhamLine(from, to, &bData);
}
void JMRaytraceLineDraw(
	const struct vec2i from, const struct vec2i to, AlgoLineDrawData *data)
{
	AlgoLineData bData;
	bData.CheckBlockedAndEarlyTerminate = false;
	bData.OnPoint = data->Draw;
	bData.data = data->data;
	JMRaytrace(from.x, from.y, to.x, to.y, &bData);
}

bool CFloodFill(struct vec2i v, FloodFillData *data)
{
	if (data->IsSame(data->data, v))
	{
		data->Fill(data->data, v);
		CFloodFill(svec2i(v.x - 1, v.y), data);
		CFloodFill(svec2i(v.x + 1, v.y), data);
		CFloodFill(svec2i(v.x, v.y - 1), data);
		CFloodFill(svec2i(v.x, v.y + 1), data);
		return true;
	}
	return false;
}
