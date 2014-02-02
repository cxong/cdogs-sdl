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


static void Swap(int *x, int *y)
{
	int temp = *x;
	*x = *y;
	*y = temp;
}

// Floating point part of a number
static double FPart(double x)
{
	return x - floor(x);
}

// Reciprocal of the floating point part of a number
static double RFPart(double x)
{
	return 1 - FPart(x);
}

static bool IsTileBlocked(int x, int y, double factor, HasClearLineData *data)
{
	if (factor > 0.4)
	{
		return data->IsBlocked(data->data, Vec2iNew(x, y));
	}
	return false;
}

bool HasClearLine(Vec2i from, Vec2i to, HasClearLineData *data)
{
	// Find all tiles that overlap with the line (from, to)
	// Uses a modified version of Xiaolin Wu's algorithm
	Vec2i delta;
	double gradient;
	Vec2i end;
	double xGap;
	Vec2i tileStart, tileEnd;
	double yIntercept;
	int x;
	int w = data->tileSize.x;
	int h = data->tileSize.y;
	int isSteep = abs(to.y - from.y) > abs(to.x - from.x);
	if (isSteep)
	{
		// Swap x and y
		// Note that this prevents the vertical line special case
		Swap(&from.x, &from.y);
		Swap(&to.x, &to.y);
		Swap(&w, &h);
	}
	if (from.x > to.x)
	{
		// swap to make sure we always go left to right
		Swap(&from.x, &to.x);
		Swap(&from.y, &to.y);
	}

	delta.x = to.x - from.x;
	delta.y = to.y - from.y;
	gradient = (double)delta.y / delta.x;

	// handle first endpoint
	end.x = from.x / w;
	end.y = (int)((from.y + gradient * (end.x * w - from.x)) / h);
	xGap = RFPart(from.x + 0.5);
	tileStart.x = end.x;
	tileStart.y = end.y;
	if (isSteep)
	{
		if (IsTileBlocked(tileStart.y, tileStart.x, RFPart(end.y) * xGap, data))
		{
			return false;
		}
		if (IsTileBlocked(tileStart.y + 1, tileStart.x, FPart(end.y) * xGap, data))
		{
			return false;
		}
	}
	else
	{
		if (IsTileBlocked(tileStart.x, tileStart.y, RFPart(end.y) * xGap, data))
		{
			return false;
		}
		if (IsTileBlocked(tileStart.x, tileStart.y + 1, FPart(end.y) * xGap, data))
		{
			return false;
		}
	}
	yIntercept = end.y + gradient;

	// handle second endpoint
	end.x = to.x / w;
	end.y = (int)((to.y + gradient * (end.x * w - to.x)) / h);
	xGap = FPart(to.x + 0.5);
	tileEnd.x = end.x;
	tileEnd.y = end.y;
	if (isSteep)
	{
		if (IsTileBlocked(tileEnd.y, tileEnd.x, RFPart(end.y) * xGap, data))
		{
			return false;
		}
		if (IsTileBlocked(tileEnd.y + 1, tileEnd.x, FPart(end.y) * xGap, data))
		{
			return false;
		}
	}
	else
	{
		if (IsTileBlocked(tileEnd.x, tileEnd.y, RFPart(end.y) * xGap, data))
		{
			return false;
		}
		if (IsTileBlocked(tileEnd.x, tileEnd.y + 1, FPart(end.y) * xGap, data))
		{
			return false;
		}
	}

	// main loop
	for (x = tileStart.x + 1; x < tileEnd.x; x++)
	{
		if (isSteep)
		{
			if (IsTileBlocked((int)yIntercept, x, RFPart(yIntercept), data))
			{
				return false;
			}
			if (IsTileBlocked((int)yIntercept + 1, x, FPart(yIntercept), data))
			{
				return false;
			}
		}
		else
		{
			if (IsTileBlocked(x, (int)yIntercept, RFPart(yIntercept), data))
			{
				return false;
			}
			if (IsTileBlocked(x, (int)yIntercept + 1, FPart(yIntercept), data))
			{
				return false;
			}
		}
		yIntercept += gradient;
	}

	return true;
}

void BresenhamLine(Vec2i start, Vec2i end, BresenhamLineData *data)
{
	// Bresenham's line algorithm
	Vec2i d = Vec2iNew(abs(end.x - start.x), abs(end.y - start.y));
	Vec2i s = Vec2iNew(start.x < end.x ? 1 : -1, start.y < end.y ? 1 : -1);
	int err = d.x - d.y;
	Vec2i v = start;
	for (;;)
	{
		int e2 = 2 * err;
		if (Vec2iEqual(v, end))
		{
			break;
		}
		data->Draw(data->data, v);
		if (e2 > -d.y)
		{
			err -= d.y;
			v.x += s.x;
		}
		if (Vec2iEqual(v, end))
		{
			break;
		}
		if (e2 < d.x)
		{
			err += d.x;
			v.y += s.y;
		}
	}
	data->Draw(data->data, v);
}
