/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2016, Cong Xu
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
#include "editor_ui_chared.h"

#include <cdogs/font.h>


UIObject *CreateCharEditorObjs(void)
{
	const int th = FontH();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int x;
	int y;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());

	// Appearance

	y = 10;
	o = UIObjectCreate(
		UITYPE_NONE, YC_APPEARANCE, Vec2iZero(), Vec2iNew(40, th));

	x = 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FACE;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SKIN;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_HAIR;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_BODY;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_ARMS;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_LEGS;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);

	// Character attributes

	y += th;
	UIObjectDestroy(o);
	o = UIObjectCreate(
		UITYPE_NONE, YC_ATTRIBUTES, Vec2iZero(), Vec2iNew(40, th));

	x = 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SPEED;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_HEALTH;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_MOVE;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_TRACK;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip,
		"Looking towards the player\n"
		"Useless for friendly characters");
	UIObjectAddChild(c, o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SHOOT;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_DELAY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Frames before making another decision");
	UIObjectAddChild(c, o2);

	// Character flags

	y += th;
	UIObjectDestroy(o);
	o = UIObjectCreate(UITYPE_NONE, YC_FLAGS, Vec2iZero(), Vec2iNew(40, th));

	x = 5;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_ASBESTOS;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_IMMUNITY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Immune to poison");
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SEETHROUGH;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_RUNS_AWAY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Runs away from player");
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SNEAKY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Shoots back when player shoots");
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_GOOD_GUY;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SLEEPING;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Doesn't move unless seen");
	UIObjectAddChild(c, o2);

	y += th;
	x = 5;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_PRISONER;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Doesn't move until touched");
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_INVULNERABLE;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FOLLOWER;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_PENALTY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Large score penalty when shot");
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_VICTIM;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Takes damage from everyone");
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_AWAKE;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);

	// Weapon

	y += th;
	UIObjectDestroy(o);
	o = UIObjectCreate(
		UITYPE_NONE, YC_WEAPON, Vec2iNew(50, y), Vec2iNew(210, th));
	UIObjectAddChild(c, o);

	return c;
}
