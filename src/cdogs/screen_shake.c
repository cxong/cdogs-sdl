/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2014, 2019 Cong Xu
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
#include "screen_shake.h"

#include <string.h>

#include "config.h"
#include "sys_config.h"

#define MAX_SHAKE (100 * ConfigGetInt(&gConfig, "Game.FPS") / 100)
#define SHAKE_STANDARD (70 * 1 * ConfigGetInt(&gConfig, "Game.FPS") / 100)


ScreenShake ScreenShakeZero(void)
{
	ScreenShake s;
	memset(&s, 0, sizeof s);
	return s;
}

ScreenShake ScreenShakeAdd(ScreenShake s, int force, int multiplier)
{
	const int extra =
		force * multiplier * ConfigGetInt(&gConfig, "Game.FPS") / 100;
	s.ticks += extra;
	/* So we don't shake too much :) */
	s.ticks = MIN(s.ticks, MAX_SHAKE);
	return s;
}

// Convert ticks left to a shake delta
#define TICKS_TO_DELTA_RATIO 28

ScreenShake ScreenShakeUpdate(ScreenShake s, int ticks)
{
	s.ticks -= ticks;
	s.ticks = MAX(s.ticks, 0);
	// Update delta
	const int maxDelta = s.ticks * TICKS_TO_DELTA_RATIO / SHAKE_STANDARD;
	s.Delta =
		maxDelta == 0 ?
		svec2_zero() :
		svec2(RAND_FLOAT(0, maxDelta), RAND_FLOAT(0, maxDelta));
	return s;
}
