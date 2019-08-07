/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003-2007 Lucas Martin-King

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

    Copyright (c) 2013-2019 Cong Xu
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
#include "weapon.h"


Weapon WeaponCreate(const WeaponClass *wc)
{
	Weapon w;
	memset(&w, 0, sizeof w);
	w.Gun = wc;
	w.state = GUNSTATE_READY;
	w.lock = 0;
	w.soundLock = 0;
	w.stateCounter = -1;
	return w;
}

void WeaponUpdate(Weapon *w, const int ticks)
{
	w->lock -= ticks;
	if (w->lock < 0)
	{
		w->lock = 0;
	}
	w->soundLock -= ticks;
	if (w->soundLock < 0)
	{
		w->soundLock = 0;
	}
	w->clickLock -= ticks;
	if (w->clickLock < 0)
	{
		w->clickLock = 0;
	}
	if (w->stateCounter >= 0)
	{
		w->stateCounter = MAX(0, w->stateCounter - ticks);
		if (w->stateCounter == 0)
		{
			switch (w->state)
			{
			case GUNSTATE_FIRING:
				WeaponSetState(w, GUNSTATE_RECOIL);
				break;
			case GUNSTATE_RECOIL:
				WeaponSetState(w, GUNSTATE_FIRING);
				break;
			default:
				// do nothing
				break;
			}
		}
	}
	w->heatCounter -= ticks;
	if (w->heatCounter < 0)
	{
		w->heatCounter = 0;
	}
}

bool WeaponIsLocked(const Weapon *w)
{
	return w->lock > 0;
}

void WeaponSetState(Weapon *w, const gunstate_e state)
{
	w->state = state;
	switch (state)
	{
	case GUNSTATE_FIRING:
		w->stateCounter = 4;
		break;
	case GUNSTATE_RECOIL:
		// This is to make sure the gun stays recoiled as long as the gun is
		// "locked", i.e. cannot fire
		w->stateCounter = MAX(1, w->lock - 3);
		break;
	default:
		w->stateCounter = -1;
		break;
	}
}

void WeaponOnFire(Weapon *w)
{
	if (w->soundLock <= 0)
	{
		w->soundLock = w->Gun->SoundLockLength;
	}

	w->heatCounter += w->Gun->Lock * 2;
	// 2 seconds of overheating max
	if (w->heatCounter > FPS_FRAMELIMIT * 3)
	{
		w->heatCounter = FPS_FRAMELIMIT * 3;
	}
	w->lock = w->Gun->Lock;
}

bool WeaponIsOverheating(const Weapon *w)
{
	// Overheat after 1 second of continuous firing
	return w->heatCounter > FPS_FRAMELIMIT;
}
