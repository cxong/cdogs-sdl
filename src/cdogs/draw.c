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
#include "drawtools.h"
#include "font.h"
#include "game_events.h"
#include "net_util.h"
#include "objs.h"
#include "pics.h"
#include "draw.h"
#include "blit.h"
#include "pic_manager.h"

#define NECK_OFFSET 13
#define FOOT_OFFSET 3
#define WRIST_OFFSET 6


typedef enum
{
	BODY_PART_HEAD,
	BODY_PART_BODY,
	BODY_PART_GUN
} BodyPart;
static Vec2i GetActorDrawOffset(
	const Vec2i pos, const Pic *pic, const BodyPart part, const direction_e d)
{
	Vec2i outPos = Vec2iMinus(pos, Vec2iScaleDiv(pic->size, 2));
	switch (part)
	{
	case BODY_PART_HEAD:
		outPos.y -= NECK_OFFSET;
		break;
	case BODY_PART_BODY:
		outPos.y -= FOOT_OFFSET;
		break;
	case BODY_PART_GUN:
		outPos = Vec2iAdd(outPos, cGunHandOffset[d]);
		outPos.y -= WRIST_OFFSET;
		break;
	default:
		CASSERT(false, "unknown body part");
		break;
	}
	return outPos;
}


// For actor drawing
typedef struct
{
	const Pic *Head;
	const Pic *Body;
	const Pic *Gun;
	BodyPart DrawOrder[3];
	const CharColors *Colors;
	bool IsDead;
	bool IsDying;
	bool IsTransparent;
	HSV *Tint;
	color_t *Mask;
} ActorPics;


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
				if (TileItemIsDebris(ti))
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
				// Don't draw debris, they are drawn later
				if (TileItemIsDebris(ti))
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
static void GetCharacterPicsFromActor(ActorPics *pics, TActor *a);
static void DrawActorPics(
	const ActorPics *pics, const Vec2i picPos, const direction_e d);
static void DrawLaserSight(
	const ActorPics *pics, const TActor *a, const Vec2i picPos);
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
		ActorPics pics;
		GetCharacterPicsFromActor(&pics, a);
		DrawActorPics(&pics, picPos, RadiansToDirection(a->DrawRadians));
		// Draw weapon indicators
		DrawLaserSight(&pics, a, picPos);
	}
	else
	{
		(*(t->drawFunc))(picPos, &t->drawData);
	}
}
static Character *ActorGetCharacterMutable(TActor *a);
static void GetCharacterPics(
	ActorPics *pics, Character *c, const direction_e dir, const int frame,
	const NamedSprites *gunPics, const gunstate_e gunState,
	const bool isTransparent, HSV *tint, color_t *mask,
	const int deadPic);
static void GetCharacterPicsFromActor(ActorPics *pics, TActor *a)
{
	const Weapon *gun = ActorGetGun(a);
	HSV *tint = NULL;
	color_t *mask = NULL;
	if (a->flamed)
	{
		tint = &tintRed;
		mask = &colorRed;
	}
	else if (a->poisoned)
	{
		tint = &tintPoison;
		mask = &colorPoison;
	}
	else if (a->petrified)
	{
		tint = &tintGray;
		mask = &colorGray;
	}
	else if (a->confused)
	{
		tint = &tintPurple;
		mask = &colorPurple;
	}
	GetCharacterPics(
		pics, ActorGetCharacterMutable(a),
		RadiansToDirection(a->DrawRadians), AnimationGetFrame(&a->anim),
		gun->Gun->Pic, gun->state,
		!!(a->flags & FLAGS_SEETHROUGH),
		tint, mask,
		a->dead);
}
static const Pic *GetHeadPic(
	const CharacterClass *c, const direction_e dir, const int state);
static const Pic *GetBodyPic(
	PicManager *pm, const direction_e dir, const int state, const bool isArmed);
static const Pic *GetGunPic(
	const NamedSprites *gunPics, const direction_e dir, const int gunState);
static const Pic *GetDeathPic(PicManager *pm, const int frame);
static void GetCharacterPics(
	ActorPics *pics, Character *c, const direction_e dir, const int frame,
	const NamedSprites *gunPics, const gunstate_e gunState,
	const bool isTransparent, HSV *tint, color_t *mask,
	const int deadPic)
{
	memset(pics, 0, sizeof *pics);
	pics->DrawOrder[0] = pics->DrawOrder[1] = pics->DrawOrder[2] = -1;
	pics->Colors = &c->Colors;

	// If the actor is dead, simply draw a dying animation
	pics->IsDead = deadPic > 0;
	if (pics->IsDead)
	{
		if (deadPic < DEATH_MAX)
		{
			pics->IsDying = true;
			pics->Body = GetDeathPic(&gPicManager, deadPic - 1);
			pics->DrawOrder[0] = BODY_PART_BODY;
		}
		return;
	}

	pics->IsTransparent = isTransparent;
	if (pics->IsTransparent)
	{
		pics->Tint = &tintDarker;
	}
	else if (tint != NULL)
	{
		pics->Tint = tint;
		pics->Mask = mask;
	}

	// Head
	direction_e headDir = dir;
	int headFrame = frame;
	// If firing, draw the firing head pic
	if (gunState == GUNSTATE_FIRING || gunState == GUNSTATE_RECOIL)
	{
		headFrame = STATE_COUNT + gunState - GUNSTATE_FIRING;
	}
	// If idle, turn head left/right on occasion
	if (headFrame == STATE_IDLELEFT) headDir = (dir + 7) % 8;
	else if (headFrame == STATE_IDLERIGHT) headDir = (dir + 1) % 8;

	pics->Head = GetHeadPic(c->Class, headDir, headFrame);

	// Body
	const bool isArmed = gunPics != NULL;
	pics->Body = GetBodyPic(&gPicManager, dir, frame, isArmed);

	// Gun
	pics->Gun = NULL;
	if (isArmed)
	{
		pics->Gun = GetGunPic(gunPics, dir, gunState);
	}

	// Determine draw order based on the direction the player is facing
	// Order is: 0: head, 1: body, 2: gun
	switch (dir)
	{
	case DIRECTION_UP:
	case DIRECTION_UPRIGHT:
		pics->DrawOrder[0] = BODY_PART_GUN;
		pics->DrawOrder[1] = BODY_PART_HEAD;
		pics->DrawOrder[2] = BODY_PART_BODY;
		break;

	case DIRECTION_RIGHT:
	case DIRECTION_DOWNRIGHT:
	case DIRECTION_DOWN:
	case DIRECTION_DOWNLEFT:
		pics->DrawOrder[0] = BODY_PART_BODY;
		pics->DrawOrder[1] = BODY_PART_HEAD;
		pics->DrawOrder[2] = BODY_PART_GUN;
		break;

	case DIRECTION_LEFT:
	case DIRECTION_UPLEFT:
		pics->DrawOrder[0] = BODY_PART_GUN;
		pics->DrawOrder[1] = BODY_PART_BODY;
		pics->DrawOrder[2] = BODY_PART_HEAD;
		break;
	default:
		CASSERT(false, "invalid direction");
		return;
	}
}
static Character *ActorGetCharacterMutable(TActor *a)
{
	if (a->PlayerUID >= 0)
	{
		return &PlayerDataGetByUID(a->PlayerUID)->Char;
	}
	return CArrayGet(&gCampaign.Setting.characters.OtherChars, a->charId);
}

static void DrawBody(GraphicsDevice *g, const ActorPics *pics, const Vec2i pos);
static void DrawActorPics(
	const ActorPics *pics, const Vec2i picPos, const direction_e d)
{
	if (pics->IsDead)
	{
		if (pics->IsDying)
		{
			DrawBody(&gGraphicsDevice, pics, picPos);
		}
	}
	else
	{
		// Draw shadow
		if (!pics->IsTransparent)
		{
			DrawShadow(&gGraphicsDevice, picPos, Vec2iNew(8, 6));
		}
		for (int i = 0; i < 3; i++)
		{
			const Pic *picp = NULL;
			switch (pics->DrawOrder[i])
			{
			case BODY_PART_HEAD:
				picp = pics->Head;
				break;
			case BODY_PART_BODY:
				picp = pics->Body;
				break;
			case BODY_PART_GUN:
				picp = pics->Gun;
				break;
			}
			if (picp == NULL)
			{
				continue;
			}
			const Vec2i drawPos = GetActorDrawOffset(
				picPos, picp, pics->DrawOrder[i], d);
			if (pics->IsTransparent)
			{
				BlitBackground(
					&gGraphicsDevice, picp, drawPos, pics->Tint, true);
			}
			else if (pics->Mask != NULL)
			{
				BlitMasked(&gGraphicsDevice, picp, drawPos, *pics->Mask, true);
			}
			else
			{
				BlitCharMultichannel(
					&gGraphicsDevice, picp, drawPos, pics->Colors);
			}
		}
	}
}
static void DrawLaserSightSingle(
	const Vec2i from, const double radians, const int range,
	const color_t color);
static void DrawLaserSight(
	const ActorPics *pics, const TActor *a, const Vec2i picPos)
{
	// Don't draw if dead or transparent
	if (pics->IsDead || pics->IsTransparent) return;
	// Check config
	const LaserSight ls = ConfigGetEnum(&gConfig, "Game.LaserSight");
	if (ls != LASER_SIGHT_ALL &&
		!(ls == LASER_SIGHT_PLAYERS && a->PlayerUID >= 0))
	{
		return;
	}
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
			CA_FOREACH(ThingId, tid, tile->things)
				TTileItem *ti = ThingIdGetTileItem(tid);
				DrawObjectiveHighlight(ti, tile, b, offset);
			CA_FOREACH_END()
		}
		tile += X_TILES - b->Size.x;
	}
}
static void DrawActorHighlight(
	const ActorPics *pics, const Vec2i pos, const color_t color,
	const direction_e d);
static void DrawObjectiveHighlight(
	TTileItem *ti, Tile *tile, DrawBuffer *b, Vec2i offset)
{
	if (!(ti->flags & TILEITEM_OBJECTIVE))
	{
		return;
	}
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
	const Vec2i pos = Vec2iNew(
		ti->x - b->xTop + offset.x, ti->y - b->yTop + offset.y);
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
	else if (ti->kind == KIND_CHARACTER)
	{
		TActor *a = CArrayGet(&gActors, ti->id);
		ActorPics pics;
		GetCharacterPicsFromActor(&pics, a);
		DrawActorHighlight(&pics, pos, color, a->direction);
	}
}
static void DrawActorHighlight(
	const ActorPics *pics, const Vec2i pos, const color_t color,
	const direction_e d)
{
	// Do not highlight dead, dying or transparent characters
	if (pics->IsDead || pics->IsTransparent)
	{
		return;
	}
	const Vec2i headPos = GetActorDrawOffset(
		pos, pics->Head, BODY_PART_HEAD, d);
	BlitPicHighlight(&gGraphicsDevice, pics->Head, headPos, color);
	if (pics->Body != NULL)
	{
		const Vec2i bodyPos = GetActorDrawOffset(
			pos, pics->Body, BODY_PART_BODY, d);
		BlitPicHighlight(&gGraphicsDevice, pics->Body, bodyPos, color);
	}
	if (pics->Gun != NULL)
	{
		const Vec2i gunPos = GetActorDrawOffset(
			pos, pics->Gun, BODY_PART_GUN, d);
		BlitPicHighlight(&gGraphicsDevice, pics->Gun, gunPos, color);
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
			CA_FOREACH(ThingId, tid, tile->things)
				const TTileItem *ti = ThingIdGetTileItem(tid);
				if (ti->kind != KIND_CHARACTER)
				{
					continue;
				}
				DrawChatter(ti, b, offset);
			CA_FOREACH_END()
		}
		tile += X_TILES - b->Size.x;
	}
}
#define ACTOR_HEIGHT 25
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

static const Pic *GetHeadPic(
	const CharacterClass *c, const direction_e dir, const int state)
{
	const int idx = (int)dir + (state >= STATE_COUNT ? DIRECTION_COUNT : 0);
	return CPicGetPic(&c->HeadPics, idx);
}
static const Pic *GetBodyPic(
	PicManager *pm, const direction_e dir, const int state, const bool isArmed)
{
	const bool isIdle = state <= STATE_IDLERIGHT;
	const int row = (isArmed ? 5 : 0) + (isIdle ? 0 : state - STATE_IDLERIGHT);
	const int idx = row * DIRECTION_COUNT + dir;
	return CArrayGet(
		&PicManagerGetSprites(pm, "chars/body_arms_legs")->pics, idx);
}
static const Pic *GetGunPic(
	const NamedSprites *gunPics, const direction_e dir, const int gunState)
{
	const int idx = (gunState == GUNSTATE_READY ? 8 : 0) + dir;
	return CArrayGet(&gunPics->pics, idx);
}
static const Pic *GetDeathPic(PicManager *pm, const int frame)
{
	return CArrayGet(&PicManagerGetSprites(pm, "chars/death")->pics, frame);
}

void DrawCharacterSimple(
	Character *c, const Vec2i pos, const direction_e d,
	const bool hilite, const bool showGun)
{
	ActorPics pics;
	GetCharacterPics(
		&pics, c, d, STATE_IDLE, NULL, GUNSTATE_READY, false, NULL, NULL, 0);
	DrawActorPics(&pics, pos, d);
	if (hilite)
	{
		FontCh('>', Vec2iAdd(pos, Vec2iNew(-8, -16)));
		if (showGun)
		{
			FontStr(c->Gun->name, Vec2iAdd(pos, Vec2iNew(-8, 8)));
		}
	}
}

void DrawHead(
	const Character *c, const direction_e dir,
	const int state, const Vec2i pos)
{
	const Pic *head = GetHeadPic(c->Class, dir, state);
	const Vec2i drawPos = Vec2iMinus(pos, Vec2iNew(
		head->size.x / 2, head->size.y / 2));
	BlitCharMultichannel(&gGraphicsDevice, head, drawPos, &c->Colors);
}
static void DrawBody(GraphicsDevice *g, const ActorPics *pics, const Vec2i pos)
{
	const Pic *body = pics->Body;
	const Vec2i drawPos = Vec2iMinus(pos, Vec2iNew(
		body->size.x / 2, body->size.y / 2 + FOOT_OFFSET));
	const color_t mask = pics->Mask != NULL ? *pics->Mask : colorWhite;
	BlitMasked(g, pics->Body, drawPos, mask, true);
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
