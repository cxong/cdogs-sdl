/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2017, 2019 Cong Xu
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
#include "animation.h"

#include "defs.h"


static Animation animIdling =
{
	ACTORANIMATION_IDLE,
	0,
	{ IDLEHEAD_NORMAL, IDLEHEAD_LEFT, IDLEHEAD_RIGHT },
	{ 90, 60, 60 },
	0,
	true,
	true
};
static Animation animWalking =
{
	ACTORANIMATION_WALKING,
	0,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 4, 4, 4, 4, 4, 4, 4, 4 },
	0,
	false,
	true
};


Animation AnimationGetActorAnimation(const ActorAnimation aa)
{
	switch (aa)
	{
	case ACTORANIMATION_IDLE: return animIdling;
	case ACTORANIMATION_WALKING: return animWalking;
	default: CASSERT(false, "Unknown actor animation"); return animIdling;
	}
}

void AnimationUpdate(Animation *a, const float ticks)
{
	a->frameCounter += ticks;
	a->newFrame = false;
	if (a->frameCounter > a->ticksPerFrame[a->frame])
	{
		a->frameCounter -= a->ticksPerFrame[a->frame];
		a->newFrame = true;
		// Advance to next frame
		if (a->randomFrames)
		{
			// If we're not on the first frame, return to first frame
			// Otherwise, pick a random non-first frame
			if (a->frame == 0)
			{
				// Note: -1 means frame not used, so pick another frame
				do
				{
					a->frame = (rand() % (ANIMATION_MAX_FRAMES - 1)) + 1;
				} while (a->ticksPerFrame[a->frame] < 0);
			}
			else
			{
				a->frame = 0;
			}
		}
		else
		{
			a->frame++;
			if (a->frame >= ANIMATION_MAX_FRAMES ||
				a->ticksPerFrame[a->frame] < 0)
			{
				a->frame = 0;
			}
		}
	}
}
int AnimationGetFrame(const Animation *a)
{
	return a->frames[a->frame];
}
