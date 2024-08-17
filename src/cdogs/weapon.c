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

	Copyright (c) 2013-2019, 2021, 2024 Cong Xu
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
	if (wc != NULL)
	{
		for (int i = 0; i < WeaponClassNumBarrels(wc); i++)
		{
			w.barrels[i].state = GUNSTATE_READY;
			w.barrels[i].stateCounter = -1;
		}
	}
	return w;
}

void WeaponUpdate(Weapon *w, const int ticks)
{
	w->clickLock -= ticks;
	if (w->clickLock < 0)
	{
		w->clickLock = 0;
	}
	for (int i = 0; i < WeaponClassNumBarrels(w->Gun); i++)
	{
		w->barrels[i].lock -= ticks;
		if (w->barrels[i].lock < 0)
		{
			w->barrels[i].lock = 0;
		}
		w->barrels[i].soundLock -= ticks;
		if (w->barrels[i].soundLock < 0)
		{
			w->barrels[i].soundLock = 0;
		}
		if (w->barrels[i].stateCounter >= 0)
		{
			w->barrels[i].stateCounter =
				MAX(0, w->barrels[i].stateCounter - ticks);
			if (w->barrels[i].stateCounter == 0)
			{
				switch (w->barrels[i].state)
				{
				case GUNSTATE_FIRING:
					WeaponBarrelSetState(w, i, GUNSTATE_RECOIL);
					break;
				case GUNSTATE_RECOIL:
					WeaponBarrelSetState(w, i, GUNSTATE_FIRING);
					break;
				default:
					// do nothing
					break;
				}
			}
		}
		w->barrels[i].heatCounter -= ticks;
		if (w->barrels[i].heatCounter < 0)
		{
			w->barrels[i].heatCounter = 0;
		}
	}
}

int WeaponGetUnlockedBarrel(const Weapon *w)
{
	// To fire:
	// One barrel has lock == 0
	// No other barrel has lock > (gun lock - inter-gun lock)
	int unlockedBarrel = -1;
	for (int i = 0; i < WeaponClassNumBarrels(w->Gun); i++)
	{
		if (w->barrels[i].lock == 0 && unlockedBarrel < 0)
		{
			unlockedBarrel = i;
		}
		else if (
			w->barrels[i].lock >
			(WeaponClassGetBarrel(w->Gun, i)->Lock - w->Gun->Lock))
		{
			return -1;
		}
	}
	return unlockedBarrel;
}

void WeaponBarrelSetState(Weapon *w, const int barrel, const gunstate_e state)
{
	w->barrels[barrel].state = state;
	switch (state)
	{
	case GUNSTATE_FIRING:
		w->barrels[barrel].stateCounter = 4;
		break;
	case GUNSTATE_RECOIL:
		// This is to make sure the gun stays recoiled as long as the gun is
		// "locked", i.e. cannot fire
		w->barrels[barrel].stateCounter = MAX(1, w->barrels[barrel].lock - 3);
		break;
	default:
		w->barrels[barrel].stateCounter = -1;
		break;
	}
}

void WeaponBarrelOnFire(Weapon *w, const int barrel)
{
	const WeaponClass *wc = WeaponClassGetBarrel(w->Gun, barrel);
	if (w->barrels[barrel].soundLock <= 0)
	{
		w->barrels[barrel].soundLock = wc->u.Normal.SoundLockLength;
	}

	w->barrels[barrel].heatCounter += wc->Lock * 2;
	// 2 seconds of overheating max
	if (w->barrels[barrel].heatCounter > FPS_FRAMELIMIT * 3)
	{
		w->barrels[barrel].heatCounter = FPS_FRAMELIMIT * 3;
	}
	w->barrels[barrel].lock = wc->Lock;
}

bool WeaponBarrelIsOverheating(const Weapon *w, const int idx)
{
	if (!WeaponClassHasMuzzle(w->Gun) || w->Gun->OverheatTicks < 0)
	{
		return false;
	}
	// Overheat after some duration of continuous firing
	return w->barrels[idx].heatCounter > w->Gun->OverheatTicks;
}
