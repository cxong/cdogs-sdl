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
#include "draw_highlight.h"

#include "draw_actor.h"
#include "gamedata.h"
#include "pickup.h"


static void DrawObjectiveHighlight(
	TTileItem *ti, Tile *tile, DrawBuffer *b, Vec2i offset);
void DrawObjectiveHighlights(DrawBuffer *b, const Vec2i offset)
{
	if (!ConfigGetBool(&gConfig, "Graphics.ShowHUD"))
	{
		return;
	}

	Tile *tile = &b->tiles[0][0];
	for (int y = 0; y < Y_TILES; y++)
	{
		for (int x = 0; x < b->Size.x; x++, tile++)
		{
			// Draw the items that are in LOS
			CA_FOREACH(ThingId, tid, tile->things)
				TTileItem *ti = ThingIdGetTileItem(tid);
				DrawObjectiveHighlight(ti, tile, b, offset);
			CA_FOREACH_END()
		}
		tile += X_TILES - b->Size.x;
	}
}
static void DrawObjectiveHighlight(
	TTileItem *ti, Tile *tile, DrawBuffer *b, Vec2i offset)
{
	color_t color;
	if (ti->flags & TILEITEM_OBJECTIVE)
	{
		// Objective
		const int objective = ObjectiveFromTileItem(ti->flags);
		const Objective *o =
			CArrayGet(&gMission.missionData->Objectives, objective);
		if (o->Flags & OBJECTIVE_HIDDEN)
		{
			return;
		}
		if (!(o->Flags & OBJECTIVE_POSKNOWN) &&
			(tile->flags & MAPTILE_OUT_OF_SIGHT))
		{
			return;
		}
		color = o->color;
	}
	else if (ti->kind == KIND_PICKUP)
	{
		// Gun pickup
		const Pickup *p = CArrayGet(&gPickups, ti->id);
		if (!PickupIsManual(p))
		{
			return;
		}
		color = colorDarker;
	}
	else
	{
		return;
	}
	
	const Vec2i pos = Vec2iNew(
		ti->x - b->xTop + offset.x, ti->y - b->yTop + offset.y);
	const int pulsePeriod = ConfigGetInt(&gConfig, "Game.FPS");
	int alphaUnscaled =
		(gMission.time % pulsePeriod) * 255 / (pulsePeriod / 2);
	if (alphaUnscaled > 255)
	{
		alphaUnscaled = 255 * 2 - alphaUnscaled;
	}
	color.a = (Uint8)alphaUnscaled;
	if (ti->getPicFunc != NULL)
	{
		Vec2i picOffset;
		const Pic *pic = ti->getPicFunc(ti->id, &picOffset);
		BlitPicHighlight(
			&gGraphicsDevice, pic, Vec2iAdd(pos, picOffset), color);
	}
	else if (ti->kind == KIND_CHARACTER)
	{
		TActor *a = CArrayGet(&gActors, ti->id);
		ActorPics pics = GetCharacterPicsFromActor(a);
		DrawActorHighlight(&pics, pos, color);
	}
}
