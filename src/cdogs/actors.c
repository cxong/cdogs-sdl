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
#include "actors.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "actor_placement.h"
#include "ai_utils.h"
#include "ammo.h"
#include "character.h"
#include "collision.h"
#include "config.h"
#include "drawtools.h"
#include "game_events.h"
#include "pic_manager.h"
#include "sounds.h"
#include "defs.h"
#include "objs.h"
#include "pickup.h"
#include "gamedata.h"
#include "triggers.h"
#include "hiscores.h"
#include "mission.h"
#include "game.h"
#include "utils.h"

#define SOUND_LOCK_FOOTSTEP 4
#define FOOTSTEP_DISTANCE_PLUS 380
#define REPEL_STRENGTH 14
#define SLIDE_LOCK 50
#define SLIDE_X (TILE_WIDTH / 3)
#define SLIDE_Y (TILE_HEIGHT / 3)
#define VEL_DECAY_X (TILE_WIDTH * 2)
#define VEL_DECAY_Y (TILE_WIDTH * 2)	// Note: deliberately tile width
#define SOUND_LOCK_WEAPON_CLICK 20


CArray gPlayerIds;

TranslationTable tableFlamed;
TranslationTable tableGreen;
TranslationTable tablePoison;
TranslationTable tableGray;
TranslationTable tableBlack;
TranslationTable tableDarker;
TranslationTable tablePurple;


CArray gActors;
static int sActorUIDs = 0;

static int transitionTable[STATE_COUNT] = {
	0,
	STATE_IDLE,
	STATE_IDLE,
	STATE_WALKING_2,
	STATE_WALKING_3,
	STATE_WALKING_4,
	STATE_WALKING_1
};

static int delayTable[STATE_COUNT] = {
	90,
	60,
	60,
	4,
	4,
	4,
	4,
};


static ActorPics GetCharacterPics(int id)
{
	ActorPics pics;
	memset(&pics, 0, sizeof pics);
	TActor *actor = CArrayGet(&gActors, id);
	direction_e dir = actor->direction;
	direction_e headDir = dir;
	int state = actor->state;
	int headState = state;

	Character *c = actor->Character;
	if (actor->playerIndex >= 0)
	{
		PlayerData *p = CArrayGet(&gPlayerDatas, actor->playerIndex);
		c = &p->Char;
	}
	pics.Table = (TranslationTable *)c->table;
	int f = c->looks.face;
	const Weapon *gun = ActorGetGun(actor);
	int g = gun->Gun->pic;
	gunstate_e gunState = gun->state;

	TOffsetPic body, head, gunPic;

	pics.IsTransparent = !!(actor->flags & FLAGS_SEETHROUGH);

	if (gunState == GUNSTATE_FIRING || gunState == GUNSTATE_RECOIL)
	{
		headState = STATE_COUNT + gunState - GUNSTATE_FIRING;
	}

	if (actor->flamed)
	{
		pics.Table = &tableFlamed;
		pics.Tint = &tintRed;
	}
	else if (actor->poisoned)
	{
		pics.Table = &tableGreen;
		pics.Tint = &tintPoison;
	}
	else if (actor->petrified)
	{
		pics.Table = &tableGray;
		pics.Tint = &tintGray;
	}
	else if (actor->confused)
	{
		pics.Table = &tablePurple;
		pics.Tint = &tintPurple;
	}
	else if (pics.IsTransparent)
	{
		pics.Table = &tableDarker;
		pics.Tint = &tintDarker;
	}

	actor->flags |= FLAGS_VISIBLE;

	if (state == STATE_IDLELEFT)
		headDir = (dir + 7) % 8;
	else if (state == STATE_IDLERIGHT)
		headDir = (dir + 1) % 8;

	int b = g < 0 ? BODY_UNARMED : BODY_ARMED;

	body.dx = cBodyOffset[b][dir].dx;
	body.dy = cBodyOffset[b][dir].dy;
	body.picIndex = cBodyPic[b][dir][state];

	if (actor->dead)
	{
		pics.IsDead = true;
		if (actor->dead <= DEATH_MAX)
		{
			pics.IsDying = true;
			body = cDeathPics[actor->dead - 1];
			pics.Pics[0] = PicFromTOffsetPic(&gPicManager, body);
			pics.OldPics[0] = body.picIndex;
		}
		goto bail;
	}

	head.dx = cNeckOffset[b][dir].dx + cHeadOffset[f][headDir].dx;
	head.dy = cNeckOffset[b][dir].dy + cHeadOffset[f][headDir].dy;
	head.picIndex = cHeadPic[f][headDir][headState];

	if (g >= 0)
	{
		gunPic.dx =
		    cGunHandOffset[b][dir].dx +
		    cGunPics[g][dir][gunState].dx;
		gunPic.dy =
		    cGunHandOffset[b][dir].dy +
		    cGunPics[g][dir][gunState].dy;
		gunPic.picIndex = cGunPics[g][dir][gunState].picIndex;
	}
	else
	{
		gunPic.picIndex = -1;
	}

	switch (dir)
	{
	case DIRECTION_UP:
	case DIRECTION_UPRIGHT:
		pics.Pics[0] = PicFromTOffsetPic(&gPicManager, gunPic);
		pics.Pics[1] = PicFromTOffsetPic(&gPicManager, head);
		pics.Pics[2] = PicFromTOffsetPic(&gPicManager, body);
		pics.OldPics[0] = gunPic.picIndex;
		pics.OldPics[1] = head.picIndex;
		pics.OldPics[2] = body.picIndex;
		break;

	case DIRECTION_RIGHT:
	case DIRECTION_DOWNRIGHT:
	case DIRECTION_DOWN:
	case DIRECTION_DOWNLEFT:
		pics.Pics[0] = PicFromTOffsetPic(&gPicManager, body);
		pics.Pics[1] = PicFromTOffsetPic(&gPicManager, head);
		pics.Pics[2] = PicFromTOffsetPic(&gPicManager, gunPic);
		pics.OldPics[0] = body.picIndex;
		pics.OldPics[1] = head.picIndex;
		pics.OldPics[2] = gunPic.picIndex;
		break;

	case DIRECTION_LEFT:
	case DIRECTION_UPLEFT:
		pics.Pics[0] = PicFromTOffsetPic(&gPicManager, gunPic);
		pics.Pics[1] = PicFromTOffsetPic(&gPicManager, body);
		pics.Pics[2] = PicFromTOffsetPic(&gPicManager, head);
		pics.OldPics[0] = gunPic.picIndex;
		pics.OldPics[1] = body.picIndex;
		pics.OldPics[2] = head.picIndex;
		break;
	default:
		assert(0 && "invalid direction");
		goto bail;
	}

bail:
	return pics;
}


void SetStateForActor(TActor * actor, int state)
{
	actor->state = state;
	actor->stateCounter = delayTable[state];
}

void UpdateActorState(TActor * actor, int ticks)
{
	Weapon *gun = ActorGetGun(actor);
	WeaponUpdate(gun, ticks, actor->Pos, actor->direction);

	if (actor->health > 0)
	{
		actor->flamed = MAX(0, actor->flamed - ticks);
		if (actor->poisoned)
		{
			if ((actor->poisoned & 7) == 0)
			{
				InjureActor(actor, 1);
			}
			actor->poisoned = MAX(0, actor->poisoned - ticks);
		}
		actor->petrified = MAX(0, actor->petrified - ticks);
		actor->confused = MAX(0, actor->confused - ticks);
	}
	
	actor->slideLock = MAX(0, actor->slideLock - ticks);

	actor->stateCounter = MAX(0, actor->stateCounter - ticks);
	if (actor->stateCounter > 0)
	{
		return;
	}

	if (actor->health <= 0) {
		actor->dead++;
		actor->stateCounter = 4;
		actor->tileItem.flags = 0;
		return;
	}

	// Footstep sounds
	if (ConfigGetBool(&gConfig, "Sound.Footsteps") &&
		(actor->state == STATE_WALKING_1 ||
		actor->state == STATE_WALKING_2 ||
		actor->state == STATE_WALKING_3 ||
		actor->state == STATE_WALKING_4) &&
		actor->soundLock <= 0)
	{
		SoundPlayAtPlusDistance(
			&gSoundDevice,
			gSoundDevice.footstepSound,
			Vec2iNew(actor->tileItem.x, actor->tileItem.y),
			FOOTSTEP_DISTANCE_PLUS);
		actor->soundLock = SOUND_LOCK_FOOTSTEP;
	}

	if (actor->state == STATE_IDLE && !actor->petrified)
		SetStateForActor(actor, ((rand() & 1) != 0 ?
					 STATE_IDLELEFT :
					 STATE_IDLERIGHT));
	else
		SetStateForActor(actor, transitionTable[actor->state]);

	// Sound lock
	actor->soundLock = MAX(0, actor->soundLock - ticks);

	// Chatting
	actor->ChatterCounter = MAX(0, actor->ChatterCounter - ticks);
	if (actor->ChatterCounter == 0)
	{
		// Stop chatting
		actor->Chatter = NULL;
	}
}


static void CheckTrigger(const Vec2i tilePos)
{
	const Tile *t = MapGetTile(&gMap, tilePos);
	int i;
	for (i = 0; i < (int)t->triggers.size; i++)
	{
		Trigger **tp = CArrayGet(&t->triggers, i);
		if (TriggerCanActivate(*tp, gMission.flags))
		{
			GameEvent e = GameEventNew(GAME_EVENT_TRIGGER);
			e.u.Trigger.Id = (*tp)->id;
			e.u.Trigger.TilePos = tilePos;
			GameEventsEnqueue(&gGameEvents, e);
		}
	}
}

static Vec2i GetConstrainedFullPos(
	const Map *map, const Vec2i fromFull, const Vec2i toFull,
	const Vec2i size);
static void CheckPickups(TActor *actor, const Vec2i realPos);
bool TryMoveActor(TActor *actor, Vec2i pos)
{
	CASSERT(!Vec2iEqual(actor->Pos, pos), "trying to move to same position");

	actor->CanPickupSpecial = false;

	const Vec2i oldPos = actor->Pos;
	pos = GetConstrainedFullPos(&gMap, actor->Pos, pos, actor->tileItem.size);
	if (Vec2iEqual(oldPos, pos))
	{
		return false;
	}

	Vec2i realPos = Vec2iFull2Real(pos);
	TTileItem *target = CollideGetFirstItem(
		&actor->tileItem, realPos, TILEITEM_IMPASSABLE,
		CalcCollisionTeam(1, actor),
		IsPVP(gCampaign.Entry.Mode));
	if (target)
	{
		Vec2i realXPos, realYPos;

		if (actor->playerIndex >= 0 && target->kind == KIND_CHARACTER)
		{
			TActor *otherCharacter = CArrayGet(&gActors, target->id);
			CASSERT(otherCharacter->isInUse, "Cannot find nonexistent player");
			if (otherCharacter
			    && (otherCharacter->flags & FLAGS_PRISONER) !=
			    0) {
				otherCharacter->flags &= ~FLAGS_PRISONER;
				UpdateMissionObjective(
					&gMission,
					otherCharacter->tileItem.flags,
					OBJECTIVE_RESCUE,
					actor->playerIndex,
					Vec2iNew(actor->tileItem.x, actor->tileItem.y));
			}
		}

		// TODO: special type for knives
		Weapon *gun = ActorGetGun(actor);
		if (!gun->Gun->CanShoot && actor->health > 0)
		{
			const TObject *object = target->kind == KIND_OBJECT ?
				CArrayGet(&gObjs, target->id) : NULL;
			if (!object || !ObjIsDangerous(object))
			{
				// Knife hit sound
				// Special case: only allow enemy slice sounds
				HitSounds knifeSounds;
				knifeSounds.Flesh = gun->soundLock <= 0 ?
					StrSound("knife_flesh") : NULL;
				knifeSounds.Object = gun->soundLock <= 0 ?
					StrSound("knife_hard") : NULL;
				knifeSounds.Wall = NULL;
				DamageSomething(
					Vec2iZero(),
					2,
					actor->flags,
					actor->playerIndex,
					actor->uid,
					target,
					SPECIAL_NONE,
					&knifeSounds,
					false);
				if (gun->soundLock <= 0)
				{
					gun->soundLock += gun->Gun->SoundLockLength;
				}
				return 0;
			}
		}

		realYPos = Vec2iFull2Real(Vec2iNew(actor->Pos.x, pos.y));
		if (CollideGetFirstItem(
			&actor->tileItem, realYPos, TILEITEM_IMPASSABLE,
			CalcCollisionTeam(1, actor),
			IsPVP(gCampaign.Entry.Mode)))
		{
			pos.y = actor->Pos.y;
		}
		realXPos = Vec2iFull2Real(Vec2iNew(pos.x, actor->Pos.y));
		if (CollideGetFirstItem(
			&actor->tileItem, realXPos, TILEITEM_IMPASSABLE,
			CalcCollisionTeam(1, actor),
			IsPVP(gCampaign.Entry.Mode)))
		{
			pos.x = actor->Pos.x;
		}
		if (pos.x != actor->Pos.x && pos.y != actor->Pos.y)
		{
			// Both x-only or y-only movement are viable,
			// i.e. we are colliding corner vs corner
			// Arbitrarily choose x-only movement
			pos.y = actor->Pos.y;
		}
		realPos = Vec2iFull2Real(pos);
		if ((pos.x == actor->Pos.x && pos.y == actor->Pos.y) ||
			IsCollisionWithWall(realPos, actor->tileItem.size))
		{
			return false;
		}
	}

	CheckTrigger(Vec2iToTile(realPos));

	CheckPickups(actor, realPos);

	GameEvent e = GameEventNew(GAME_EVENT_ACTOR_MOVE);
	e.u.ActorMove.UID = actor->uid;
	e.u.ActorMove.Pos.x = pos.x;
	e.u.ActorMove.Pos.y = pos.y;
	GameEventsEnqueue(&gGameEvents, e);

	return true;
}
// Get a movement position that is constrained by collisions
// May return a position that is the same as the 'from', that is, we cannot
// move in the direction specified.
// Note: must use full coordinates to do collisions, despite collisions using
// real coordinates, because fractional movement will be blocked otherwise
// since real coordinates are the same.
static Vec2i GetConstrainedFullPos(
	const Map *map, const Vec2i fromFull, const Vec2i toFull,
	const Vec2i size)
{
	// Check collision with wall
	if (!IsCollisionWithWall(Vec2iFull2Real(toFull), size))
	{
		// Not in collision; just return where we wanted to go
		return toFull;
	}
	
	CASSERT(size.x >= size.y, "tall collision not supported");
	const Vec2i dv = Vec2iMinus(toFull, fromFull);

	// If moving diagonally, use rectangular bounds and
	// try to move in only x or y directions
	if (dv.x != 0 && dv.y != 0)
	{
		// X-only movement
		const Vec2i xVec = Vec2iNew(toFull.x, fromFull.y);
		if (!IsCollisionWithWall(Vec2iFull2Real(xVec), size))
		{
			return xVec;
		}
		// Y-only movement
		const Vec2i yVec = Vec2iNew(fromFull.x, toFull.y);
		if (!IsCollisionWithWall(Vec2iFull2Real(yVec), size))
		{
			return yVec;
		}
		// If we're still stuck, we're possibly stuck on a corner which is not
		// in collision with a diamond but is colliding with the box.
		// If so try x- or y-only movement, but with the benefit of diamond
		// slipping.
		const Vec2i xPos = GetConstrainedFullPos(map, fromFull, xVec, size);
		if (!Vec2iEqual(xPos, fromFull))
		{
			return xPos;
		}
		const Vec2i yPos = GetConstrainedFullPos(map, fromFull, yVec, size);
		if (!Vec2iEqual(yPos, fromFull))
		{
			return yPos;
		}
	}

	// Now check diagonal movement, if we were moving in an x- or y-
	// only direction
	// Note: we're moving at extra speed because dx/dy are only magnitude 1;
	// if we divide then we get 0 which ruins the logic
	if (dv.x == 0)
	{
		// Moving up or down; try moving to the left or right diagonally
		// Scale X movement because our diamond is wider than tall, so we
		// may need to scale the diamond wider.
		const int xScale =
			size.x > size.y ? (int)ceil((double)size.x / size.y) : 1;
		const Vec2i diag1Vec =
			Vec2iAdd(fromFull, Vec2iNew(-dv.y * xScale, dv.y));
		if (!IsCollisionDiamond(map, Vec2iFull2Real(diag1Vec), size))
		{
			return diag1Vec;
		}
		const Vec2i diag2Vec =
			Vec2iAdd(fromFull, Vec2iNew(dv.y * xScale, dv.y));
		if (!IsCollisionDiamond(map, Vec2iFull2Real(diag2Vec), size))
		{
			return diag2Vec;
		}
	}
	else if (dv.y == 0)
	{
		// Moving left or right; try moving up or down diagonally
		const Vec2i diag1Vec =
			Vec2iAdd(fromFull, Vec2iNew(dv.x, -dv.x));
		if (!IsCollisionDiamond(map, Vec2iFull2Real(diag1Vec), size))
		{
			return diag1Vec;
		}
		const Vec2i diag2Vec =
			Vec2iAdd(fromFull, Vec2iNew(dv.x, dv.x));
		if (!IsCollisionDiamond(map, Vec2iFull2Real(diag2Vec), size))
		{
			return diag2Vec;
		}
	}

	// All alternative movements are in collision; don't move
	return fromFull;
}
// Check if the player can pickup any item
static bool CheckPickupFunc(TTileItem *ti, void *data);
static void CheckPickups(TActor *actor, const Vec2i realPos)
{
	// NPCs can't pickup
	if (actor->playerIndex < 0)
	{
		return;
	}
	CollideTileItems(
		&actor->tileItem, realPos, 0, CalcCollisionTeam(true, actor),
		IsPVP(gCampaign.Entry.Mode), CheckPickupFunc, actor);
}
static bool CheckPickupFunc(TTileItem *ti, void *data)
{
	TActor *actor = data;
	if (ti->kind == KIND_PICKUP)
	{
		PickupPickup(actor, CArrayGet(&gPickups, ti->id));
	}
	return true;
}

void ActorHeal(TActor *actor, int health)
{
	actor->health += health;
	actor->health = MIN(actor->health, ModeMaxHealth(gCampaign.Entry.Mode));
}

void InjureActor(TActor * actor, int injury)
{
	const int lastHealth = actor->health;
	actor->health -= injury;
	if (lastHealth > 0 && actor->health <= 0)
	{
		actor->stateCounter = 0;
		Vec2i pos = Vec2iNew(actor->tileItem.x, actor->tileItem.y);
		GameEvent sound = GameEventNew(GAME_EVENT_SOUND_AT);
		sound.u.SoundAt.Sound = SoundGetRandomScream(&gSoundDevice);
		sound.u.SoundAt.Pos = pos;
		GameEventsEnqueue(&gGameEvents, sound);
		if (actor->playerIndex >= 0)
		{
			SoundPlayAt(
				&gSoundDevice,
				StrSound("hahaha"),
				pos);
		}
		UpdateMissionObjective(
			&gMission, actor->tileItem.flags, OBJECTIVE_KILL,
			-1, pos);
	}
}

void ActorAddAmmo(TActor *actor, AddAmmo a)
{
	int *ammo = CArrayGet(&actor->ammo, a.Id);
	*ammo += a.Amount;
	const int ammoMax = AmmoGetById(&gAmmo, a.Id)->Max;
	*ammo = CLAMP(*ammo, 0, ammoMax);
}

static bool ActorHasGun(const TActor *a, const GunDescription *gun);
void ActorReplaceGun(
	TActor *actor, const int gunIdx, const GunDescription *gun)
{
	CASSERT(gunIdx >= 0 && gunIdx < MAX_WEAPONS, "invalid gun idx");
	// If player already has gun, don't do anything
	if (ActorHasGun(actor, gun))
	{
		return;
	}
	Weapon w = WeaponCreate(gun);
	if ((int)actor->guns.size <= gunIdx)
	{
		CASSERT(gunIdx < (int)actor->guns.size + 1, "gun idx would leave gap");
		CArrayPushBack(&actor->guns, &w);
	}
	else
	{
		memcpy(CArrayGet(&actor->guns, gunIdx), &w, actor->guns.elemSize);
	}
}
static bool ActorHasGun(const TActor *a, const GunDescription *gun)
{
	for (int i = 0; i < (int)a->guns.size; i++)
	{
		const Weapon *w = CArrayGet(&a->guns, i);
		if (w->Gun == gun)
		{
			return true;
		}
	}
	return false;
}

// Set AI state and possibly say something based on the state
void ActorSetAIState(TActor *actor, const AIState s)
{
	if (AIContextSetState(actor->aiContext, s) &&
		AIContextShowChatter(
		actor->aiContext, ConfigGetEnum(&gConfig, "Interface.AIChatter")))
	{
		// Say something for a while
		actor->Chatter = AIStateGetChatterText(actor->aiContext->State);
		actor->ChatterCounter = 2;
	}
}

void Shoot(TActor *actor)
{
	Weapon *gun = ActorGetGun(actor);
	if (!ActorCanFire(actor))
	{
		if (!WeaponIsLocked(gun) && ConfigGetBool(&gConfig, "Game.Ammo"))
		{
			CASSERT(ActorGunGetAmmo(actor, gun) == 0, "should be out of ammo");
			// Play a clicking sound if this gun is out of ammo
			if (gun->clickLock <= 0)
			{
				GameEvent e = GameEventNew(GAME_EVENT_SOUND_AT);
				e.u.SoundAt.Pos = Vec2iFull2Real(actor->Pos);
				e.u.SoundAt.Sound = gSoundDevice.clickSound;
				GameEventsEnqueue(&gGameEvents, e);
				gun->clickLock = SOUND_LOCK_WEAPON_CLICK;
			}
		}
		return;
	}
	WeaponFire(
		gun,
		actor->direction,
		actor->Pos,
		actor->flags,
		actor->playerIndex,
		actor->uid);
	if (actor->playerIndex >= 0)
	{
		if (ConfigGetBool(&gConfig, "Game.Ammo") && gun->Gun->AmmoId >= 0)
		{
			GameEvent e = GameEventNew(GAME_EVENT_USE_AMMO);
			e.u.UseAmmo.PlayerIndex = actor->playerIndex;
			e.u.UseAmmo.UseAmmo.Id = gun->Gun->AmmoId;
			e.u.UseAmmo.UseAmmo.Amount = -1;
			GameEventsEnqueue(&gGameEvents, e);
		}
		else if (gun->Gun->Cost != 0)
		{
			// Classic C-Dogs score consumption
			GameEvent e = GameEventNew(GAME_EVENT_SCORE);
			e.u.Score.PlayerIndex = actor->playerIndex;
			e.u.Score.Score = -gun->Gun->Cost;
			GameEventsEnqueue(&gGameEvents, e);
		}
	}
}

static bool ActorTryChangeDirection(
	TActor *actor, const int cmd, const int prevCmd)
{
	const bool willChangeDirecton =
		!actor->petrified &&
		CMD_HAS_DIRECTION(cmd) &&
		(!(cmd & CMD_BUTTON2) || ConfigGetEnum(&gConfig, "Game.SwitchMoveStyle") != SWITCHMOVE_STRAFE) &&
		(!(prevCmd & CMD_BUTTON1) || ConfigGetEnum(&gConfig, "Game.FireMoveStyle") != FIREMOVE_STRAFE);
	if (willChangeDirecton)
	{
		actor->direction = CmdToDirection(cmd);
	}
	return willChangeDirecton;
}

int ActorTryShoot(TActor *actor, int cmd)
{
	int willShoot = !actor->petrified && (cmd & CMD_BUTTON1);
	if (willShoot)
	{
		Shoot(actor);
	}
	else
	{
		WeaponHoldFire(ActorGetGun(actor));
	}
	return willShoot;
}

static bool ActorTryMove(TActor *actor, int cmd, int hasShot, int ticks);
void CommandActor(TActor * actor, int cmd, int ticks)
{
	actor->MovePos = actor->Pos;

	if (actor->confused)
	{
		cmd = CmdGetReverse(cmd);
	}

	if (actor->health > 0)
	{
		int hasChangedDirection, hasShot, hasMoved;
		hasChangedDirection = ActorTryChangeDirection(actor, cmd, actor->lastCmd);
		hasShot = ActorTryShoot(actor, cmd);
		hasMoved = ActorTryMove(actor, cmd, hasShot, ticks);
		if (!hasChangedDirection && !hasShot && !hasMoved)
		{
			// Idle if player hasn't done anything
			if (actor->state != STATE_IDLE &&
				actor->state != STATE_IDLELEFT &&
				actor->state != STATE_IDLERIGHT)
			{
				SetStateForActor(actor, STATE_IDLE);
			}
		}
	}

	actor->lastCmd = cmd;
	if (cmd & CMD_BUTTON2)
	{
		if (CMD_HAS_DIRECTION(cmd))
		{
			actor->specialCmdDir = true;
		}
		else if (actor->CanPickupSpecial)
		{
			actor->PickupAll = true;
			// Special: check pickups that can only be picked up on demand
			CheckPickups(actor, Vec2iFull2Real(actor->Pos));
			actor->PickupAll = false;
			// Cancel the last cmd having switch
			actor->lastCmd &= ~CMD_BUTTON2;
		}
	}
	else
	{
		actor->specialCmdDir = false;
	}
}
static bool ActorTryMove(TActor *actor, int cmd, int hasShot, int ticks)
{
	const bool canMoveWhenShooting =
		ConfigGetEnum(&gConfig, "Game.FireMoveStyle") != FIREMOVE_STOP ||
		!hasShot ||
		(ConfigGetEnum(&gConfig, "Game.SwitchMoveStyle") == SWITCHMOVE_STRAFE &&
		(cmd & CMD_BUTTON2));
	const bool willMove =
		!actor->petrified && CMD_HAS_DIRECTION(cmd) && canMoveWhenShooting;
	if (willMove)
	{
		int moveAmount = ActorGetCharacter(actor)->speed * ticks;
		if (cmd & CMD_LEFT)
		{
			actor->MovePos.x -= moveAmount;
		}
		else if (cmd & CMD_RIGHT)
		{
			actor->MovePos.x += moveAmount;
		}
		if (cmd & CMD_UP)
		{
			actor->MovePos.y -= moveAmount;
		}
		else if (cmd & CMD_DOWN)
		{
			actor->MovePos.y += moveAmount;
		}

		if (actor->state != STATE_WALKING_1 &&
			actor->state != STATE_WALKING_2 &&
			actor->state != STATE_WALKING_3 &&
			actor->state != STATE_WALKING_4)
		{
			SetStateForActor(actor, STATE_WALKING_1);
		}
	}
	else
	{
		if (actor->state == STATE_WALKING_1 ||
			actor->state == STATE_WALKING_2 ||
			actor->state == STATE_WALKING_3 ||
			actor->state == STATE_WALKING_4)
		{
			SetStateForActor(actor, STATE_IDLE);
		}
	}
	return willMove;
}

void SlideActor(TActor *actor, int cmd)
{
	// Check that actor can slide
	if (actor->slideLock > 0)
	{
		return;
	}

	if (actor->petrified)
		return;

	if (actor->confused)
	{
		cmd = CmdGetReverse(cmd);
	}

	actor->Vel = Vec2iZero();
	if (cmd & CMD_LEFT)			actor->Vel.x = -SLIDE_X * 256;
	else if (cmd & CMD_RIGHT)	actor->Vel.x = SLIDE_X * 256;
	if (cmd & CMD_UP)			actor->Vel.y = -SLIDE_Y * 256;
	else if (cmd & CMD_DOWN)	actor->Vel.y = SLIDE_Y * 256;

	// Slide sound
	if (ConfigGetBool(&gConfig, "Sound.Footsteps"))
	{
		SoundPlayAt(
			&gSoundDevice,
			gSoundDevice.slideSound,
			Vec2iNew(actor->tileItem.x, actor->tileItem.y));
	}
	
	actor->slideLock = SLIDE_LOCK;
}

static void ActorUpdatePosition(TActor *actor, int ticks);
static void ActorDie(TActor *actor, const int idx);
void UpdateAllActors(int ticks)
{
	for (int i = 0; i < (int)gActors.size; i++)
	{
		TActor *actor = CArrayGet(&gActors, i);
		if (!actor->isInUse)
		{
			continue;
		}
		ActorUpdatePosition(actor, ticks);
		UpdateActorState(actor, ticks);
		if (actor->dead > DEATH_MAX)
		{
			ActorDie(actor, i);
			continue;
		}
		// Find actors that are on the same team and colliding,
		// and repel them
		if (gCollisionSystem.allyCollision == ALLYCOLLISION_REPEL)
		{
			Vec2i realPos = Vec2iFull2Real(actor->Pos);
			TTileItem *collidingItem = CollideGetFirstItem(
				&actor->tileItem, realPos, TILEITEM_IMPASSABLE,
				COLLISIONTEAM_NONE,
				IsPVP(gCampaign.Entry.Mode));
			if (collidingItem && collidingItem->kind == KIND_CHARACTER)
			{
				TActor *collidingActor = CArrayGet(
					&gActors, collidingItem->id);
				if (CalcCollisionTeam(1, collidingActor) ==
					CalcCollisionTeam(1, actor))
				{
					Vec2i v = Vec2iMinus(actor->Pos, collidingActor->Pos);
					if (Vec2iIsZero(v))
					{
						v = Vec2iNew(1, 0);
					}
					v = Vec2iScale(Vec2iNorm(v), REPEL_STRENGTH);
					GameEvent e = GameEventNew(GAME_EVENT_ACTOR_IMPULSE);
					e.u.ActorImpulse.Id = actor->tileItem.id;
					e.u.ActorImpulse.Vel = v;
					GameEventsEnqueue(&gGameEvents, e);
					e.u.ActorImpulse.Id = collidingActor->tileItem.id;
					e.u.ActorImpulse.Vel = Vec2iScale(v, -1);
					GameEventsEnqueue(&gGameEvents, e);
				}
			}
		}
	}
}
static void ActorUpdatePosition(TActor *actor, int ticks)
{
	if (!Vec2iIsZero(actor->Vel))
	{
		actor->MovePos = Vec2iAdd(
			actor->MovePos, Vec2iScale(actor->Vel, ticks));

		for (int i = 0; i < ticks; i++)
		{
			if (actor->Vel.x > 0)
			{
				actor->Vel.x = MAX(0, actor->Vel.x - VEL_DECAY_X);
			}
			else
			{
				actor->Vel.x = MIN(0, actor->Vel.x + VEL_DECAY_X);
			}
			if (actor->Vel.y > 0)
			{
				actor->Vel.y = MAX(0, actor->Vel.y - VEL_DECAY_Y);
			}
			else
			{
				actor->Vel.y = MIN(0, actor->Vel.y + VEL_DECAY_Y);
			}
		}
	}

	if (!Vec2iEqual(actor->Pos, actor->MovePos))
	{
		TryMoveActor(actor, actor->MovePos);
		actor->MovePos = Vec2iZero();
	}
}
static void ActorAddAmmoPickup(const TActor *actor);
static void ActorDie(TActor *actor, const int idx)
{
	// Check if the player has lives to revive
	if (actor->playerIndex >= 0)
	{
		PlayerData *p = CArrayGet(&gPlayerDatas, actor->playerIndex);
		p->Lives--;
		CASSERT(p->Lives >= 0, "Player has died too many times");
		if (p->Lives > 0)
		{
			// Find the first player alive; try to spawn next to that position
			// if no other suitable position exists
			Vec2i defaultSpawnPosition = Vec2iZero();
			for (int i = 0; i < (int)gPlayerDatas.size; i++)
			{
				const PlayerData *pOther = CArrayGet(&gPlayerDatas, i);
				if (IsPlayerAlive(pOther))
				{
					const TActor *a = CArrayGet(&gActors, pOther->Id);
					defaultSpawnPosition = a->Pos;
					break;
				}
			}
			const Vec2i spawnPos =
				PlacePlayer(&gMap, p, defaultSpawnPosition, false);

			// Play a spawn sound for players
			GameEvent sound = GameEventNew(GAME_EVENT_SOUND_AT);
			// Need to delay it a bit because the camera takes time to update
			sound.Delay = 1;
			sound.u.SoundAt.Sound = StrSound("spawn");
			sound.u.SoundAt.Pos = Vec2iFull2Real(spawnPos);
			GameEventsEnqueue(&gGameEvents, sound);
		}
	}

	// Add an ammo pickup of the actor's gun
	if (ConfigGetBool(&gConfig, "Game.Ammo"))
	{
		ActorAddAmmoPickup(actor);
	}

	// Add a blood pool
	ObjAdd(RandomBloodMapObject(&gMapObjects), Vec2iFull2Real(actor->Pos), 0);

	ActorDestroy(idx);
}
static void ActorAddAmmoPickup(const TActor *actor)
{
	// Note: if the actor is AI with no shooting time,
	// then it's an unarmed actor
	if (actor->Character->bot != NULL &&
		actor->Character->bot->probabilityToShoot == 0)
	{
		return;
	}

	// Add ammo pickups for each of the actor's guns
	for (int i = 0; i < (int)actor->guns.size; i++)
	{
		const Weapon *w = CArrayGet(&actor->guns, i);

		// Check if the actor's gun has ammo at all
		if (w->Gun->AmmoId < 0)
		{
			continue;
		}

		// Don't spawn ammo if no players use it
		if (PlayersNumUseAmmo(w->Gun->AmmoId) == 0)
		{
			continue;
		}

		GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
		// Add a little random offset so the pickups aren't all together
		const Vec2i offset = Vec2iNew(
			RAND_INT(-TILE_WIDTH, TILE_WIDTH) / 2,
			RAND_INT(-TILE_HEIGHT, TILE_HEIGHT) / 2);
		e.u.AddPickup.Pos = Vec2iAdd(Vec2iFull2Real(actor->Pos), offset);
		const Ammo *a = AmmoGetById(&gAmmo, w->Gun->AmmoId);
		char buf[256];
		sprintf(buf, "ammo_%s", a->Name);
		e.u.AddPickup.PickupClassId = StrPickupClassId(buf);
		GameEventsEnqueue(&gGameEvents, e);
	}

}

void ActorsInit(void)
{
	CArrayInit(&gActors, sizeof(TActor));
	CArrayReserve(&gActors, 64);
}
void ActorsTerminate(void)
{
	for (int i = 0; i < (int)gActors.size; i++)
	{
		TActor *actor = CArrayGet(&gActors, i);
		if (actor->isInUse)
		{
			ActorDestroy(i);
		}
	}
	CArrayTerminate(&gActors);
}
int ActorsGetNextUID(void)
{
	return sActorUIDs;
}
int ActorsGetFreeIndex(void)
{
	// Find an empty slot in actor list
	// actors.size if no slot found (i.e. add to end)
	for (int i = 0; i < (int)gActors.size; i++)
	{
		const TActor *a = CArrayGet(&gActors, i);
		if (!a->isInUse)
		{
			return i;
		}
	}
	return (int)gActors.size;
}
TActor *ActorAdd(NetMsgActorAdd aa)
{
	const int id = ActorsGetFreeIndex();
	while (id >= (int)gActors.size)
	{
		TActor a;
		memset(&a, 0, sizeof a);
		CArrayPushBack(&gActors, &a);
	}
	TActor *actor = CArrayGet(&gActors, id);
	memset(actor, 0, sizeof *actor);
	actor->uid = aa.UID;
	while (aa.UID >= sActorUIDs)
	{
		sActorUIDs++;
	}
	CArrayInit(&actor->guns, sizeof(Weapon));
	CArrayInit(&actor->ammo, sizeof(int));
	for (int i = 0; i < AmmoGetNumClasses(&gAmmo); i++)
	{
		// Initialise with twice the standard ammo amount
		// TODO: special game modes, keeping track of ammo, ammo persistence
		int amount = AmmoGetById(&gAmmo, i)->Amount * 2;
		CArrayPushBack(&actor->ammo, &amount);
	}
	Character *c;
	if (aa.PlayerId >= 0)
	{
		// Add all player weapons
		PlayerData *p = CArrayGet(&gPlayerDatas, aa.PlayerId);
		c = &p->Char;
		for (int i = 0; i < p->weaponCount; i++)
		{
			Weapon gun = WeaponCreate(p->weapons[i]);
			CArrayPushBack(&actor->guns, &gun);
		}
		p->Id = id;
	}
	else
	{
		c = CArrayGet(&gCampaign.Setting.characters.OtherChars, aa.CharId);
		// Add sole weapon from character type
		Weapon gun = WeaponCreate(c->Gun);
		CArrayPushBack(&actor->guns, &gun);
	}
	actor->gunIndex = 0;
	actor->health = aa.Health;
	actor->action = ACTORACTION_MOVING;
	actor->tileItem.x = actor->tileItem.y = -1;
	actor->tileItem.kind = KIND_CHARACTER;
	actor->tileItem.getPicFunc = NULL;
	actor->tileItem.getActorPicsFunc = GetCharacterPics;
	actor->tileItem.drawFunc = NULL;
	actor->tileItem.size = Vec2iNew(ACTOR_W, ACTOR_H);
	actor->tileItem.flags =
		TILEITEM_IMPASSABLE | TILEITEM_CAN_BE_SHOT | aa.TileItemFlags;
	actor->tileItem.id = id;
	actor->isInUse = true;
	actor->flags = FLAGS_SLEEPING | c->flags;
	if (actor->flags & FLAGS_AWAKEALWAYS)
	{
		actor->flags &= ~FLAGS_SLEEPING;
	}
	actor->Character = c;
	actor->playerIndex = aa.PlayerId;
	actor->uid = sActorUIDs++;
	actor->direction = DIRECTION_DOWN;
	actor->state = STATE_IDLE;
	actor->slideLock = 0;
	if (c->bot)
	{
		actor->aiContext = AIContextNew();
		ActorSetAIState(actor, AI_STATE_IDLE);
	}
	TryMoveActor(actor, Vec2iNew(aa.FullPos.x, aa.FullPos.y));
	return actor;
}
void ActorDestroy(int id)
{
	TActor *actor = CArrayGet(&gActors, id);
	CASSERT(actor->isInUse, "Destroying in-use actor");
	CArrayTerminate(&actor->guns);
	CArrayTerminate(&actor->ammo);
	MapRemoveTileItem(&gMap, &actor->tileItem);
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (id == p->Id)
		{
			p->Id = -1;
			break;
		}
	}
	AIContextDestroy(actor->aiContext);
	actor->isInUse = false;
}

unsigned char BestMatch(const TPalette palette, int r, int g, int b)
{
	int d, dMin = 0;
	int i;
	int best = -1;

	for (i = 0; i < 256; i++)
	{
		d = (r - palette[i].r) * (r - palette[i].r) +
			(g - palette[i].g) * (g - palette[i].g) +
			(b - palette[i].b) * (b - palette[i].b);
		if (best < 0 || d < dMin)
		{
			best = i;
			dMin = d;
		}
	}
	return (unsigned char)best;
}

void BuildTranslationTables(const TPalette palette)
{
	int i;
	unsigned char f;

	for (i = 0; i < 256; i++)
	{
		f = (unsigned char)floor(
			0.3 * palette[i].r +
			0.59 * palette[i].g +
			0.11 * palette[i].b);
		tableFlamed[i] = BestMatch(palette, f, 0, 0);
	}
	for (i = 0; i < 256; i++)
	{
		f = (unsigned char)floor(
			0.4 * palette[i].r +
			0.49 * palette[i].g +
			0.11 * palette[i].b);
		tableGreen[i] = BestMatch(palette, 0, 2 * f / 3, 0);
	}
	for (i = 0; i < 256; i++)
	{
		tablePoison[i] = BestMatch(
			palette,
			palette[i].r + 5,
			palette[i].g + 15,
			palette[i].b + 5);
	}
	for (i = 0; i < 256; i++)
	{
		f = (unsigned char)floor(
			0.4 * palette[i].r +
			0.49 * palette[i].g +
			0.11 * palette[i].b);
		tableGray[i] = BestMatch(palette, f, f, f);
	}
	for (i = 0; i < 256; i++)
	{
		tableBlack[i] = BestMatch(palette, 0, 0, 0);
	}
	for (i = 0; i < 256; i++)
	{
		f = (unsigned char)floor(
			0.4 * palette[i].r +
			0.49 * palette[i].g +
			0.11 * palette[i].b);
		tablePurple[i] = BestMatch(palette, f, 0, f);
	}
	for (i = 0; i < 256; i++)
	{
		tableDarker[i] = BestMatch(
			palette,
			(200 * palette[i].r) / 256,
			(200 * palette[i].g) / 256,
			(200 * palette[i].b) / 256);
	}
}

TActor *ActorGetByUID(const int uid)
{
	for (int i = 0; i < (int)gActors.size; i++)
	{
		TActor *a = CArrayGet(&gActors, i);
		if (a->uid == uid)
		{
			return a;
		}
	}
	return NULL;
}

const Character *ActorGetCharacter(const TActor *a)
{
	if (a->playerIndex >= 0)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, a->playerIndex);
		return &p->Char;
	}
	return a->Character;
}

Weapon *ActorGetGun(const TActor *a)
{
	return CArrayGet(&a->guns, a->gunIndex);
}
int ActorGunGetAmmo(const TActor *a, const Weapon *w)
{
	if (w->Gun->AmmoId == -1)
	{
		return -1;
	}
	return *(int *)CArrayGet(&a->ammo, w->Gun->AmmoId);
}
bool ActorCanFire(const TActor *a)
{
	const Weapon *w = ActorGetGun(a);
	const bool hasAmmo = ActorGunGetAmmo(a, w) != 0;
	return
		!WeaponIsLocked(w) &&
		(!ConfigGetBool(&gConfig, "Game.Ammo") || hasAmmo);
}
bool ActorTrySwitchGun(TActor *a)
{
	if (a->guns.size < 2)
	{
		return false;
	}
	a->gunIndex++;
	if (a->gunIndex >= (int)a->guns.size)
	{
		a->gunIndex = 0;
	}
	return true;
}

bool ActorIsImmune(const TActor *actor, const special_damage_e damage)
{
	// Fire immunity
	if (damage == SPECIAL_FLAME && (actor->flags & FLAGS_ASBESTOS))
	{
		return 1;
	}
	// Poison immunity
	if (damage == SPECIAL_POISON && (actor->flags & FLAGS_IMMUNITY))
	{
		return 1;
	}
	// Confuse immunity
	if (damage == SPECIAL_CONFUSE && (actor->flags & FLAGS_IMMUNITY))
	{
		return 1;
	}
	// Don't bother if health already 0 or less
	if (actor->health <= 0)
	{
		return 1;
	}
	return 0;
}


// Special damage durations
#define FLAMED_COUNT        10
#define POISONED_COUNT       8
#define MAX_POISONED_COUNT 140
#define PETRIFIED_COUNT     95
#define CONFUSED_COUNT     700

void ActorTakeSpecialDamage(TActor *actor, special_damage_e damage)
{
	switch (damage)
	{
	case SPECIAL_FLAME:
		actor->flamed = FLAMED_COUNT;
		break;
	case SPECIAL_POISON:
		if (actor->poisoned < MAX_POISONED_COUNT)
		{
			actor->poisoned += POISONED_COUNT;
		}
		break;
	case SPECIAL_PETRIFY:
		if (!actor->petrified)
		{
			actor->petrified = PETRIFIED_COUNT;
		}
		break;
	case SPECIAL_CONFUSE:
		actor->confused = CONFUSED_COUNT;
		break;
	default:
		// do nothing
		break;
	}
}

void ActorTakeHit(TActor *actor, const special_damage_e damage)
{
	// Wake up if this is an AI
	if (actor->aiContext)
	{
		actor->flags &= ~FLAGS_SLEEPING;
		ActorSetAIState(actor, AI_STATE_NONE);
	}
	// Check immune again
	// This can happen if multiple damage events overkill this actor,
	// need to ignore the overkill scores
	if (ActorIsImmune(actor, damage))
	{
		return;
	}
	ActorTakeSpecialDamage(actor, damage);
}

bool ActorIsInvulnerable(
	const TActor *actor, const int flags, const int player,
	const GameMode mode)
{
	if (actor->flags & FLAGS_INVULNERABLE)
	{
		return 1;
	}

	if (!(flags & FLAGS_HURTALWAYS) && !(actor->flags & FLAGS_VICTIM))
	{
		// Same player hits
		if (player >= 0 && player == actor->playerIndex)
		{
			return 1;
		}
		const bool isGood = player >= 0 || (flags & FLAGS_GOOD_GUY);
		const bool isTargetGood =
			actor->playerIndex >= 0 || (actor->flags & FLAGS_GOOD_GUY);
		// Friendly fire (NPCs)
		if (!IsPVP(mode) &&
			!ConfigGetBool(&gConfig, "Game.FriendlyFire") &&
			isGood && isTargetGood)
		{
			return 1;
		}
		// Enemies don't hurt each other
		if (!isGood && !isTargetGood)
		{
			return 1;
		}
	}

	return 0;
}
