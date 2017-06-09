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
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "actors.h"
#include "algorithms.h"
#include "config.h"
#include "draw/draw_actor.h"
#include "draw_highlight.h"
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
	if (tile->flags & MAPTILE_OUT_OF_SIGHT)
	{
		return useFog ? TILE_LOS_FOG : TILE_LOS_NONE;
	}
	return TILE_LOS_NORMAL;
}
void DrawWallColumn(int y, Vec2i pos, Tile *tile)
{
	const bool useFog = ConfigGetBool(&gConfig, "Game.Fog");
	while (y >= 0 && (tile->flags & MAPTILE_IS_WALL))
	{
		switch (GetTileLOS(tile, useFog))
		{
		case TILE_LOS_NORMAL:
			Blit(&gGraphicsDevice, &tile->pic->pic, pos);
			break;
		case TILE_LOS_FOG:
			BlitMasked(&gGraphicsDevice, &tile->pic->pic, pos, colorFog, false);
			break;
		case TILE_LOS_NONE:
		default:
			// don't draw anything
			break;
		}
		pos.y -= TILE_HEIGHT;
		tile -= X_TILES;
		y--;
	}
}


static void DrawFloor(DrawBuffer *b, Vec2i offset);
static void DrawDebris(DrawBuffer *b, Vec2i offset);
static void DrawWallsAndThings(DrawBuffer *b, Vec2i offset);
static void DrawExtra(DrawBuffer *b, Vec2i offset, GrafxDrawExtra *extra);

void DrawBufferDraw(DrawBuffer *b, Vec2i offset, GrafxDrawExtra *extra)
{
	// First draw the floor tiles (which do not obstruct anything)
	DrawFloor(b, offset);
	// Then draw debris (wrecks)
	DrawDebris(b, offset);
	// Now draw walls and (non-wreck) things in proper order
	DrawWallsAndThings(b, offset);
	// Draw objective highlights, for visible and always-visible objectives
	DrawObjectiveHighlights(b, offset);
	// Draw actor chatter
	DrawChatters(b, offset);
	// Draw editor-only things
	if (extra)
	{
		DrawExtra(b, offset, extra);
	}
}

static void DrawFloor(DrawBuffer *b, Vec2i offset)
{
	int x, y;
	Vec2i pos;
	const Tile *tile = &b->tiles[0][0];
	const bool useFog = ConfigGetBool(&gConfig, "Game.Fog");
	for (y = 0, pos.y = b->dy + offset.y;
		 y < Y_TILES;
		 y++, pos.y += TILE_HEIGHT)
	{
		for (x = 0, pos.x = b->dx + offset.x;
			x < b->Size.x;
			x++, tile++, pos.x += TILE_WIDTH)
		{
			if (tile->pic != NULL && tile->pic->pic.Data != NULL &&
				!(tile->flags & MAPTILE_IS_WALL))
			{
				switch (GetTileLOS(tile, useFog))
				{
				case TILE_LOS_NORMAL:
					Blit(&gGraphicsDevice, &tile->pic->pic, pos);
					break;
				case TILE_LOS_FOG:
					BlitMasked(
						&gGraphicsDevice,
						&tile->pic->pic,
						pos,
						colorFog,
						false);
					break;
				case TILE_LOS_NONE:
				default:
					// don't draw
					break;
				}
			}
		}
		tile += X_TILES - b->Size.x;
	}
}

static void DrawThing(DrawBuffer *b, const TTileItem *t, const Vec2i offset);

static void DrawDebris(DrawBuffer *b, Vec2i offset)
{
	Tile *tile = &b->tiles[0][0];
	for (int y = 0; y < Y_TILES; y++)
	{
		CArrayClear(&b->displaylist);
		for (int x = 0; x < b->Size.x; x++, tile++)
		{
			if (tile->flags & MAPTILE_OUT_OF_SIGHT)
			{
				continue;
			}
			CA_FOREACH(ThingId, tid, tile->things)
				const TTileItem *ti = ThingIdGetTileItem(tid);
				if (TileItemDrawLast(ti))
				{
					CArrayPushBack(&b->displaylist, &ti);
				}
			CA_FOREACH_END()
		}
		DrawBufferSortDisplayList(b);
		CA_FOREACH(const TTileItem *, tp, b->displaylist)
			DrawThing(b, *tp, offset);
		CA_FOREACH_END()
		tile += X_TILES - b->Size.x;
	}
}

#define WALL_OFFSET_Y (-12)
static void DrawWallsAndThings(DrawBuffer *b, Vec2i offset)
{
	Vec2i pos;
	Tile *tile = &b->tiles[0][0];
	pos.y = b->dy + WALL_OFFSET_Y + offset.y;
	const bool useFog = ConfigGetBool(&gConfig, "Game.Fog");
	for (int y = 0; y < Y_TILES; y++, pos.y += TILE_HEIGHT)
	{
		CArrayClear(&b->displaylist);
		pos.x = b->dx + offset.x;
		for (int x = 0; x < b->Size.x; x++, tile++, pos.x += TILE_WIDTH)
		{
			if (tile->flags & MAPTILE_IS_WALL)
			{
				if (!(tile->flags & MAPTILE_DELAY_DRAW))
				{
					DrawWallColumn(y, pos, tile);
				}
			}
			else if (tile->flags & MAPTILE_OFFSET_PIC)
			{
				// Drawing doors
				// Doors may be offset; vertical doors are drawn centered
				// horizontal doors are bottom aligned
				Vec2i doorPos = pos;
				doorPos.x += (TILE_WIDTH - tile->picAlt->pic.size.x) / 2;
				if (tile->picAlt->pic.size.y > 16)
				{
					doorPos.y +=
						TILE_HEIGHT - (tile->picAlt->pic.size.y % TILE_HEIGHT);
				}
				switch (GetTileLOS(tile, useFog))
				{
				case TILE_LOS_NORMAL:
					Blit(&gGraphicsDevice, &tile->picAlt->pic, doorPos);
					break;
				case TILE_LOS_FOG:
					BlitMasked(
						&gGraphicsDevice,
						&tile->picAlt->pic,
						doorPos,
						colorFog,
						false);
					break;
				case TILE_LOS_NONE:
				default:
					// don't draw anything
					break;
				}
			}

			// Draw the items that are in LOS
			if (tile->flags & MAPTILE_OUT_OF_SIGHT)
			{
				continue;
			}
			CA_FOREACH(ThingId, tid, tile->things)
				const TTileItem *ti = ThingIdGetTileItem(tid);
				// Drawn later
				if (TileItemDrawLast(ti))
				{
					continue;
				}
				CArrayPushBack(&b->displaylist, &ti);
			CA_FOREACH_END()
		}
		DrawBufferSortDisplayList(b);
		CA_FOREACH(const TTileItem *, tp, b->displaylist)
			DrawThing(b, *tp, offset);
		CA_FOREACH_END()
		tile += X_TILES - b->Size.x;
	}
}
static void DrawThing(DrawBuffer *b, const TTileItem *t, const Vec2i offset)
{
	const Vec2i picPos = Vec2iNew(
		t->x - b->xTop + offset.x, t->y - b->yTop + offset.y);

	if (!Vec2iIsZero(t->ShadowSize))
	{
		DrawShadow(&gGraphicsDevice, picPos, t->ShadowSize);
	}

	if (t->CPicFunc)
	{
		CPicDrawContext c = t->CPicFunc(t->id);
		CPicDraw(b->g, &t->CPic, picPos, &c);
	}
	else if (t->getPicFunc)
	{
		Vec2i picOffset;
		const Pic *pic = t->getPicFunc(t->id, &picOffset);
		Blit(&gGraphicsDevice, pic, Vec2iAdd(picPos, picOffset));
	}
	else if (t->kind == KIND_CHARACTER)
	{
		TActor *a = CArrayGet(&gActors, t->id);
		ActorPics pics = GetCharacterPicsFromActor(a);
		DrawActorPics(&pics, picPos);
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
		&gGraphicsDevice, Vec2iMinus(picPos, Vec2iScaleDiv(t->size, 2)),
		t->size, color, DRAW_FLAG_LINE);
#endif
}

static void DrawEditorTiles(DrawBuffer *b, const Vec2i offset);
static void DrawGuideImage(
	DrawBuffer *b, SDL_Surface *guideImage, Uint8 alpha);
static void DrawObjectNames(DrawBuffer *b, const Vec2i offset);
static void DrawExtra(DrawBuffer *b, Vec2i offset, GrafxDrawExtra *extra)
{
	// Draw guide image
	if (extra->guideImage && extra->guideImageAlpha > 0)
	{
		DrawGuideImage(b, extra->guideImage, extra->guideImageAlpha);
	}
	DrawEditorTiles(b, offset);
	DrawObjectNames(b, offset);
}

static void DrawEditorTiles(DrawBuffer *b, const Vec2i offset)
{
	Vec2i pos;
	Tile *tile = &b->tiles[0][0];
	pos.y = b->dy + offset.y;
	for (int y = 0; y < Y_TILES; y++, pos.y += TILE_HEIGHT)
	{
		pos.x = b->dx + offset.x;
		for (int x = 0; x < b->Size.x; x++, tile++, pos.x += TILE_WIDTH)
		{
			if (gMission.missionData->Type == MAPTYPE_STATIC)
			{
				Vec2i start = gMission.missionData->u.Static.Start;
				if (!Vec2iIsZero(start) &&
					Vec2iEqual(start, Vec2iNew(x + b->xStart, y + b->yStart)))
				{
					// mission start
					BlitMasked(
						&gGraphicsDevice,
						PicManagerGetPic(&gPicManager, "editor/start"),
						pos, colorWhite, 1);
				}
			}
		}
		tile += X_TILES - b->Size.x;
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
	const TTileItem *ti, DrawBuffer *b, const Vec2i offset);
static void DrawSpawnerName(
	const TObject *obj, DrawBuffer *b, const Vec2i offset);
static void DrawObjectNames(DrawBuffer *b, const Vec2i offset)
{
	const Tile *tile = &b->tiles[0][0];
	for (int y = 0; y < Y_TILES; y++)
	{
		for (int x = 0; x < b->Size.x; x++, tile++)
		{
			CA_FOREACH(ThingId, tid, tile->things)
				const TTileItem *ti = ThingIdGetTileItem(tid);
				if (ti->flags & TILEITEM_OBJECTIVE)
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
	const TTileItem *ti, DrawBuffer *b, const Vec2i offset)
{
	const int objective = ObjectiveFromTileItem(ti->flags);
	const Objective *o =
		CArrayGet(&gMission.missionData->Objectives, objective);
	const char *typeName = ObjectiveTypeStr(o->Type);
	const Vec2i textPos = Vec2iNew(
		ti->x - b->xTop + offset.x - FontStrW(typeName) / 2,
		ti->y - b->yTop + offset.y);
	FontStr(typeName, textPos);
}
static void DrawSpawnerName(
	const TObject *obj, DrawBuffer *b, const Vec2i offset)
{
	const char *name = obj->Class->u.PickupClass->Name;
	const Vec2i textPos = Vec2iNew(
		obj->tileItem.x - b->xTop + offset.x - FontStrW(name) / 2,
		obj->tileItem.y - b->yTop + offset.y);
	FontStr(name, textPos);
}
