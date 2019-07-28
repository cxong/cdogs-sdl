/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2017, 2019 Cong Xu
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
#include "health_gauge.h"

#include "gamedata.h"
#include "gauge.h"

#define WAIT_MS 1000


void HealthGaugeInit(HealthGauge *h)
{
	h->health = 0;
	h->waitHealth = 0;
	h->waitMs = 0;
}

void HealthGaugeUpdate(HealthGauge *h, const TActor *a, const int ms)
{
	// Initialise health to actor's health
	if (h->health == 0)
	{
		h->health = a->health;
		h->waitHealth = a->health;
	}
	h->waitMs = MAX(0, h->waitMs - ms);
	if (h->waitHealth != a->health)
	{
		// Health has changed; start waiting
		h->waitHealth = a->health;
		h->waitMs = WAIT_MS;
	}
	else if (h->waitMs == 0)
	{
		// End of wait; start moving health towards real health
		if (h->health != a->health)
		{
			const int d = SIGN(a->health - h->health);
			h->health += d;
		}
	}
}

void HealthGaugeDraw(
	const HealthGauge *h, GraphicsDevice *device, const TActor *actor,
	const struct vec2i pos, const int width, const FontOpts opts,
	const color_t mask)
{
	// Draw gauge background
	HUDDrawGauge(device, &gPicManager, pos, width, 0, colorTransparent, mask);
	if (actor == NULL)
	{
		return;
	}

	HSV hsv = { 0.0, 1.0, 1.0 };
	const int health = actor->health;
	const int maxHealth = ActorGetCharacter(actor)->maxHealth;
	if (actor->poisoned)
	{
		hsv.h = 120.0;
		hsv.v = 0.5;
	}
	else
	{
		double maxHealthHue = 50.0;
		double minHealthHue = 0.0;
		hsv.h =
			(maxHealthHue - minHealthHue) * health / maxHealth + minHealthHue;
	}
	color_t barColor;
	if (h->health != health)
	{
		// Draw different-coloured health gauge representing health change
		const int higherHealth = MAX(h->health, health);
		const int innerWidthUpdate = MAX(1, width * higherHealth / maxHealth);
		barColor = h->health > health ? colorMaroon : colorGreen;
		barColor.a = opts.Mask.a;
		HUDDrawGaugeInner(device, &gPicManager, pos, innerWidthUpdate, barColor);
	}
	const int lowerHealth = MIN(h->health, health);
	const int innerWidth = MAX(1, width * lowerHealth / maxHealth);
	barColor = ColorTint(colorWhite, hsv);
	barColor.a = opts.Mask.a;
	HUDDrawGaugeInner(device, &gPicManager, pos, innerWidth, barColor);

	// Draw health number label
	char s[50];
	sprintf(s, "%d", health);
	FontOpts fOpts = opts;
	fOpts.Area = svec2i(width, 11);
	fOpts.Pad = svec2i(2, 1);
	// If low health, draw text with different colours, flashing
	if (ActorIsLowHealth(actor))
	{
		// Fast flashing
		const int fps = ConfigGetInt(&gConfig, "Game.FPS");
		const int pulsePeriod = fps / 4;
		if ((gMission.time % pulsePeriod) < (pulsePeriod / 2))
		{
			fOpts.Mask = colorRed;
		}
	}
	FontStrOpt(s, pos, fOpts);
}
