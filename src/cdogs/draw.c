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

    Copyright (c) 2013-2015, Cong Xu
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
#include "drawtools.h"
#include "font.h"
#include "game_events.h"
#include "net_util.h"
#include "objs.h"
#include "pics.h"
#include "draw.h"
#include "blit.h"
#include "pic_manager.h"

//#define DEBUG_DRAW_BOUNDS


// Three types of tile drawing, based on line of sight:
// Unvisited: black
// Out of sight: dark, or if fog disabled, black
// In sight: full color
static color_t GetTileLOSMask(Tile *tile)
{
	if (!tile->isVisited)
	{
		return colorBlack;
	}
	if (tile->flags & MAPTILE_OUT_OF_SIGHT)
	{
		if (ConfigGetBool(&gConfig, "Game.Fog"))
		{
			color_t mask = { 96, 96, 96, 255 };
			return mask;
		}
		else
		{
			return colorBlack;
		}
	}
	return colorWhite;
}

void DrawWallColumn(int y, Vec2i pos, Tile *tile)
{
	while (y >= 0 && (tile->flags & MAPTILE_IS_WALL))
	{
		BlitMasked(
			&gGraphicsDevice,
			&tile->pic->pic,
			pos,
			GetTileLOSMask(tile),
			0);
		pos.y -= TILE_HEIGHT;
		tile -= X_TILES;
		y--;
	}
}


static void DrawFloor(DrawBuffer *b, Vec2i offset);
static void DrawDebris(DrawBuffer *b, Vec2i offset);
static void DrawWallsAndThings(DrawBuffer *b, Vec2i offset);
static void DrawObjectiveHighlights(DrawBuffer *b, Vec2i offset);
static void DrawChatters(DrawBuffer *b, Vec2i offset);
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
	Tile *tile = &b->tiles[0][0];
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
				BlitMasked(
					&gGraphicsDevice,
					&tile->pic->pic,
					pos,
					GetTileLOSMask(tile),
					0);
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
			for (int i = 0; i < (int)tile->things.size; i++)
			{
				const TTileItem *ti =
					ThingIdGetTileItem(CArrayGet(&tile->things, i));
				if (TileItemIsDebris(ti))
				{
					CArrayPushBack(&b->displaylist, &ti);
				}
			}
		}
		DrawBufferSortDisplayList(b);
		for (int i = 0; i < (int)b->displaylist.size; i++)
		{
			const TTileItem **tp = CArrayGet(&b->displaylist, i);
			const TTileItem *t = *tp;
			DrawThing(b, t, offset);
		}
		tile += X_TILES - b->Size.x;
	}
}

static void DrawWallsAndThings(DrawBuffer *b, Vec2i offset)
{
	Vec2i pos;
	Tile *tile = &b->tiles[0][0];
	pos.y = b->dy + cWallOffset.dy + offset.y;
	for (int y = 0; y < Y_TILES; y++, pos.y += TILE_HEIGHT)
	{
		CArrayClear(&b->displaylist);
		pos.x = b->dx + cWallOffset.dx + offset.x;
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
				BlitMasked(
					&gGraphicsDevice,
					&tile->picAlt->pic,
					doorPos,
					GetTileLOSMask(tile),
					0);
			}

			// Draw the items that are in LOS
			if (tile->flags & MAPTILE_OUT_OF_SIGHT)
			{
				continue;
			}
			for (int i = 0; i < (int)tile->things.size; i++)
			{
				const TTileItem *ti =
					ThingIdGetTileItem(CArrayGet(&tile->things, i));
				// Don't draw debris, they are drawn later
				if (TileItemIsDebris(ti))
				{
					continue;
				}
				CArrayPushBack(&b->displaylist, &ti);
			}
		}
		DrawBufferSortDisplayList(b);
		for (int i = 0; i < (int)b->displaylist.size; i++)
		{
			const TTileItem **tp = CArrayGet(&b->displaylist, i);
			DrawThing(b, *tp, offset);
		}
		tile += X_TILES - b->Size.x;
	}
}
static void DrawActorPics(const TTileItem *t, const Vec2i picPos);
static void DrawThing(DrawBuffer *b, const TTileItem *t, const Vec2i offset)
{
	const Vec2i picPos = Vec2iNew(
		t->x - b->xTop + offset.x, t->y - b->yTop + offset.y);
#ifdef DEBUG_DRAW_BOUNDS
	Draw_Box(
		picPos.x - t->size.x / 2, picPos.y - t->size.y / 2,
		picPos.x + t->size.x / 2, picPos.y + t->size.y / 2,
		colorGray);
#endif

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
	else if (t->getActorPicsFunc)
	{
		DrawActorPics(t, picPos);
	}
	else
	{
		(*(t->drawFunc))(picPos, &t->drawData);
	}
}
#define ACTOR_HEIGHT 25
static void DrawLaserSight(const TActor *a, const Vec2i picPos);
static void DrawActorPics(const TTileItem *t, const Vec2i picPos)
{
	const ActorPics pics = t->getActorPicsFunc(t->id);
	if (pics.IsDead)
	{
		if (pics.IsDying)
		{
			int pic = pics.OldPics[0];
			if (pic == 0)
			{
				return;
			}
			if (pics.IsTransparent)
			{
				DrawBTPic(
					&gGraphicsDevice,
					PicManagerGetFromOld(&gPicManager, pic),
					Vec2iAdd(picPos, pics.Pics[0].offset),
					pics.Tint);
			}
			else
			{
				DrawTTPic(
					picPos.x + pics.Pics[0].offset.x,
					picPos.y + pics.Pics[0].offset.y,
					PicManagerGetOldPic(&gPicManager, pic),
					pics.Table);
			}
		}
	}
	else if (pics.IsTransparent)
	{
		for (int i = 0; i < 3; i++)
		{
			Pic *oldPic = PicManagerGetFromOld(
				&gPicManager, pics.OldPics[i]);
			if (oldPic == NULL)
			{
				continue;
			}
			DrawBTPic(
				&gGraphicsDevice,
				oldPic,
				Vec2iAdd(picPos, pics.Pics[i].offset),
				pics.Tint);
		}
	}
	else
	{
		DrawShadow(&gGraphicsDevice, picPos, Vec2iNew(8, 6));
		for (int i = 0; i < 3; i++)
		{
			PicPaletted *oldPic = PicManagerGetOldPic(
				&gPicManager, pics.OldPics[i]);
			if (oldPic == NULL)
			{
				continue;
			}
			BlitOld(
				picPos.x + pics.Pics[i].offset.x,
				picPos.y + pics.Pics[i].offset.y,
				oldPic,
				pics.Table, BLIT_TRANSPARENT);
		}

		const TActor *a = CArrayGet(&gActors, t->id);

		// Draw weapon indicators
		if (ConfigGetEnum(&gConfig, "Game.LaserSight") == LASER_SIGHT_ALL ||
			(ConfigGetEnum(&gConfig, "Game.LaserSight") == LASER_SIGHT_PLAYERS && a->PlayerUID >= 0))
		{
			DrawLaserSight(a, picPos);
		}
	}
}
static void DrawLaserSightSingle(
	const Vec2i from, const double radians, const int range,
	const color_t color);
static void DrawLaserSight(const TActor *a, const Vec2i picPos)
{
	// Draw weapon indicators
	const GunDescription *g = ActorGetGun(a)->Gun;
	Vec2i muzzlePos = Vec2iAdd(
		picPos, Vec2iFull2Real(GunGetMuzzleOffset(g, a->direction)));
	muzzlePos.y -= g->MuzzleHeight / Z_FACTOR;
	const double radians = dir2radians[a->direction] + g->AngleOffset;
	const int range = GunGetRange(g);
	color_t color = colorCyan;
	color.a = 64;
	const double spreadHalf =
		(g->Spread.Count - 1) * g->Spread.Width / 2 + g->Recoil / 2;
	if (spreadHalf > 0)
	{
		DrawLaserSightSingle(muzzlePos, radians - spreadHalf, range, color);
		DrawLaserSightSingle(muzzlePos, radians + spreadHalf, range, color);
	}
	else
	{
		DrawLaserSightSingle(muzzlePos, radians, range, color);
	}
}
static void DrawLaserSightSingle(
	const Vec2i from, const double radians, const int range,
	const color_t color)
{
	double x, y;
	GetVectorsForRadians(radians, &x, &y);
	const Vec2i to = Vec2iAdd(
		from, Vec2iNew((int)round(x * range), (int)round(y * range)));
	DrawLine(from, to, color);
}

static void DrawObjectiveHighlight(
	TTileItem *ti, Tile *tile, DrawBuffer *b, Vec2i offset);
static void DrawObjectiveHighlights(DrawBuffer *b, Vec2i offset)
{
	Tile *tile = &b->tiles[0][0];
	for (int y = 0; y < Y_TILES; y++)
	{
		for (int x = 0; x < b->Size.x; x++, tile++)
		{
			// Draw the items that are in LOS
			for (int i = 0; i < (int)tile->things.size; i++)
			{
				TTileItem *ti =
					ThingIdGetTileItem(CArrayGet(&tile->things, i));
				DrawObjectiveHighlight(ti, tile, b, offset);
			}
		}
		tile += X_TILES - b->Size.x;
	}
}
static void DrawObjectiveHighlight(
	TTileItem *ti, Tile *tile, DrawBuffer *b, Vec2i offset)
{
	if (!(ti->flags & TILEITEM_OBJECTIVE))
	{
		return;
	}
	int objective = ObjectiveFromTileItem(ti->flags);
	MissionObjective *mo =
		CArrayGet(&gMission.missionData->Objectives, objective);
	if (mo->Flags & OBJECTIVE_HIDDEN)
	{
		return;
	}
	if (!(mo->Flags & OBJECTIVE_POSKNOWN) &&
		(tile->flags & MAPTILE_OUT_OF_SIGHT))
	{
		return;
	}
	Vec2i pos = Vec2iNew(
		ti->x - b->xTop + offset.x, ti->y - b->yTop + offset.y);
	const ObjectiveDef *o = CArrayGet(&gMission.Objectives, objective);
	color_t color = o->color;
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
	else if (ti->getActorPicsFunc != NULL)
	{
		ActorPics pics = ti->getActorPicsFunc(ti->id);
		// Do not highlight dead, dying or transparent characters
		if (!pics.IsDead && !pics.IsTransparent)
		{
			for (int i = 0; i < 3; i++)
			{
				if (PicIsNotNone(&pics.Pics[i]))
				{
					BlitPicHighlight(
						&gGraphicsDevice,
						&pics.Pics[i], pos, color);
				}
			}
		}
	}
}

static void DrawChatter(
	const TTileItem *ti, DrawBuffer *b, const Vec2i offset);
static void DrawChatters(DrawBuffer *b, Vec2i offset)
{
	const Tile *tile = &b->tiles[0][0];
	for (int y = 0; y < Y_TILES; y++)
	{
		for (int x = 0; x < b->Size.x; x++, tile++)
		{
			for (int i = 0; i < (int)tile->things.size; i++)
			{
				const TTileItem *ti =
					ThingIdGetTileItem(CArrayGet(&tile->things, i));
				if (ti->getActorPicsFunc == NULL)
				{
					continue;
				}
				DrawChatter(ti, b, offset);
			}
		}
		tile += X_TILES - b->Size.x;
	}
}
static void DrawChatter(
	const TTileItem *ti, DrawBuffer *b, const Vec2i offset)
{
	const TActor *a = CArrayGet(&gActors, ti->id);
	// Draw character text
	if (strlen(a->Chatter) > 0)
	{
		const Vec2i textPos = Vec2iNew(
			a->tileItem.x - b->xTop + offset.x -
			FontStrW(a->Chatter) / 2,
			a->tileItem.y - b->yTop + offset.y - ACTOR_HEIGHT);
		FontStr(a->Chatter, textPos);
	}
}

TOffsetPic GetHeadPic(
	const int bodyType, const direction_e dir, const int face, const int state)
{
	TOffsetPic head;
	head.dx = cNeckOffset[bodyType][dir].dx + cHeadOffset[face][dir].dx;
	head.dy = cNeckOffset[bodyType][dir].dy + cHeadOffset[face][dir].dy;
	head.picIndex = cHeadPic[face][dir][state];
	return head;
}

void DrawCharacterSimple(
	const Character *c, const Vec2i pos,
	const direction_e dir, const int state,
	const int gunPic, const gunstate_e gunState,
	const TranslationTable *table)
{
	TOffsetPic body, head, gun;
	TOffsetPic pic1, pic2, pic3;
	direction_e headDir = dir;
	int headState = state;
	if (gunState == GUNSTATE_FIRING || gunState == GUNSTATE_RECOIL)
	{
		headState = STATE_COUNT + gunState - GUNSTATE_FIRING;
	}
	if (state == STATE_IDLELEFT)
	{
		headDir = (direction_e)((dir + 7) % 8);
	}
	else if (state == STATE_IDLERIGHT)
	{
		headDir = (direction_e)((dir + 1) % 8);
	}

	int bodyType = gunPic < 0 ? BODY_UNARMED : BODY_ARMED;

	body.dx = cBodyOffset[bodyType][dir].dx;
	body.dy = cBodyOffset[bodyType][dir].dy;
	body.picIndex = cBodyPic[bodyType][dir][state];

	head = GetHeadPic(bodyType, headDir, c->looks.Face, headState);

	gun.picIndex = -1;
	if (gunPic >= 0)
	{
		gun.dx =
		    cGunHandOffset[bodyType][dir].dx +
		    cGunPics[gunPic][dir][gunState].dx;
		gun.dy =
		    cGunHandOffset[bodyType][dir].dy +
		    cGunPics[gunPic][dir][gunState].dy;
		gun.picIndex = cGunPics[gunPic][dir][gunState].picIndex;
	}

	switch (dir)
	{
	case DIRECTION_UP:
	case DIRECTION_UPRIGHT:
		pic1 = gun;
		pic2 = head;
		pic3 = body;
		break;

	case DIRECTION_RIGHT:
	case DIRECTION_DOWNRIGHT:
	case DIRECTION_DOWN:
	case DIRECTION_DOWNLEFT:
		pic1 = body;
		pic2 = head;
		pic3 = gun;
		break;

	case DIRECTION_LEFT:
	case DIRECTION_UPLEFT:
		pic1 = gun;
		pic2 = body;
		pic3 = head;
		break;
	default:
		assert(0 && "invalid direction");
		return;
	}

	if (pic1.picIndex >= 0)
	{
		BlitOld(
			pos.x + pic1.dx, pos.y + pic1.dy,
			PicManagerGetOldPic(&gPicManager, pic1.picIndex),
			table, BLIT_TRANSPARENT);
	}
	if (pic2.picIndex >= 0)
	{
		BlitOld(
			pos.x + pic2.dx, pos.y + pic2.dy,
			PicManagerGetOldPic(&gPicManager, pic2.picIndex),
			table, BLIT_TRANSPARENT);
	}
	if (pic3.picIndex >= 0)
	{
		BlitOld(
			pos.x + pic3.dx, pos.y + pic3.dy,
			PicManagerGetOldPic(&gPicManager, pic3.picIndex),
			table, BLIT_TRANSPARENT);
	}
}

void DisplayCharacter(
	const Vec2i pos, const Character *c, const bool hilite, const bool showGun)
{
	DrawCharacterSimple(
		c, pos,
		DIRECTION_DOWN, STATE_IDLE, -1, GUNSTATE_READY, &c->table);
	if (hilite)
	{
		FontCh('>', Vec2iAdd(pos, Vec2iNew(-8, -16)));
		if (showGun)
		{
			FontStr(c->Gun->name, Vec2iAdd(pos, Vec2iNew(-8, 8)));
		}
	}
}


static void DrawEditorTiles(DrawBuffer *b, Vec2i offset);
static void DrawGuideImage(
	DrawBuffer *b, SDL_Surface *guideImage, Uint8 alpha);
static void DrawExtra(DrawBuffer *b, Vec2i offset, GrafxDrawExtra *extra)
{
	DrawEditorTiles(b, offset);
	// Draw guide image
	if (extra->guideImage && extra->guideImageAlpha > 0)
	{
		DrawGuideImage(b, extra->guideImage, extra->guideImageAlpha);
	}
}

static void DrawEditorTiles(DrawBuffer *b, Vec2i offset)
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
