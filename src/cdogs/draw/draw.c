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

    Copyright (c) 2013-2016, 2018-2019 Cong Xu
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
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "actors.h"
#include "algorithms.h"
#include "config.h"
#include "draw/draw_actor.h"
#include "draw/drawtools.h"
#include "font.h"
#include "game_events.h"
#include "net_util.h"
#include "objs.h"
#include "pickup.h"
#include "pics.h"
#include "draw/draw.h"
#include "blit.h"
#include "pic_manager.h"

//#define DEBUG_DRAW_HITBOXES


// Three types of tile drawing, based on line of sight:
// Unvisited: black
// Out of sight: dark, or if fog disabled, black
// In sight: full color
typedef enum
{
	TILE_LOS_NORMAL,
	TILE_LOS_FOG,
	TILE_LOS_NONE
} TileLOS;
static TileLOS GetTileLOS(const Tile *tile, const bool useFog)
{
	if (!tile->isVisited)
	{
		return TILE_LOS_NONE;
	}
	if (tile->outOfSight)
	{
		return useFog ? TILE_LOS_FOG : TILE_LOS_NONE;
	}
	return TILE_LOS_NORMAL;
}
static color_t GetLOSMask(const Tile *tile, const bool useFog)
{
	switch (GetTileLOS(tile, useFog))
	{
	case TILE_LOS_NORMAL:
		return colorWhite;
	case TILE_LOS_FOG:
		return colorFog;
	case TILE_LOS_NONE:
	default:
		// don't draw
		return colorTransparent;
	}
}
static void DrawLOSPic(
	const Tile *tile, const Pic *pic, const struct vec2i pos,
	const bool useFog)
{
	const color_t mask = GetLOSMask(tile, useFog);
	if (!ColorEquals(mask, colorTransparent))
	{
		PicRender(
			pic, gGraphicsDevice.gameWindow.renderer, pos, mask, 0, svec2_one(),
			SDL_FLIP_NONE, Rect2iZero());
	}
}


static void DrawThing(DrawBuffer *b, const Thing *t, const struct vec2i offset);

static void DrawTiles(
	DrawBuffer *b, const struct vec2i offset,
	void (*drawTileFunc)(
		DrawBuffer *, const struct vec2i, const Tile *, const struct vec2i,
		const bool))
{
	const bool useFog = ConfigGetBool(&gConfig, "Game.Fog");
	const Tile **tile = DrawBufferGetFirstTile(b);
	struct vec2i pos;
	int x, y;
	for (y = 0, pos.y = b->dy + offset.y;
		y < Y_TILES;
		y++, pos.y += TILE_HEIGHT)
	{
		CArrayClear(&b->displaylist);
		for (x = 0, pos.x = b->dx + offset.x;
			x < b->Size.x;
			x++, tile++, pos.x += TILE_WIDTH)
		{
			if (*tile == NULL) continue;
			drawTileFunc(b, offset, *tile, pos, useFog);
		}
		DrawBufferSortDisplayList(b);
		CA_FOREACH(const Thing *, tp, b->displaylist)
			DrawThing(b, *tp, offset);
		CA_FOREACH_END()
		tile += X_TILES - b->Size.x;
	}
}

static void DrawFloor(
	DrawBuffer *b, const struct vec2i offset, const Tile *t,
	const struct vec2i pos, const bool useFog);
static void DrawThingsBelow(
	DrawBuffer *b, const struct vec2i offset, const Tile *t,
	const struct vec2i pos, const bool useFog);
static void DrawWallsAndThings(
	DrawBuffer *b, const struct vec2i offset, const Tile *t,
	const struct vec2i pos, const bool useFog);
static void DrawThingsAbove(
	DrawBuffer *b, const struct vec2i offset, const Tile *t,
	const struct vec2i pos, const bool useFog);
static void DrawObjectiveHighlights(
	DrawBuffer *b, const struct vec2i offset, const Tile *t,
	const struct vec2i pos, const bool useFog);
static void DrawChatters(
	DrawBuffer *b, const struct vec2i offset, const Tile *t,
	const struct vec2i pos, const bool useFog);
static void DrawExtra(DrawBuffer *b, struct vec2i offset, GrafxDrawExtra *extra);

void DrawBufferDraw(DrawBuffer *b, struct vec2i offset, GrafxDrawExtra *extra)
{
	// First draw the floor tiles (which do not obstruct anything)
	DrawTiles(b, offset, DrawFloor);
	// Then draw things that are below everything like debris (wrecks)
	DrawTiles(b, offset, DrawThingsBelow);
	// Now draw walls and (non-wreck) things in proper order
	DrawTiles(b, offset, DrawWallsAndThings);
	// Draw things that are above everything
	DrawTiles(b, offset, DrawThingsAbove);
	if (ConfigGetBool(&gConfig, "Graphics.ShowHUD"))
	{
		// Draw objective highlights, for visible and always-visible objectives
		DrawTiles(b, offset, DrawObjectiveHighlights);
		// Draw actor chatter
		DrawTiles(b, offset, DrawChatters);
	}
	// Draw editor-only things
	if (extra)
	{
		DrawExtra(b, offset, extra);
	}
}

static void DrawFloor(
	DrawBuffer *b, const struct vec2i offset, const Tile *t,
	const struct vec2i pos, const bool useFog)
{
	UNUSED(b);
	UNUSED(offset);
	if (t->Class != NULL &&
		t->Class->Pic != NULL &&
		t->Class->Pic->Data != NULL &&
		t->Class->Type != TILE_CLASS_WALL)
	{
		DrawLOSPic(t, t->Class->Pic, pos, useFog);
	}
}

static void DrawThingsBelow(
	DrawBuffer *b, const struct vec2i offset, const Tile *t,
	const struct vec2i pos, const bool useFog)
{
	UNUSED(pos);
	UNUSED(offset);
	UNUSED(useFog);
	if (t->outOfSight)
	{
		return;
	}
	CA_FOREACH(ThingId, tid, t->things)
		const Thing *ti = ThingIdGetThing(tid);
		if (ThingDrawBelow(ti))
		{
			CArrayPushBack(&b->displaylist, &ti);
		}
	CA_FOREACH_END()
}

#define WALL_OFFSET_Y (-12)
static void DrawWallsAndThings(
	DrawBuffer *b, const struct vec2i offset, const Tile *t,
	const struct vec2i pos, const bool useFog)
{
	UNUSED(offset);
	if (t->Class->Type == TILE_CLASS_WALL)
	{
		DrawLOSPic(
			t, t->Class->Pic, svec2i_add(pos, svec2i(0, WALL_OFFSET_Y)),
			useFog);
	}
	else if (
		t->Class->Type == TILE_CLASS_DOOR && t->ClassAlt && t->ClassAlt->Pic)
	{
		// Drawing doors
		// Doors may be offset; vertical doors are drawn centered
		// horizontal doors are bottom aligned
		struct vec2i doorPos = pos;
		const Pic *pic = t->ClassAlt->Pic;
		doorPos.x += (TILE_WIDTH - pic->size.x) / 2;
		if (pic->size.y > 16)
		{
			doorPos.y += TILE_HEIGHT - (pic->size.y % TILE_HEIGHT);
		}
		DrawLOSPic(t, pic, doorPos, useFog);
	}

	// Draw the items that are in LOS
	if (t->outOfSight)
	{
		return;
	}
	CA_FOREACH(ThingId, tid, t->things)
		const Thing *ti = ThingIdGetThing(tid);
		if (ThingDrawBelow(ti) || ThingDrawAbove(ti))
		{
			continue;
		}
		CArrayPushBack(&b->displaylist, &ti);
	CA_FOREACH_END()
}

static void DrawThingsAbove(
	DrawBuffer *b, const struct vec2i offset, const Tile *t,
	const struct vec2i pos, const bool useFog)
{
	UNUSED(offset);
	UNUSED(pos);
	UNUSED(useFog);
	if (t->outOfSight)
	{
		return;
	}
	CA_FOREACH(ThingId, tid, t->things)
		const Thing *ti = ThingIdGetThing(tid);
		if (ThingDrawAbove(ti))
		{
			CArrayPushBack(&b->displaylist, &ti);
		}
	CA_FOREACH_END()
}

static void DrawObjectiveHighlights(
	DrawBuffer *b, const struct vec2i offset, const Tile *t,
	const struct vec2i pos, const bool useFog)
{
	UNUSED(pos);
	UNUSED(useFog);
	CA_FOREACH(ThingId, tid, t->things)
		Thing *ti = ThingIdGetThing(tid);
		const Pic *pic = NULL;
		color_t color = colorWhite;
		struct vec2i drawOffsetExtra = svec2i_zero();

		if (ti->flags & THING_OBJECTIVE)
		{
			// Objective
			const int objective = ObjectiveFromThing(ti->flags);
			const Objective *o =
				CArrayGet(&gMission.missionData->Objectives, objective);
			if (o->Flags & OBJECTIVE_HIDDEN)
			{
				continue;
			}
			if (!(o->Flags & OBJECTIVE_POSKNOWN) && t->outOfSight)
			{
				continue;
			}
			switch (o->Type)
			{
			case OBJECTIVE_KILL:
			case OBJECTIVE_DESTROY:	// fallthrough
				pic = PicManagerGetPic(&gPicManager, "hud/objective_kill");
				break;
			case OBJECTIVE_RESCUE:
			case OBJECTIVE_COLLECT:	// fallthrough
				pic = PicManagerGetPic(&gPicManager, "hud/objective_collect");
				break;
			default:
				CASSERT(false, "unexpected objective to draw");
				continue;
			}
			color = o->color;
			if (ti->kind == KIND_CHARACTER)
			{
				drawOffsetExtra.y -= 10;
			}
		}
		else if (ti->kind == KIND_PICKUP)
		{
			// Gun pickup
			const Pickup *p = CArrayGet(&gPickups, ti->id);
			if (!PickupIsManual(p))
			{
				continue;
			}
			pic = CPicGetPic(&p->thing.CPic, 0);
			color = colorDarker;
			color.a = (Uint8)Pulse256(gMission.time);
		}

		if (pic != NULL)
		{
			const struct vec2i picPos = svec2i_add(
				svec2i_subtract(
					svec2i_floor(ti->Pos), svec2i(b->xTop, b->yTop)),
				offset);
			color.a = (Uint8)Pulse256(gMission.time);
			// Centre the drawing
			const struct vec2i drawOffset = svec2i_scale_divide(pic->size, -2);
			PicRender(
				pic, gGraphicsDevice.gameWindow.renderer,
				svec2i_add(picPos, svec2i_add(drawOffset, drawOffsetExtra)),
				color, 0, svec2_one(), SDL_FLIP_NONE, Rect2iZero());
		}
	CA_FOREACH_END()
}

#define ACTOR_HEIGHT 25
static void DrawChatters(
	DrawBuffer *b, const struct vec2i offset, const Tile *t,
	const struct vec2i pos, const bool useFog)
{
	UNUSED(pos);
	UNUSED(useFog);
	CA_FOREACH(ThingId, tid, t->things)
		const Thing *ti = ThingIdGetThing(tid);
		if (ti->kind != KIND_CHARACTER)
		{
			continue;
		}

		const TActor *a = CArrayGet(&gActors, ti->id);
		// Draw character text
		if (strlen(a->Chatter) > 0)
		{
			const struct vec2i textPos = svec2i(
				(int)a->thing.Pos.x - b->xTop + offset.x -
				FontStrW(a->Chatter) / 2,
				(int)a->thing.Pos.y - b->yTop + offset.y - ACTOR_HEIGHT);
			const color_t mask = GetLOSMask(t, useFog);
			if (!ColorEquals(mask, colorTransparent))
			{
				FontStrMask(a->Chatter, textPos, mask);
			}
		}
	CA_FOREACH_END()
}

static void DrawThing(
	DrawBuffer *b, const Thing *t, const struct vec2i offset)
{
	const struct vec2i picPos = svec2i_add(
		svec2i_subtract(
			svec2i_floor(svec2_add(t->Pos, t->drawShake)),
			svec2i(b->xTop, b->yTop)),
		offset);

	if (!svec2i_is_zero(t->ShadowSize))
	{
		DrawShadow(
			&gGraphicsDevice, picPos, svec2_assign_vec2i(t->ShadowSize),
			colorBlack);
	}

	if (t->CPicFunc)
	{
		t->CPicFunc(b->g, t->id, picPos);
	}
	else if (t->kind == KIND_CHARACTER)
	{
		TActor *a = CArrayGet(&gActors, t->id);
		ActorPics pics = GetCharacterPicsFromActor(a);
		DrawActorPics(&pics, picPos, false, Rect2iZero());
		// Draw weapon indicators
		DrawLaserSight(&pics, a, picPos);
	}
	else
	{
		(*(t->drawFunc))(picPos, &t->drawData);
	}

#ifdef DEBUG_DRAW_HITBOXES
	const int pulsePeriod = ConfigGetInt(&gConfig, "Game.FPS");
	int alphaUnscaled =
		(gMission.time % pulsePeriod) * 255 / (pulsePeriod / 2);
	if (alphaUnscaled > 255)
	{
		alphaUnscaled = 255 * 2 - alphaUnscaled;
	}
	color_t color = colorPurple;
	color.a = (Uint8)alphaUnscaled;
	DrawRectangle(
		&gGraphicsDevice, svec2i_subtract(picPos, svec2i_scale_divide(t->size, 2)),
		t->size, color, DRAW_FLAG_LINE);
#endif
}

static void DrawEditorTiles(DrawBuffer *b, const struct vec2i offset);
static void DrawGuideImage(
	DrawBuffer *b, SDL_Surface *guideImage, Uint8 alpha);
static void DrawObjectNames(DrawBuffer *b, const struct vec2i offset);
static void DrawExtra(DrawBuffer *b, struct vec2i offset, GrafxDrawExtra *extra)
{
	// Draw guide image
	if (extra->guideImage && extra->guideImageAlpha > 0)
	{
		DrawGuideImage(b, extra->guideImage, extra->guideImageAlpha);
	}
	DrawEditorTiles(b, offset);
	DrawObjectNames(b, offset);
}

static void DrawEditorTiles(DrawBuffer *b, const struct vec2i offset)
{
	struct vec2i pos;
	pos.y = b->dy + offset.y;
	for (int y = 0; y < Y_TILES; y++, pos.y += TILE_HEIGHT)
	{
		pos.x = b->dx + offset.x;
		for (int x = 0; x < b->Size.x; x++, pos.x += TILE_WIDTH)
		{
			if (gMission.missionData->Type == MAPTYPE_STATIC)
			{
				struct vec2i start = gMission.missionData->u.Static.Start;
				if (!svec2i_is_zero(start) &&
					svec2i_is_equal(start, svec2i(x + b->xStart, y + b->yStart)))
				{
					// mission start
					BlitMasked(
						&gGraphicsDevice,
						PicManagerGetPic(&gPicManager, "editor/start"),
						pos, colorWhite, 1);
				}
			}
		}
	}
}

static void DrawGuideImage(
	DrawBuffer *b, SDL_Surface *guideImage, Uint8 alpha)
{
	SDL_LockSurface(guideImage);
	// Scale based on ratio between map size and guide image size,
	// so that the guide image stretches to the map size
	double xScale = (double)guideImage->w / (gMap.Size.x * TILE_WIDTH);
	double yScale = (double)guideImage->h / (gMap.Size.y * TILE_HEIGHT);
	for (int j = 0; j < b->g->cachedConfig.Res.y; j++)
	{
		int y = (int)round((j + b->yTop) * yScale);
		for (int i = 0; i < b->g->cachedConfig.Res.x; i++)
		{
			int x = (int)round((i + b->xTop) * xScale);
			if (x >= 0 && x < guideImage->w && y >= 0 && y < guideImage->h)
			{
				int imgIndex = y * guideImage->w + x;
				Uint32 p = ((Uint32 *)guideImage->pixels)[imgIndex];
				color_t c = PIXEL2COLOR(p);
				c.a = alpha;
				Draw_Point(i, j, c);
			}
		}
	}
	SDL_UnlockSurface(guideImage);
}

// Draw names of objects (objectives, spawners etc.)
static void DrawObjectiveName(
	const Thing *ti, DrawBuffer *b, const struct vec2i offset);
static void DrawSpawnerName(
	const TObject *obj, DrawBuffer *b, const struct vec2i offset);
static void DrawObjectNames(DrawBuffer *b, const struct vec2i offset)
{
	const Tile **tile = DrawBufferGetFirstTile(b);
	for (int y = 0; y < Y_TILES; y++)
	{
		for (int x = 0; x < b->Size.x; x++, tile++)
		{
			if (*tile == NULL) continue;
			CA_FOREACH(ThingId, tid, (*tile)->things)
				const Thing *ti = ThingIdGetThing(tid);
				if (ti->flags & THING_OBJECTIVE)
				{
					DrawObjectiveName(ti, b, offset);
				}
				else if (ti->kind == KIND_OBJECT)
				{
					const TObject *obj = CArrayGet(&gObjs, ti->id);
					if (obj->Class->Type == MAP_OBJECT_TYPE_PICKUP_SPAWNER)
					{
						DrawSpawnerName(obj, b, offset);
					}
				}
			CA_FOREACH_END()
		}
		tile += X_TILES - b->Size.x;
	}
}
static void DrawObjectiveName(
	const Thing *ti, DrawBuffer *b, const struct vec2i offset)
{
	const int objective = ObjectiveFromThing(ti->flags);
	const Objective *o =
		CArrayGet(&gMission.missionData->Objectives, objective);
	const char *typeName = ObjectiveTypeStr(o->Type);
	const struct vec2i textPos = svec2i(
		(int)ti->Pos.x - b->xTop + offset.x - FontStrW(typeName) / 2,
		(int)ti->Pos.y - b->yTop + offset.y);
	FontStr(typeName, textPos);
}
static void DrawSpawnerName(
	const TObject *obj, DrawBuffer *b, const struct vec2i offset)
{
	const char *name = obj->Class->u.PickupClass->Name;
	const struct vec2i textPos = svec2i(
		(int)obj->thing.Pos.x - b->xTop + offset.x - FontStrW(name) / 2,
		(int)obj->thing.Pos.y - b->yTop + offset.y);
	FontStr(name, textPos);
}
