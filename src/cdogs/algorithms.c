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
#include "algorithms.h"

#include <math.h>


static bool IsTileBlocked(int x, int y, double factor, HasClearLineData *data)
{
	if (factor > 0.4)
	{
		return data->IsBlocked(data->data, Vec2iNew(x, y));
	}
	return false;
}

typedef struct
{
	// Whether to use the IsBlocked func to check visibility,
	// and also whether to early-terminate if the line is blocked
	bool CheckBlockedAndEarlyTerminate;
	bool (*IsBlocked)(void *, Vec2i);
	void (*Draw)(void *, Vec2i);
	void *data;
} BresenhamLineData;
static bool BresenhamLine(Vec2i from, Vec2i to, BresenhamLineData *data)
{
	Vec2i d = Vec2iNew(abs(to.x - from.x), abs(to.y - from.y));
	Vec2i s = Vec2iNew(from.x < to.x ? 1 : -1, from.y < to.y ? 1 : -1);
	int err = d.x - d.y;
	Vec2i v = from;
	for (;;)
	{
		int e2 = 2 * err;
		if (Vec2iEqual(v, to))
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
			data->Draw(data->data, v);
		}
		if (e2 > -d.y)
		{
			err -= d.y;
			v.x += s.x;
		}
		if (Vec2iEqual(v, to))
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
		data->Draw(data->data, v);
	}
	return true;
}

bool HasClearLineBresenham(Vec2i from, Vec2i to, HasClearLineData *data)
{
	BresenhamLineData bData;
	bData.CheckBlockedAndEarlyTerminate = true;
	bData.IsBlocked = data->IsBlocked;
	bData.data = data->data;
	return BresenhamLine(from, to, &bData);
}

void BresenhamLineDraw(Vec2i from, Vec2i to, BresenhamLineDrawData *data)
{
	BresenhamLineData bData;
	bData.CheckBlockedAndEarlyTerminate = false;
	bData.Draw = data->Draw;
	bData.data = data->data;
	BresenhamLine(from, to, &bData);
}

bool FloodFill(Vec2i v, FloodFillData *data)
{
	if (data->IsSame(data->data, v))
	{
		data->Fill(data->data, v);
		FloodFill(Vec2iNew(v.x - 1, v.y), data);
		FloodFill(Vec2iNew(v.x + 1, v.y), data);
		FloodFill(Vec2iNew(v.x, v.y - 1), data);
		FloodFill(Vec2iNew(v.x, v.y + 1), data);
		return true;
	}
	return false;
}
