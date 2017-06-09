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

    Copyright (c) 2013-2017, Cong Xu
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
#include "draw/draw_actor.h"

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "actors.h"
#include "algorithms.h"
#include "config.h"
#include "draw/drawtools.h"
#include "font.h"
#include "game_events.h"
#include "net_util.h"
#include "objs.h"
#include "pics.h"
#include "draw/draw.h"
#include "blit.h"
#include "pic_manager.h"


static Vec2i GetActorDrawOffset(
	const Pic *pic, const BodyPart part, const CharSprites *cs,
	const ActorAnimation anim, const int frame, const direction_e d)
{
	Vec2i offset = Vec2iScaleDiv(pic->size, -2);
	offset = Vec2iMinus(offset, CharSpritesGetOffset(
		cs->Offsets.Frame[part],
		anim == ACTORANIMATION_WALKING ? "run" : "idle",
		frame));
	offset = Vec2iAdd(offset, cs->Offsets.Dir[part][d]);
	return offset;
}

static Character *ActorGetCharacterMutable(TActor *a);
ActorPics GetCharacterPicsFromActor(TActor *a)
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
	return GetCharacterPics(
		ActorGetCharacterMutable(a),
		RadiansToDirection(a->DrawRadians), a->anim.Type,
		AnimationGetFrame(&a->anim),
		gun->Gun->Pic, gun->state,
		!!(a->flags & FLAGS_SEETHROUGH),
		tint, mask,
		a->dead);
}
static const Pic *GetBodyPic(
	PicManager *pm, const CharSprites *cs, const direction_e dir,
	const ActorAnimation anim, const int frame, const bool isArmed);
static const Pic *GetLegsPic(
	PicManager *pm, const CharSprites *cs, const direction_e dir,
	const ActorAnimation anim, const int frame);
static const Pic *GetGunPic(
	const NamedSprites *gunPics, const direction_e dir, const int gunState);
static const Pic *GetDeathPic(PicManager *pm, const int frame);
ActorPics GetCharacterPics(
	Character *c, const direction_e dir,
	const ActorAnimation anim, const int frame,
	const NamedSprites *gunPics, const gunstate_e gunState,
	const bool isTransparent, HSV *tint, color_t *mask,
	const int deadPic)
{
	ActorPics pics;
	memset(&pics, 0, sizeof pics);
	pics.Colors = &c->Colors;

	// If the actor is dead, simply draw a dying animation
	pics.IsDead = deadPic > 0;
	if (pics.IsDead)
	{
		if (deadPic < DEATH_MAX)
		{
			pics.IsDying = true;
			pics.Body = GetDeathPic(&gPicManager, deadPic - 1);
			pics.OrderedPics[0] = pics.Body;
		}
		return pics;
	}

	pics.IsTransparent = isTransparent;
	if (pics.IsTransparent)
	{
		pics.Tint = &tintDarker;
	}
	else if (tint != NULL)
	{
		pics.Tint = tint;
		pics.Mask = mask;
	}

	// Head
	direction_e headDir = dir;
	// If idle, turn head left/right on occasion
	if (anim == ACTORANIMATION_IDLE)
	{
		if (frame == IDLEHEAD_LEFT) headDir = (dir + 7) % 8;
		else if (frame == IDLEHEAD_RIGHT) headDir = (dir + 1) % 8;
	}
	pics.Head = GetHeadPic(c->Class, headDir, gunState);
	pics.HeadOffset = GetActorDrawOffset(
		pics.Head, BODY_PART_HEAD, c->Class->Sprites, anim, frame, dir);

	// Body
	const bool isArmed = gunPics != NULL;
	pics.Body = GetBodyPic(
		&gPicManager, c->Class->Sprites, dir, anim, frame, isArmed);
	pics.BodyOffset = GetActorDrawOffset(
		pics.Body, BODY_PART_BODY, c->Class->Sprites, anim, frame, dir);

	// Legs
	pics.Legs = GetLegsPic(
		&gPicManager, c->Class->Sprites, dir, anim, frame);
	pics.LegsOffset = GetActorDrawOffset(
		pics.Legs, BODY_PART_LEGS, c->Class->Sprites, anim, frame, dir);

	// Gun
	pics.Gun = NULL;
	if (isArmed)
	{
		pics.Gun = GetGunPic(gunPics, dir, gunState);
		pics.GunOffset = GetActorDrawOffset(
			pics.Gun, BODY_PART_GUN, c->Class->Sprites, anim, frame, dir);
	}

	// Determine draw order based on the direction the player is facing
	for (BodyPart bp = BODY_PART_HEAD; bp < BODY_PART_COUNT; bp++)
	{
		const BodyPart drawOrder = c->Class->Sprites->Order[dir][bp];
		switch (drawOrder)
		{
		case BODY_PART_HEAD:
			pics.OrderedPics[bp] = pics.Head;
			pics.OrderedOffsets[bp] = pics.HeadOffset;
			break;
		case BODY_PART_BODY:
			pics.OrderedPics[bp] = pics.Body;
			pics.OrderedOffsets[bp] = pics.BodyOffset;
			break;
		case BODY_PART_LEGS:
			pics.OrderedPics[bp] = pics.Legs;
			pics.OrderedOffsets[bp] = pics.LegsOffset;
			break;
		case BODY_PART_GUN:
			pics.OrderedPics[bp] = pics.Gun;
			pics.OrderedOffsets[bp] = pics.GunOffset;
			break;
		default:
			break;
		}
	}

	pics.Sprites = c->Class->Sprites;

	return pics;
}
static Character *ActorGetCharacterMutable(TActor *a)
{
	if (a->PlayerUID >= 0)
	{
		return &PlayerDataGetByUID(a->PlayerUID)->Char;
	}
	return CArrayGet(&gCampaign.Setting.characters.OtherChars, a->charId);
}

static void DrawDyingBody(
	GraphicsDevice *g, const ActorPics *pics, const Vec2i pos);
void DrawActorPics(const ActorPics *pics, const Vec2i pos)
{
	if (pics->IsDead)
	{
		if (pics->IsDying)
		{
			DrawDyingBody(&gGraphicsDevice, pics, pos);
		}
	}
	else
	{
		// Draw shadow
		if (!pics->IsTransparent)
		{
			DrawShadow(&gGraphicsDevice, pos, Vec2iNew(8, 6));
		}
		for (int i = 0; i < BODY_PART_COUNT; i++)
		{
			const Pic *pic = pics->OrderedPics[i];
			if (pic == NULL)
			{
				continue;
			}
			const Vec2i drawPos = Vec2iAdd(pos, pics->OrderedOffsets[i]);
			if (pics->IsTransparent)
			{
				BlitBackground(
					&gGraphicsDevice, pic, drawPos, pics->Tint, true);
			}
			else if (pics->Mask != NULL)
			{
				BlitMasked(&gGraphicsDevice, pic, drawPos, *pics->Mask, true);
			}
			else
			{
				BlitCharMultichannel(
					&gGraphicsDevice, pic, drawPos, pics->Colors);
			}
		}
	}
}
static void DrawLaserSightSingle(
	const Vec2i from, const double radians, const int range,
	const color_t color);
void DrawLaserSight(
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
		picPos, Vec2iFull2Real(ActorGetGunMuzzleOffset(a)));
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

void DrawActorHighlight(
	const ActorPics *pics, const Vec2i pos, const color_t color)
{
	// Do not highlight dead, dying or transparent characters
	if (pics->IsDead || pics->IsTransparent)
	{
		return;
	}
	BlitPicHighlight(
		&gGraphicsDevice, pics->Head, Vec2iAdd(pos, pics->HeadOffset), color);
	if (pics->Body != NULL)
	{
		BlitPicHighlight(
			&gGraphicsDevice, pics->Body, Vec2iAdd(pos, pics->BodyOffset),
			color);
	}
	if (pics->Legs != NULL)
	{
		BlitPicHighlight(
			&gGraphicsDevice, pics->Legs, Vec2iAdd(pos, pics->LegsOffset),
			color);
	}
	if (pics->Gun != NULL)
	{
		BlitPicHighlight(
			&gGraphicsDevice, pics->Gun, Vec2iAdd(pos, pics->GunOffset), color);
	}
}

static void DrawChatter(
	const TTileItem *ti, DrawBuffer *b, const Vec2i offset);
void DrawChatters(DrawBuffer *b, const Vec2i offset)
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
	if (!ConfigGetBool(&gConfig, "Graphics.ShowHUD"))
	{
		return;
	}

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

const Pic *GetHeadPic(
	const CharacterClass *c, const direction_e dir, const gunstate_e gunState)
{
	// If firing, draw the firing head pic
	const int row =
		(gunState == GUNSTATE_FIRING || gunState == GUNSTATE_RECOIL) ? 1 : 0;
	const int idx = (int)dir + row * 8;
	return CPicGetPic(&c->HeadPics, idx);
}
static const Pic *GetBodyPic(
	PicManager *pm, const CharSprites *cs, const direction_e dir,
	const ActorAnimation anim, const int frame, const bool isArmed)
{
	const int stride = anim == ACTORANIMATION_IDLE ? 1 : 8;
	const int col = frame % stride;
	const int row = (int)dir;
	const int idx = col + row * stride;
	char buf[CDOGS_PATH_MAX];
	sprintf(
		buf, "chars/bodies/%s/upper_%s%s",
		cs->Name,
		anim == ACTORANIMATION_IDLE ? "idle" : "run",
		isArmed ? "_handgun" : "");	// TODO: other gun holding poses
	return CArrayGet(&PicManagerGetSprites(pm, buf)->pics, idx);
}
static const Pic *GetLegsPic(
	PicManager *pm, const CharSprites *cs, const direction_e dir,
	const ActorAnimation anim, const int frame)
{
	const int stride = anim == ACTORANIMATION_IDLE ? 1 : 8;
	const int col = frame % stride;
	const int row = (int)dir;
	const int idx = col + row * stride;
	char buf[CDOGS_PATH_MAX];
	sprintf(
		buf, "chars/bodies/%s/legs_%s",
		cs->Name, anim == ACTORANIMATION_IDLE ? "idle" : "run");
	return CArrayGet(&PicManagerGetSprites(pm, buf)->pics, idx);
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
	ActorPics pics = GetCharacterPics(
		c, d, ACTORANIMATION_IDLE, 0, NULL, GUNSTATE_READY,
		false, NULL, NULL, 0);
	DrawActorPics(&pics, pos);
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
	const Character *c, const direction_e dir, const Vec2i pos)
{
	const Pic *head = GetHeadPic(c->Class, dir, GUNSTATE_READY);
	const Vec2i drawPos = Vec2iMinus(pos, Vec2iNew(
		head->size.x / 2, head->size.y / 2));
	BlitCharMultichannel(&gGraphicsDevice, head, drawPos, &c->Colors);
}
#define DYING_BODY_OFFSET 3
static void DrawDyingBody(
	GraphicsDevice *g, const ActorPics *pics, const Vec2i pos)
{
	const Pic *body = pics->Body;
	const Vec2i drawPos = Vec2iMinus(pos, Vec2iNew(
		body->size.x / 2, body->size.y / 2 + DYING_BODY_OFFSET));
	const color_t mask = pics->Mask != NULL ? *pics->Mask : colorWhite;
	BlitMasked(g, pics->Body, drawPos, mask, true);
}
