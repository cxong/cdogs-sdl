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


typedef struct
{
	// Whether to use the IsBlocked func to check visibility,
	// and also whether to early-terminate if the line is blocked
	bool CheckBlockedAndEarlyTerminate;
	bool (*IsBlocked)(void *, Vec2i);
	void (*Draw)(void *, Vec2i);
	void *data;
} AlgoLineData;
static bool BresenhamLine(Vec2i from, Vec2i to, AlgoLineData *data)
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

static void Vec2iSwap(Vec2i *a, Vec2i *b);
static bool XiaolinWuDraw(
	const Vec2i a, const Vec2i b,
	const double aa, const bool isEnd, AlgoLineData *data);
static bool XiaolinWuLine(Vec2i from, Vec2i to, AlgoLineData *data)
{
	if (Vec2iEqual(from, to))
	{
		return true;
	}
	if (from.x == to.x || from.y == to.y)
	{
		return BresenhamLine(from, to, data);
	}
	const double dx = to.x - from.x;
	const double dy = to.y - from.y;
	bool swapped = false;
	double gradient;
	double xyend;
	Vec2i p1, p2;
	double interx = 0;
	double intery = 0;
	Vec2i d = Vec2iZero();
	bool shallow = fabs(dx) > fabs(dy);
	if (shallow)
	{
		if (to.x < from.x)
		{
			Vec2iSwap(&from, &to);
			swapped = true;
		}
		gradient = dy / dx;
		int xend = from.x;
		xyend = from.y + gradient*(xend - from.x);
		p1 = Vec2iNew(xend, (int)xyend);
		intery = xyend + gradient;

		xend = to.x;
		xyend = to.y + gradient*(xend - to.x);
		p2 = Vec2iNew(xend, (int)xyend);

		d.x = swapped ? -1 : 1;
		if (swapped)
		{
			intery = xyend - gradient;
		}
	}
	else
	{
		if (to.y < from.y)
		{
			Vec2iSwap(&from, &to);
			swapped = true;
		}
		gradient = dx / dy;
		int yend = from.y;
		xyend = from.x + gradient*(yend - from.y);
		p1 = Vec2iNew((int)xyend, yend);
		interx = xyend + gradient;

		yend = to.y;
		xyend = to.x + gradient*(yend - to.y);
		p2 = Vec2iNew((int)xyend, yend);

		d.y = swapped ? -1 : 1;
		if (swapped)
		{
			interx = xyend - gradient;
		}
	}

	const Vec2i pstart = swapped ? p2 : p1;
	if (!XiaolinWuDraw(
		pstart, Vec2iNew(pstart.x, pstart.y + 1), xyend, true, data))
	{
		return false;
	}

	Vec2i start = Vec2iAdd(swapped ? p2 : p1, d);
	Vec2i end = Vec2iMinus(swapped ? p1 : p2, d);
	const Vec2i dp = shallow ? Vec2iNew(0, 1) : Vec2iNew(1, 0);
	for (Vec2i xy = start;; xy = Vec2iAdd(xy, d))
	{
		if (shallow)
		{
			if (swapped ? xy.x < end.x : xy.x > end.x)
			{
				break;
			}
		}
		else
		{
			if (swapped ? xy.y < end.y : xy.y > end.y)
			{
				break;
			}
		}
		const Vec2i p = Vec2iNew(
			shallow ? xy.x : (int)interx,
			shallow ? (int)intery : xy.y);
		if (!XiaolinWuDraw(
			p, Vec2iAdd(p, dp), shallow ? intery : interx, false, data))
		{
			return false;
		}
		if (shallow)
		{
			intery += swapped ? -gradient : gradient;
		}
		else
		{
			interx += swapped ? -gradient : gradient;
		}
	}

	const Vec2i pend = swapped ? p1 : p2;
	if (!XiaolinWuDraw(
		pend, Vec2iNew(pend.x, pend.y + 1), xyend, true, data))
	{
		return false;
	}
	return true;
}
static void Vec2iSwap(Vec2i *a, Vec2i *b)
{
	Vec2i temp = *b;
	*b = *a;
	*a = temp;
}
#define FPART(_x) ((double)(_x) - (int)(_x))
#define RFPART(_x) (1.0 - FPART(_x))
#define AAFACTOR -0.1
static bool XiaolinWuDraw(
	const Vec2i a, const Vec2i b,
	const double aa, const bool isEnd, AlgoLineData *data)
{
	if (data->CheckBlockedAndEarlyTerminate)
	{
		if ((RFPART(aa) > AAFACTOR && data->IsBlocked(data->data, a)) ||
			(FPART(aa) > AAFACTOR && !isEnd && data->IsBlocked(data->data, b)))
		{
			return false;
		}
	}
	else
	{
		if (RFPART(aa) > AAFACTOR) data->Draw(data->data, a);
		if (FPART(aa) > AAFACTOR && !isEnd) data->Draw(data->data, b);
	}
	return true;
}

bool HasClearLineBresenham(Vec2i from, Vec2i to, HasClearLineData *data)
{
	AlgoLineData bData;
	bData.CheckBlockedAndEarlyTerminate = true;
	bData.IsBlocked = data->IsBlocked;
	bData.data = data->data;
	return BresenhamLine(from, to, &bData);
}
bool HasClearLineXiaolinWu(Vec2i from, Vec2i to, HasClearLineData *data)
{
	AlgoLineData bData;
	bData.CheckBlockedAndEarlyTerminate = true;
	bData.IsBlocked = data->IsBlocked;
	bData.data = data->data;
	return XiaolinWuLine(from, to, &bData);
}


void BresenhamLineDraw(Vec2i from, Vec2i to, AlgoLineDrawData *data)
{
	AlgoLineData bData;
	bData.CheckBlockedAndEarlyTerminate = false;
	bData.Draw = data->Draw;
	bData.data = data->data;
	BresenhamLine(from, to, &bData);
}
void XiaolinWuLineDraw(Vec2i from, Vec2i to, AlgoLineDrawData *data)
{
	AlgoLineData bData;
	bData.CheckBlockedAndEarlyTerminate = false;
	bData.Draw = data->Draw;
	bData.data = data->data;
	XiaolinWuLine(from, to, &bData);
}

bool CFloodFill(Vec2i v, FloodFillData *data)
{
	if (data->IsSame(data->data, v))
	{
		data->Fill(data->data, v);
		CFloodFill(Vec2iNew(v.x - 1, v.y), data);
		CFloodFill(Vec2iNew(v.x + 1, v.y), data);
		CFloodFill(Vec2iNew(v.x, v.y - 1), data);
		CFloodFill(Vec2iNew(v.x, v.y + 1), data);
		return true;
	}
	return false;
}
