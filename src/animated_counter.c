/*
    Copyright (c) 2018-2019 Cong Xu
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
#include "animated_counter.h"

#include <math.h>
#include <cdogs/font.h>

#define INC_RATIO 0.01f


AnimatedCounter AnimatedCounterNew(const char *prefix, const int max)
{
	AnimatedCounter a = { NULL, max, 0 };
	CSTRDUP(a.prefix, prefix);
	return a;
}
void AnimatedCounterTerminate(AnimatedCounter *a)
{
	CFREE(a->prefix);
}

void AnimatedCounterUpdate(AnimatedCounter *a, const int ticks)
{
	if (a->max == 0)
	{
		return;
	}
	float inc = a->max * INC_RATIO;
	for (int i = 0; i < ticks; i++)
	{
		const int diff = a->max - a->current;
		while (a->max > 0 ? inc > diff : inc < diff)
		{
			inc *= INC_RATIO;
		}
		a->current += (int)(a->max > 0 ? ceil(inc) : floor(inc));
	}
}
void AnimatedCounterDraw(const AnimatedCounter *a, const struct vec2i pos)
{
	const struct vec2i pos2 = FontStr(a->prefix, pos);
	char buf[256];
	sprintf(buf, "%d", a->current);
	FontStr(buf, pos2);
}
