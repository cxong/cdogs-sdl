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

	Copyright (c) 2013-2021, 2023, 2025 Cong Xu
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
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "actors.h"
#include "algorithms.h"
#include "blit.h"
#include "config.h"
#include "draw/draw.h"
#include "draw/drawtools.h"
#include "font.h"
#include "game_events.h"
#include "net_util.h"
#include "objs.h"
#include "pic_manager.h"
#include "pics.h"

#define TRANSPARENT_ACTOR_ALPHA 64

static struct vec2i GetActorDrawOffset(
	const Pic *pic, const BodyPart part, const CharSprites *cs,
	const ActorAnimation anim, const int frame, const direction_e d,
	const gunstate_e state)
{
	if (pic == NULL)
	{
		return svec2i_zero();
	}
	struct vec2i offset = svec2i_scale_divide(pic->size, -2);
	offset = svec2i_subtract(
		offset, CharSpritesGetOffset(
					cs->Offsets.Frame[part],
					anim == ACTORANIMATION_WALKING ? "run" : "idle", frame));
	offset = svec2i_add(offset, svec2i_assign_vec2(cs->Offsets.Dir[part][d]));
	if ((part == BODY_PART_GUN_R || part == BODY_PART_GUN_L) &&
		state == GUNSTATE_RECOIL)
	{
		// Offset the gun pic towards the player
		const struct vec2i recoilOffsets[DIRECTION_COUNT] = {
			{0, 1},	 {-1, 1}, {-1, 0}, {-1, -1},
			{0, -1}, {1, -1}, {1, 0},  {1, 1}};
		offset = svec2i_add(offset, recoilOffsets[d]);
	}
	return offset;
}

static const char *GetFestiveHat(void)
{
	static struct tm *t = NULL;
	if (t == NULL)
	{
		time_t now = time(NULL);
		t = localtime(&now);
	}
	if (t->tm_mon + 1 == 1 && t->tm_mday == 1)
		return "party";
	if (t->tm_mon + 1 == 3 && t->tm_mday == 17)
		return "leprechaun";
	if (t->tm_mon + 1 == 5 && t->tm_mday == 5)
		return "sombrero";
	if (t->tm_mon + 1 == 12 && t->tm_mday == 25)
		return "santa";
	return NULL;
}

static direction_e GetLegDirAndFrame(
	const TActor *a, const direction_e bodyDir, int *frame);
static ActorPics GetUnorderedPics(
	const Character *c, const direction_e dir, const direction_e legDir,
	const ActorAnimation anim, const int frame, const WeaponClass *gun,
	const gunstate_e barrelStates[MAX_BARRELS], const bool isGrimacing,
	const color_t shadowMask, const color_t *mask, const CharColors *colors,
	const int deadPic);
static void UpdatePilotHeadPic(
	ActorPics *pics, const TActor *a, const direction_e dir);
static void ReorderPics(
	ActorPics *pics, const Character *c, const direction_e dir,
	const WeaponClass *gun, const gunstate_e barrelStates[MAX_BARRELS]);
ActorPics GetCharacterPicsFromActor(const TActor *a)
{
	if (a->vehicleUID != -1)
	{
		// Don't draw if piloting a vehicle - this actor will be drawn
		// when the vehicle is drawn
		ActorPics p;
		memset(&p, 0, sizeof p);
		return p;
	}
	const Character *c = ActorGetCharacter(a);
	const Weapon *gun = ACTOR_GET_WEAPON(a);

	color_t mask;
	bool hasStatus = false;
	if (a->flamed)
	{
		mask = colorRed;
		hasStatus = true;
	}
	else if (a->poisoned)
	{
		mask = colorPoison;
		hasStatus = true;
	}
	else if (a->petrified)
	{
		mask = colorGray;
		hasStatus = true;
	}
	else if (a->confused)
	{
		mask = colorPurple;
		hasStatus = true;
	}

	const CharColors allBlack = CharColorsFromOneColor(colorBlack);
	const CharColors allWhite = CharColorsFromOneColor(colorWhite);
	const bool isTransparent = !!(a->flags & FLAGS_SEETHROUGH);
	const CharColors *colors = NULL;
	const color_t *maskP = NULL;
	color_t shadowMask = colorTransparent;
	if (isTransparent)
	{
		colors = &allBlack;
		maskP = &mask;
		mask.a = TRANSPARENT_ACTOR_ALPHA;
	}
	else
	{
		shadowMask = a->PlayerUID >= 0 ? c->Colors.Body : colorBlack;
		if (hasStatus)
		{
			maskP = &mask;
			colors = &allWhite;
		}
	}

	const direction_e dir = RadiansToDirection(a->DrawRadians);
	int frame;
	const direction_e legDir = GetLegDirAndFrame(a, dir, &frame);
	gunstate_e gunStates[MAX_BARRELS];
	memset(&gunStates, 0, sizeof gunStates);
	for (int i = 0; gun->Gun != NULL && i < WeaponClassNumBarrels(gun->Gun);
		 i++)
	{
		gunStates[i] = gun->barrels[i].state;
	}

	ActorPics pics = GetUnorderedPics(
		c, dir, legDir, a->anim.Type, frame, gun->Gun, gunStates,
		ActorIsGrimacing(a), shadowMask, maskP, colors, a->dead);
	UpdatePilotHeadPic(&pics, a, dir);
	ReorderPics(&pics, c, dir, gun->Gun, gunStates);
	return pics;
}
static void UpdatePilotHeadPic(
	ActorPics *pics, const TActor *a, const direction_e dir)
{
	if (a->pilotUID == a->uid)
	{
		return;
	}
	// If this is a vehicle, take the head/hair pic from the pilot
	memset(&pics->HeadParts, 0, sizeof pics->HeadParts);
	const TActor *pilot = ActorGetByUID(a->pilotUID);
	if (pilot == NULL)
	{
		return;
	}
	const Character *c = ActorGetCharacter(pilot);
	const bool grimace = ActorIsGrimacing(a);
	pics->Head = GetHeadPic(c->Class, dir, grimace, &c->Colors);

	for (HeadPart hp = HEAD_PART_HAIR; hp < HEAD_PART_COUNT; hp++)
	{
		if (c->Class->HasHeadParts[hp])
		{
			pics->HeadParts[hp] =
				GetHeadPartPic(c->HeadParts[hp], hp, dir, grimace, &c->Colors);
		}
	}
}
ActorPics GetCharacterPics(
	const Character *c, const direction_e dir, const direction_e legDir,
	const ActorAnimation anim, const int frame, const WeaponClass *gun,
	const gunstate_e barrelStates[MAX_BARRELS], const bool isGrimacing,
	const color_t shadowMask, const color_t *mask, const CharColors *colors,
	const int deadPic)
{
	ActorPics pics = GetUnorderedPics(
		c, dir, legDir, anim, frame, gun, barrelStates, isGrimacing,
		shadowMask, mask, colors, deadPic);

	ReorderPics(&pics, c, dir, gun, barrelStates);

	return pics;
}
static const Pic *GetBodyPic(
	PicManager *pm, const CharSprites *cs, const direction_e dir,
	const ActorAnimation anim, const int frame, const int numBarrels,
	const int grips, const gunstate_e barrelState, const CharColors *colors);
static const Pic *GetLegsPic(
	PicManager *pm, const CharSprites *cs, const direction_e dir,
	const ActorAnimation anim, const int frame, const CharColors *colors);
static const Pic *GetGunPic(
	PicManager *pm, const char *gunSprites, const direction_e dir,
	const int gunState, const CharColors *colors);
static ActorPics GetUnorderedPics(
	const Character *c, const direction_e dir, const direction_e legDir,
	const ActorAnimation anim, const int frame, const WeaponClass *gun,
	const gunstate_e barrelStates[MAX_BARRELS], const bool isGrimacing,
	const color_t shadowMask, const color_t *mask, const CharColors *colors,
	const int deadPic)
{
	ActorPics pics;
	memset(&pics, 0, sizeof pics);

	pics.Sprites = c->Class->Sprites;

	// Dummy return to handle invalid character class
	if (c->Class == NULL)
	{
		pics.IsDead = true;
		pics.IsDying = true;
		pics.Body = NULL;
		pics.OrderedPics[0] = pics.Body;
		return pics;
	}

	pics.ShadowMask = shadowMask;
	if (mask != NULL)
	{
		pics.Mask = *mask;
	}
	else
	{
		pics.Mask = colorWhite;
	}

	// If the actor is dead, simply draw a dying animation
	pics.IsDead = deadPic > 0;
	if (pics.IsDead)
	{
		const NamedSprites *deathSprites =
			CharacterClassGetDeathSprites(c->Class, &gPicManager);
		if (deadPic - 1 < (int)deathSprites->pics.size)
		{
			pics.IsDying = true;
			pics.Body = CArrayGet(&deathSprites->pics, deadPic - 1);
			pics.OrderedPics[0] = pics.Body;
		}
		return pics;
	}

	if (colors == NULL)
	{
		colors = &c->Colors;
	}

	// Head
	direction_e headDir = dir;
	// If idle, turn head left/right on occasion
	if (anim == ACTORANIMATION_IDLE)
	{
		if (frame == IDLEHEAD_LEFT)
			headDir = (dir + 7) % 8;
		else if (frame == IDLEHEAD_RIGHT)
			headDir = (dir + 1) % 8;
	}
	bool grimace = isGrimacing;
	const int numBarrels = (barrelStates == NULL || gun == NULL ||
							WC_BARREL_ATTR(*gun, Sprites, 0) == NULL)
							   ? 0
							   : WeaponClassNumBarrels(gun);
	for (int i = 0; i < MAX_BARRELS; i++)
	{
		if (barrelStates[i] == GUNSTATE_FIRING ||
			barrelStates[i] == GUNSTATE_RECOIL)
		{
			grimace = true;
			break;
		}
	}
	const int grips = gun == NULL ? 0 : WC_BARREL_ATTR(*gun, Grips, 0);
	pics.Head = GetHeadPic(c->Class, headDir, grimace, colors);
	pics.HeadOffset = GetActorDrawOffset(
		pics.Head, BODY_PART_HEAD, c->Class->Sprites, anim, frame, dir,
		GUNSTATE_READY);

	const char *festiveHat = GetFestiveHat();
	for (HeadPart hp = HEAD_PART_HAIR; hp < HEAD_PART_COUNT; hp++)
	{
		// Holiday override
		const bool hasHeadPart = c->Class->HasHeadParts[hp] ||
								 (hp == HEAD_PART_HAT && festiveHat != NULL);
		if (hasHeadPart)
		{
			const char *headPart = (hp == HEAD_PART_HAT && festiveHat)
									   ? festiveHat
									   : c->HeadParts[hp];
			pics.HeadParts[hp] =
				GetHeadPartPic(headPart, hp, headDir, grimace, colors);
			pics.HeadPartOffsets[hp] = GetActorDrawOffset(
				pics.HeadParts[hp], BODY_PART_HEAD, c->Class->Sprites, anim,
				frame, dir, GUNSTATE_READY);
		}
	}

	// Gun
	for (int i = 0; i < numBarrels; i++)
	{
		pics.Guns[i] = GetGunPic(
			&gPicManager, WC_BARREL_ATTR(*gun, Sprites, i), dir,
			barrelStates[i], colors);
		if (pics.Guns[i] != NULL)
		{
			pics.GunOffsets[i] = GetActorDrawOffset(
				pics.Guns[i], i == 0 ? BODY_PART_GUN_R : BODY_PART_GUN_L,
				c->Class->Sprites, anim, frame,
				i == 0 ? dir : DirectionMirrorX(dir), barrelStates[i]);
			if (i == 1)
			{
				const int xHalf = pics.Guns[i]->size.x / 2;
				const int xOffsetOrig = pics.GunOffsets[i].x + xHalf;
				pics.GunOffsets[i].x = -xOffsetOrig - xHalf;
			}
		}
	}

	// Body
	pics.Body = GetBodyPic(
		&gPicManager, c->Class->Sprites, dir, anim, frame, numBarrels, grips,
		barrelStates[0], colors);
	pics.BodyOffset = GetActorDrawOffset(
		pics.Body, BODY_PART_BODY, c->Class->Sprites, anim, frame, dir,
		GUNSTATE_READY);

	// Legs
	pics.Legs = GetLegsPic(
		&gPicManager, c->Class->Sprites, legDir, anim, frame, colors);
	pics.LegsOffset = GetActorDrawOffset(
		pics.Legs, BODY_PART_LEGS, c->Class->Sprites, anim, frame, legDir,
		GUNSTATE_READY);
	return pics;
}
static direction_e GetLegDirAndFrame(
	const TActor *a, const direction_e bodyDir, int *frame)
{
	*frame = AnimationGetFrame(&a->anim);
	const struct vec2 vel = svec2_add(a->MoveVel, a->thing.Vel);
	if (svec2_is_zero(vel))
	{
		return bodyDir;
	}
	const direction_e legDir = RadiansToDirection(svec2_angle(vel) + MPI_2);
	// Walk backwards if the leg dir is >90 degrees from body dir
	const int dirDiff = abs((int)bodyDir - (int)legDir);
	const bool reversed = dirDiff > 2 && dirDiff < 6;
	if (reversed)
	{
		*frame = ANIMATION_MAX_FRAMES - *frame;
		return DirectionOpposite(legDir);
	}
	return legDir;
}
static void ReorderPics(
	ActorPics *pics, const Character *c, const direction_e dir,
	const WeaponClass *gun, const gunstate_e barrelStates[MAX_BARRELS])
{
	// Determine draw order based on the direction the player is facing
	// Rotate direction left for 2-grip guns, as the gun is held in front
	// of the actor
	const int grips = gun == NULL ? 0 : WC_BARREL_ATTR(*gun, Grips, 0);
	const direction_e drawOrderDir =
		grips == 2 && barrelStates[0] == GUNSTATE_READY
			? DirectionRotate(dir, -1)
			: dir;
	for (int bp = 0; bp < BODY_PART_COUNT; bp++)
	{
		const BodyPart drawOrder = c->Class->Sprites->Order[drawOrderDir][bp];
		switch (drawOrder)
		{
		case BODY_PART_HEAD:
			pics->OrderedPics[bp] = pics->Head;
			pics->OrderedOffsets[bp] = pics->HeadOffset;
			break;
		case BODY_PART_HAIR:
			pics->OrderedPics[bp] = pics->HeadParts[HEAD_PART_HAIR];
			pics->OrderedOffsets[bp] = pics->HeadPartOffsets[HEAD_PART_HAIR];
			break;
		case BODY_PART_FACEHAIR:
			pics->OrderedPics[bp] = pics->HeadParts[HEAD_PART_FACEHAIR];
			pics->OrderedOffsets[bp] =
				pics->HeadPartOffsets[HEAD_PART_FACEHAIR];
			break;
		case BODY_PART_HAT:
			pics->OrderedPics[bp] = pics->HeadParts[HEAD_PART_HAT];
			pics->OrderedOffsets[bp] = pics->HeadPartOffsets[HEAD_PART_HAT];
			break;
		case BODY_PART_GLASSES:
			pics->OrderedPics[bp] = pics->HeadParts[HEAD_PART_GLASSES];
			pics->OrderedOffsets[bp] =
				pics->HeadPartOffsets[HEAD_PART_GLASSES];
			break;
		case BODY_PART_BODY:
			pics->OrderedPics[bp] = pics->Body;
			pics->OrderedOffsets[bp] = pics->BodyOffset;
			break;
		case BODY_PART_LEGS:
			pics->OrderedPics[bp] = pics->Legs;
			pics->OrderedOffsets[bp] = pics->LegsOffset;
			break;
		case BODY_PART_GUN_R:
			pics->OrderedPics[bp] = pics->Guns[0];
			pics->OrderedOffsets[bp] = pics->GunOffsets[0];
			break;
		case BODY_PART_GUN_L:
			pics->OrderedPics[bp] = pics->Guns[1];
			pics->OrderedOffsets[bp] = pics->GunOffsets[1];
			break;
		default:
			break;
		}
	}
}

static void DrawDyingBody(
	GraphicsDevice *g, const ActorPics *pics, const struct vec2i pos,
	const Rect2i bounds);
void DrawActorPics(
	const ActorPics *pics, const struct vec2i pos, const Rect2i bounds)
{
	if (pics->IsDead)
	{
		if (pics->IsDying)
		{
			DrawDyingBody(&gGraphicsDevice, pics, pos, bounds);
		}
	}
	else
	{
		// TODO: use bounds
		DrawShadow(&gGraphicsDevice, pos, svec2(8, 6), pics->ShadowMask);
		for (int i = 0; i < BODY_PART_COUNT; i++)
		{
			const Pic *pic = pics->OrderedPics[i];
			if (pic == NULL)
			{
				continue;
			}
			const struct vec2i drawPos =
				svec2i_add(pos, pics->OrderedOffsets[i]);
			const Rect2i drawSrc =
				Rect2iIsZero(bounds)
					? bounds
					: Rect2iNew(
						  svec2i_subtract(bounds.Pos, drawPos),
						  svec2i_subtract(bounds.Size, bounds.Pos));
			PicRender(
				pic, gGraphicsDevice.gameWindow.renderer, drawPos, pics->Mask,
				0, svec2_one(), SDL_FLIP_NONE, drawSrc);
		}
	}
}
static void DrawLaserSightSingle(
	const struct vec2i from, const float radians, const int range,
	const color_t color);
void DrawLaserSight(
	const ActorPics *pics, const TActor *a, const struct vec2i picPos)
{
	// Don't draw if dead or transparent
	if (pics->IsDead || ColorEquals(pics->ShadowMask, colorTransparent))
		return;
	// Check config
	const LaserSight ls = ConfigGetEnum(&gConfig, "Game.LaserSight");
	if (ls != LASER_SIGHT_ALL &&
		!(ls == LASER_SIGHT_PLAYERS && a->PlayerUID >= 0))
	{
		return;
	}
	// Draw weapon indicators
	const Weapon *w = ACTOR_GET_WEAPON(a);
	const WeaponClass *wc = w->Gun;
	if (wc == NULL)
	{
		return;
	}
	for (int i = 0; i < WeaponClassNumBarrels(wc); i++)
	{
		struct vec2i muzzlePos = svec2i_add(
			picPos, svec2i_assign_vec2(ActorGetMuzzleOffset(a, w, i)));
		const WeaponClass *wcb = WeaponClassGetBarrel(wc, i);
		muzzlePos.y -= wcb->u.Normal.MuzzleHeight / Z_FACTOR;
		const float radians =
			dir2radians[a->direction] + wcb->u.Normal.AngleOffset;
		const int range = (int)WeaponClassGetRange(wcb);
		color_t color = colorCyan;
		color.a = 64;
		const float spreadHalf =
			(wcb->u.Normal.Spread.Count - 1) * wcb->u.Normal.Spread.Width / 2 +
			wcb->u.Normal.Recoil / 2;
		if (spreadHalf > 0)
		{
			DrawLaserSightSingle(
				muzzlePos, radians - spreadHalf, range, color);
			DrawLaserSightSingle(
				muzzlePos, radians + spreadHalf, range, color);
		}
		else
		{
			DrawLaserSightSingle(muzzlePos, radians, range, color);
		}
	}
}
static void DrawLaserSightSingle(
	const struct vec2i from, const float radians, const int range,
	const color_t color)
{
	const struct vec2 v =
		svec2_scale(Vec2FromRadiansScaled(radians), (float)range);
	const struct vec2i to = svec2i_add(from, svec2i_assign_vec2(v));
	DrawLine(from, to, color);
}

const Pic *GetHeadPic(
	const CharacterClass *c, const direction_e dir, const bool isGrimacing,
	const CharColors *colors)
{
	if (strlen(c->HeadSprites) == 0)
	{
		return NULL;
	}
	// If firing, draw the firing head pic
	const int row = isGrimacing ? 1 : 0;
	const int idx = (int)dir + row * 8;
	// Get or generate masked sprites
	const NamedSprites *ns =
		PicManagerGetCharSprites(&gPicManager, c->HeadSprites, colors);
	return CArrayGet(&ns->pics, idx);
}
const Pic *GetHeadPartPic(
	const char *name, const HeadPart hp, const direction_e dir,
	const bool isGrimacing, const CharColors *colors)
{
	if (name == NULL)
	{
		return NULL;
	}
	const int row = isGrimacing ? 1 : 0;
	const int idx = (int)dir + row * 8;
	// Get or generate masked sprites
	char buf[CDOGS_PATH_MAX];
	const char *subpaths[] = {"hairs", "facehairs", "hats", "glasses"};
	sprintf(buf, "chars/%s/%s", subpaths[hp], name);
	const NamedSprites *ns =
		PicManagerGetCharSprites(&gPicManager, buf, colors);
	if (ns == NULL)
	{
		return NULL;
	}
	return CArrayGet(&ns->pics, idx);
}
static const Pic *GetBodyPic(
	PicManager *pm, const CharSprites *cs, const direction_e dir,
	const ActorAnimation anim, const int frame, const int numBarrels,
	const int grips, const gunstate_e barrelState, const CharColors *colors)
{
	const int stride = anim == ACTORANIMATION_WALKING ? 8 : 1;
	const int col = frame % stride;
	const int row = (int)dir;
	const int idx = col + row * stride;
	char buf[CDOGS_PATH_MAX];
	CASSERT(numBarrels <= 2, "up to 2 barrels supported");
	const NamedSprites *ns = NULL;
	const char *upperPose = "";
	if (numBarrels == 1)
	{
		upperPose = "_handgun";
	}
	if (numBarrels == 2)
	{
		upperPose = "_dualgun";
	}
	if (grips == 2)
	{
		upperPose = "_rifle";
		if (barrelState == GUNSTATE_FIRING || barrelState == GUNSTATE_RECOIL)
		{
			upperPose = "_riflefire";
		}
	}
	for (;;)
	{
		sprintf(
			buf, "chars/bodies/%s/upper_%s%s", cs->Name,
			anim == ACTORANIMATION_WALKING ? "run" : "idle",
			upperPose); // TODO: other gun holding poses
		// Get or generate masked sprites
		ns = PicManagerGetCharSprites(pm, buf, colors);
		// TODO: provide dualgun sprites for all body types
		if (ns == NULL && strcmp(upperPose, "_handgun") != 0)
		{
			upperPose = "_handgun";
			continue;
		}
		break;
	}
	return CArrayGet(&ns->pics, idx);
}
static const Pic *GetLegsPic(
	PicManager *pm, const CharSprites *cs, const direction_e dir,
	const ActorAnimation anim, const int frame, const CharColors *colors)
{
	const int stride = anim == ACTORANIMATION_WALKING ? 8 : 1;
	const int col = frame % stride;
	const int row = (int)dir;
	const int idx = col + row * stride;
	char buf[CDOGS_PATH_MAX];
	sprintf(
		buf, "chars/bodies/%s/legs_%s", cs->Name,
		anim == ACTORANIMATION_WALKING ? "run" : "idle");
	// Get or generate masked sprites
	const NamedSprites *ns = PicManagerGetCharSprites(pm, buf, colors);
	return CArrayGet(&ns->pics, idx);
}
static const Pic *GetGunPic(
	PicManager *pm, const char *gunSprites, const direction_e dir,
	const int gunState, const CharColors *colors)
{
	const int idx = (gunState == GUNSTATE_READY ? 8 : 0) + dir;
	// Get or generate masked sprites
	const NamedSprites *ns = PicManagerGetCharSprites(pm, gunSprites, colors);
	if (ns == NULL)
	{
		return NULL;
	}
	return CArrayGet(&ns->pics, idx);
}

void DrawCharacterSimple(
	const Character *c, const struct vec2i pos, const direction_e d,
	const bool hilite, const bool showGun, const WeaponClass *gun)
{
	const gunstate_e barrelStates[MAX_BARRELS] = {
		GUNSTATE_READY, GUNSTATE_READY};
	ActorPics pics = GetCharacterPics(
		c, d, d, ACTORANIMATION_IDLE, 0, gun, barrelStates, false, colorBlack,
		NULL, NULL, 0);
	DrawActorPics(&pics, pos, Rect2iZero());
	if (hilite)
	{
		FontCh('>', svec2i_add(pos, svec2i(-8, -16)));
		if (showGun)
		{
			FontStr(c->Gun->name, svec2i_add(pos, svec2i(-8, 8)));
		}
	}
}

void DrawHead(
	SDL_Renderer *renderer, const Character *c, const direction_e dir,
	const struct vec2i pos)
{
	const bool isGrimacing = false;
	const Pic *head = GetHeadPic(c->Class, dir, isGrimacing, &c->Colors);
	const struct vec2i headOffset = GetActorDrawOffset(
		head, BODY_PART_HEAD, c->Class->Sprites, ACTORANIMATION_IDLE, 0,
		DIRECTION_DOWN, GUNSTATE_READY);
	const color_t mask = colorWhite;
	const struct vec2i charOffset = svec2i(0, 12);
	PicRender(
		head, renderer, svec2i_add(svec2i_add(pos, headOffset), charOffset),
		mask, 0, svec2_one(), SDL_FLIP_NONE, Rect2iZero());

	for (HeadPart hp = HEAD_PART_HAIR; hp < HEAD_PART_COUNT; hp++)
	{
		if (c->Class->HasHeadParts[hp])
		{
			const Pic *pic = GetHeadPartPic(
				c->HeadParts[hp], hp, dir, isGrimacing, &c->Colors);
			if (pic)
			{
				const struct vec2i headPartOffset = GetActorDrawOffset(
					pic, BODY_PART_HEAD, c->Class->Sprites,
					ACTORANIMATION_IDLE, 0, DIRECTION_DOWN, GUNSTATE_READY);
				PicRender(
					pic, renderer,
					svec2i_add(svec2i_add(pos, headPartOffset), charOffset),
					mask, 0, svec2_one(), SDL_FLIP_NONE, Rect2iZero());
			}
		}
	}
}
#define DYING_BODY_OFFSET 3
static void DrawDyingBody(
	GraphicsDevice *g, const ActorPics *pics, const struct vec2i pos,
	const Rect2i bounds)
{
	const Pic *body = pics->Body;
	const struct vec2i drawPos = svec2i_subtract(
		pos, svec2i(body->size.x / 2, body->size.y / 2 + DYING_BODY_OFFSET));
	const Rect2i drawSrc = Rect2iIsZero(bounds)
							   ? bounds
							   : Rect2iNew(
									 svec2i_subtract(bounds.Pos, drawPos),
									 svec2i_subtract(bounds.Size, bounds.Pos));
	PicRender(
		body, g->gameWindow.renderer, drawPos, pics->Mask, 0, svec2_one(),
		SDL_FLIP_NONE, drawSrc);
}
