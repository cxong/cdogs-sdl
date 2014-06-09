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

    Copyright (c) 2013-2014, Cong Xu
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

#include "ai_utils.h"
#include "character.h"
#include "collision.h"
#include "config.h"
#include "drawtools.h"
#include "game_events.h"
#include "pic_manager.h"
#include "sounds.h"
#include "defs.h"
#include "objs.h"
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


int gPlayerIds[MAX_PLAYERS];

TranslationTable tableFlamed;
TranslationTable tableGreen;
TranslationTable tablePoison;
TranslationTable tableGray;
TranslationTable tableBlack;
TranslationTable tableDarker;
TranslationTable tablePurple;


CArray gActors;

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


// Initialise the actor post-placement
void ActorInit(TActor *actor)
{
	actor->direction = rand() % DIRECTION_COUNT;

	actor->health = (actor->health * gConfig.Game.NonPlayerHP) / 100;
	if (actor->health <= 0)
	{
		actor->health = 1;
	}
	if (actor->flags & FLAGS_AWAKEALWAYS)
	{
		actor->flags &= ~FLAGS_SLEEPING;
	}
}

int GetNumPlayersAlive(void)
{
	int numPlayers = 0;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (IsPlayerAlive(i))
		{
			numPlayers++;
		}
	}
	return numPlayers;
}

TActor *GetFirstAlivePlayer(void)
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (IsPlayerAlive(i))
		{
			return CArrayGet(&gActors, gPlayerIds[i]);
		}
	}
	return NULL;
}

bool IsPlayerAlive(int player)
{
	if (gPlayerIds[player] == -1)
	{
		return false;
	}
	TActor *p = CArrayGet(&gActors, gPlayerIds[player]);
	return !p->dead;
}

Vec2i PlayersGetMidpoint(void)
{
	// for all surviving players, find bounding rectangle, and get center
	Vec2i min;
	Vec2i max;
	PlayersGetBoundingRectangle(&min, &max);
	return Vec2iScaleDiv(Vec2iAdd(min, max), 2);
}

void PlayersGetBoundingRectangle(Vec2i *min, Vec2i *max)
{
	int isFirst = 1;
	*min = Vec2iZero();
	*max = Vec2iZero();
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (IsPlayerAlive(i))
		{
			TActor *player = CArrayGet(&gActors, gPlayerIds[i]);
			TTileItem *p = &player->tileItem;
			if (isFirst)
			{
				*min = *max = Vec2iNew(p->x, p->y);
			}
			else
			{
				if (p->x < min->x)	min->x = p->x;
				if (p->y < min->y)	min->y = p->y;
				if (p->x > max->x)	max->x = p->x;
				if (p->y > max->y)	max->y = p->y;
			}
			isFirst = 0;
		}
	}
}

static ActorPics GetCharacterPics(int id)
{
	ActorPics pics;
	memset(&pics, 0, sizeof pics);
	TActor *actor = CArrayGet(&gActors, id);
	direction_e dir = actor->direction;
	direction_e headDir = dir;
	int state = actor->state;
	int headState = state;

	Character *c = actor->character;
	pics.Table = (TranslationTable *)c->table;
	int f = c->looks.face;
	int g = actor->weapon.Gun->pic;
	gunstate_e gunState = actor->weapon.state;

	TOffsetPic body, head, gun;

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

	if (g >= 0) {
		gun.dx =
		    cGunHandOffset[b][dir].dx +
		    cGunPics[g][dir][gunState].dx;
		gun.dy =
		    cGunHandOffset[b][dir].dy +
		    cGunPics[g][dir][gunState].dy;
		gun.picIndex = cGunPics[g][dir][gunState].picIndex;
	} else
		gun.picIndex = -1;

	switch (dir)
	{
	case DIRECTION_UP:
	case DIRECTION_UPRIGHT:
		pics.Pics[0] = PicFromTOffsetPic(&gPicManager, gun);
		pics.Pics[1] = PicFromTOffsetPic(&gPicManager, head);
		pics.Pics[2] = PicFromTOffsetPic(&gPicManager, body);
		pics.OldPics[0] = gun.picIndex;
		pics.OldPics[1] = head.picIndex;
		pics.OldPics[2] = body.picIndex;
		break;

	case DIRECTION_RIGHT:
	case DIRECTION_DOWNRIGHT:
	case DIRECTION_DOWN:
	case DIRECTION_DOWNLEFT:
		pics.Pics[0] = PicFromTOffsetPic(&gPicManager, body);
		pics.Pics[1] = PicFromTOffsetPic(&gPicManager, head);
		pics.Pics[2] = PicFromTOffsetPic(&gPicManager, gun);
		pics.OldPics[0] = body.picIndex;
		pics.OldPics[1] = head.picIndex;
		pics.OldPics[2] = gun.picIndex;
		break;

	case DIRECTION_LEFT:
	case DIRECTION_UPLEFT:
		pics.Pics[0] = PicFromTOffsetPic(&gPicManager, gun);
		pics.Pics[1] = PicFromTOffsetPic(&gPicManager, body);
		pics.Pics[2] = PicFromTOffsetPic(&gPicManager, head);
		pics.OldPics[0] = gun.picIndex;
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
	WeaponUpdate(
		&actor->weapon,
		ticks,
		Vec2iNew(actor->tileItem.x, actor->tileItem.y));

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
	if (gConfig.Sound.Footsteps &&
		(actor->state == STATE_WALKING_1 ||
		actor->state == STATE_WALKING_2 ||
		actor->state == STATE_WALKING_3 ||
		actor->state == STATE_WALKING_4) &&
		actor->soundLock <= 0)
	{
		SoundPlayAtPlusDistance(
			&gSoundDevice,
			SND_FOOTSTEP,
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
			GameEvent e;
			e.Type = GAME_EVENT_TRIGGER;
			e.u.Trigger.Id = (*tp)->id;
			e.u.Trigger.TilePos = tilePos;
			GameEventsEnqueue(&gGameEvents, e);
		}
	}
}

static void PickupObject(TActor * actor, TObject * object)
{
	bool isKey = false;
	bool canPickup = true;
	switch (object->Type)
	{
	case OBJ_JEWEL:
		{
			GameEvent e;
			e.Type = GAME_EVENT_SCORE;
			e.u.Score.PlayerIndex = actor->pData->playerIndex;
			e.u.Score.Score = PICKUP_SCORE;
			GameEventsEnqueue(&gGameEvents, e);
			SoundPlayAt(
				&gSoundDevice,
				SND_PICKUP,
				Vec2iNew(actor->tileItem.x, actor->tileItem.y));
		}
		break;

	case OBJ_HEALTH:
		// Don't pick up unless taken damage
		canPickup = false;
		if (actor->health < actor->character->maxHealth)
		{
			canPickup = true;
			GameEvent e;
			e.Type = GAME_EVENT_TAKE_HEALTH_PICKUP;
			e.u.PickupPlayer = actor->pData->playerIndex;
			GameEventsEnqueue(&gGameEvents, e);
			SoundPlayAt(
				&gSoundDevice, SND_HEALTH,
				Vec2iNew(actor->tileItem.x, actor->tileItem.y));
		}
		break;

	case OBJ_KEYCARD_RED:
		gMission.flags |= FLAGS_KEYCARD_RED;
		isKey = true;
		break;
	case OBJ_KEYCARD_BLUE:
		gMission.flags |= FLAGS_KEYCARD_BLUE;
		isKey = true;
		break;
	case OBJ_KEYCARD_GREEN:
		gMission.flags |= FLAGS_KEYCARD_GREEN;
		isKey = true;
		break;
	case OBJ_KEYCARD_YELLOW:
		gMission.flags |= FLAGS_KEYCARD_YELLOW;
		isKey = true;
		break;
	default:
		CASSERT(false, "unexpected objective");
		break;
	}
	if (isKey)
	{
		SoundPlayAt(
			&gSoundDevice, SND_KEY,
			Vec2iNew(actor->tileItem.x, actor->tileItem.y));
	}
	UpdateMissionObjective(
		&gMission, object->tileItem.flags, OBJECTIVE_COLLECT,
		actor->pData->playerIndex,
		Vec2iNew(actor->tileItem.x, actor->tileItem.y));
	if (canPickup)
	{
		ObjDestroy(object->tileItem.id);
	}
}

bool TryMoveActor(TActor *actor, Vec2i pos)
{
	TTileItem *target;
	TObject *object;
	Vec2i realPos = Vec2iFull2Real(pos);
	Vec2i size = Vec2iNew(actor->tileItem.w, actor->tileItem.h);
	bool isDogfight = gCampaign.Entry.Mode == CAMPAIGN_MODE_DOGFIGHT;

	// Check collision with wall; try to limit x and y movement if still in
	// collision in those axes
	if (IsCollisionWallOrEdge(&gMap, realPos, size))
	{
		Vec2i realXPos, realYPos;
		realYPos = Vec2iFull2Real(Vec2iNew(actor->Pos.x, pos.y));
		if (IsCollisionWallOrEdge(&gMap, realYPos, size))
		{
			pos.y = actor->Pos.y;
		}
		realXPos = Vec2iFull2Real(Vec2iNew(pos.x, actor->Pos.y));
		if (IsCollisionWallOrEdge(&gMap, realXPos, size))
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
		else if (pos.x == actor->Pos.x && pos.y == actor->Pos.y)
		{
			return 0;
		}
	}

	realPos = Vec2iFull2Real(pos);
	target = GetItemOnTileInCollision(
		&actor->tileItem, realPos, TILEITEM_IMPASSABLE,
		CalcCollisionTeam(1, actor),
		isDogfight);
	if (target)
	{
		Vec2i realXPos, realYPos;

		if (actor->pData && target->kind == KIND_CHARACTER)
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
					actor->pData->playerIndex,
					Vec2iNew(actor->tileItem.x, actor->tileItem.y));
			}
		}

		// TODO: special type for knives
		if (!actor->weapon.Gun->CanShoot && actor->health > 0)
		{
			object = target->kind == KIND_OBJECT ?
				CArrayGet(&gObjs, target->id) : NULL;
			if (!object || (object->flags & OBJFLAG_DANGEROUS) == 0)
			{
				DamageSomething(
					Vec2iZero(),
					2,
					actor->flags,
					actor->pData ? actor->pData->playerIndex : -1,
					target,
					SPECIAL_KNIFE,
					actor->weapon.soundLock <= 0);
				if (actor->weapon.soundLock <= 0)
				{
					actor->weapon.soundLock +=
						actor->weapon.Gun->SoundLockLength;
				}
				return 0;
			}
		}

		realYPos = Vec2iFull2Real(Vec2iNew(actor->Pos.x, pos.y));
		if (GetItemOnTileInCollision(
			&actor->tileItem, realYPos, TILEITEM_IMPASSABLE,
			CalcCollisionTeam(1, actor),
			isDogfight))
		{
			pos.y = actor->Pos.y;
		}
		realXPos = Vec2iFull2Real(Vec2iNew(pos.x, actor->Pos.y));
		if (GetItemOnTileInCollision(
			&actor->tileItem, realXPos, TILEITEM_IMPASSABLE,
			CalcCollisionTeam(1, actor),
			isDogfight))
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
			IsCollisionWallOrEdge(&gMap, realPos, size))
		{
			return 0;
		}
	}

	CheckTrigger(Vec2iToTile(realPos));

	if (actor->pData)
	{
		target = GetItemOnTileInCollision(
			&actor->tileItem, realPos, TILEITEM_CAN_BE_TAKEN,
			CalcCollisionTeam(1, actor),
			isDogfight);
		if (target && target->kind == KIND_OBJECT)
		{
			PickupObject(actor, CArrayGet(&gObjs, target->id));
		}
	}

	actor->Pos = pos;
	MapMoveTileItem(&gMap, &actor->tileItem, Vec2iFull2Real(actor->Pos));

	if (MapIsTileInExit(&gMap, &actor->tileItem))
	{
		actor->action = ACTORACTION_EXITING;
	}
	else
	{
		actor->action = ACTORACTION_MOVING;
	}

	return 1;
}

void PlayRandomScreamAt(Vec2i pos)
{
	#define SCREAM_COUNT 4
	static sound_e screamTable[SCREAM_COUNT] =
		{ SND_KILL, SND_KILL2, SND_KILL3, SND_KILL4 };
	static int screamIndex = 0;
	SoundPlayAt(&gSoundDevice, screamTable[screamIndex], pos);
	screamIndex++;
	if (screamIndex >= SCREAM_COUNT)
	{
		screamIndex = 0;
	}
}

void ActorHeal(TActor *actor, int health)
{
	actor->health += health;
	actor->health = MIN(actor->health, 200 * gConfig.Game.PlayerHP / 100);
}

void InjureActor(TActor * actor, int injury)
{
	const int lastHealth = actor->health;
	actor->health -= injury;
	if (lastHealth > 0 && actor->health <= 0)
	{
		actor->stateCounter = 0;
		Vec2i pos = Vec2iNew(actor->tileItem.x, actor->tileItem.y);
		PlayRandomScreamAt(pos);
		if (actor->pData)
		{
			SoundPlayAt(
				&gSoundDevice,
				SND_HAHAHA,
				pos);
		}
		UpdateMissionObjective(
			&gMission, actor->tileItem.flags, OBJECTIVE_KILL,
			-1, pos);
	}
}

void Score(struct PlayerData *p, int points)
{
	if (p)
	{
		p->score += points;
		p->totalScore += points;
	}
}

void Shoot(TActor *actor)
{
	if (!WeaponCanFire(&actor->weapon))
	{
		return;
	}
	WeaponFire(
		&actor->weapon,
		actor->direction,
		actor->Pos,
		actor->flags,
		actor->pData ? actor->pData->playerIndex : -1);
	if (actor->pData && actor->weapon.Gun->Cost != 0)
	{
		GameEvent e;
		e.Type = GAME_EVENT_SCORE;
		e.u.Score.PlayerIndex = actor->pData->playerIndex;
		e.u.Score.Score = -actor->weapon.Gun->Cost;
		GameEventsEnqueue(&gGameEvents, e);
	}
}

int ActorTryChangeDirection(TActor *actor, int cmd)
{
	int willChangeDirecton =
		!actor->petrified &&
		(cmd & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN)) &&
		(!(cmd & CMD_BUTTON2) || gConfig.Game.SwitchMoveStyle != SWITCHMOVE_STRAFE);
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
		WeaponHoldFire(&actor->weapon);
	}
	return willShoot;
}

static bool ActorTryMove(TActor *actor, int cmd, int hasShot, int ticks);
void CommandActor(TActor * actor, int cmd, int ticks)
{
	actor->MovePos = actor->Pos;
	actor->lastCmd = cmd;

	if (actor->confused)
	{
		cmd = CmdGetReverse(cmd);
	}

	if (actor->health > 0)
	{
		int hasChangedDirection, hasShot, hasMoved;
		hasChangedDirection = ActorTryChangeDirection(actor, cmd);
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
}
static bool ActorTryMove(TActor *actor, int cmd, int hasShot, int ticks)
{
	int canMoveWhenShooting =
		gConfig.Game.MoveWhenShooting ||
		!hasShot ||
		(gConfig.Game.SwitchMoveStyle == SWITCHMOVE_STRAFE &&
		actor->flags & FLAGS_SPECIAL_USED);
	bool willMove =
		!actor->petrified &&
		(cmd & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN)) &&
		canMoveWhenShooting;
	if (willMove)
	{
		int moveAmount = actor->character->speed * ticks;
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
	if (gConfig.Sound.Footsteps)
	{
		SoundPlayAt(
			&gSoundDevice,
			SND_SLIDE,
			Vec2iNew(actor->tileItem.x, actor->tileItem.y));
	}
	
	actor->slideLock = SLIDE_LOCK;
}

static void ActorUpdatePosition(TActor *actor, int ticks);
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
			AddObjectOld(
				actor->Pos.x, actor->Pos.y,
				Vec2iZero(),
				&cBloodPics[rand() % BLOOD_MAX],
				OBJ_NONE,
				TILEITEM_IS_WRECK);
			ActorDestroy(i);
		}
		else
		{
			// Find actors that are on the same team and colliding,
			// and repel them
			if (gConfig.Game.AllyCollision == ALLYCOLLISION_REPEL)
			{
				Vec2i realPos = Vec2iFull2Real(actor->Pos);
				TTileItem *collidingItem = GetItemOnTileInCollision(
					&actor->tileItem, realPos, TILEITEM_IMPASSABLE,
					COLLISIONTEAM_NONE,
					gCampaign.Entry.Mode == CAMPAIGN_MODE_DOGFIGHT);
				if (collidingItem && collidingItem->kind == KIND_CHARACTER)
				{
					TActor *collidingActor = CArrayGet(
						&gActors, collidingItem->id);
					if (CalcCollisionTeam(1, collidingActor) ==
						CalcCollisionTeam(1, actor))
					{
						Vec2i v = Vec2iMinus(actor->Pos, collidingActor->Pos);
						if (Vec2iEqual(v, Vec2iZero()))
						{
							v = Vec2iNew(1, 0);
						}
						v = Vec2iScale(Vec2iNorm(v), REPEL_STRENGTH);
						GameEvent e;
						e.Type = GAME_EVENT_ACTOR_IMPULSE;
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
}
static void ActorUpdatePosition(TActor *actor, int ticks)
{
	if (!Vec2iEqual(actor->Vel, Vec2iZero()))
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

	if (!Vec2iEqual(actor->MovePos, Vec2iZero()))
	{
		TryMoveActor(actor, actor->MovePos);
		actor->MovePos = Vec2iZero();
	}
}

void ActorsInit(void)
{
	CArrayInit(&gActors, sizeof(TActor));
	CArrayReserve(&gActors, 64);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		gPlayerIds[i] = -1;
	}
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
int ActorAdd(Character *c, struct PlayerData *p)
{
	// Find an empty slot in actor list
	TActor *actor = NULL;
	int i;
	for (i = 0; i < (int)gActors.size; i++)
	{
		TActor *a = CArrayGet(&gActors, i);
		if (!a->isInUse)
		{
			actor = a;
			break;
		}
	}
	if (actor == NULL)
	{
		TActor a;
		memset(&a, 0, sizeof a);
		CArrayPushBack(&gActors, &a);
		i = (int)gActors.size - 1;
		actor = CArrayGet(&gActors, i);
	}
	memset(actor, 0, sizeof *actor);
	actor->weapon = WeaponCreate(c->Gun);
	actor->health = c->maxHealth;
	actor->action = ACTORACTION_MOVING;
	actor->tileItem.x = actor->tileItem.y = -1;
	actor->tileItem.kind = KIND_CHARACTER;
	actor->tileItem.getPicFunc = NULL;
	actor->tileItem.getActorPicsFunc = GetCharacterPics;
	actor->tileItem.drawFunc = NULL;
	actor->tileItem.w = ACTOR_W;
	actor->tileItem.h = ACTOR_H;
	actor->tileItem.flags = TILEITEM_IMPASSABLE | TILEITEM_CAN_BE_SHOT;
	actor->tileItem.id = i;
	actor->isInUse = true;
	actor->flags = FLAGS_SLEEPING | c->flags;
	actor->character = c;
	actor->pData = p;
	actor->direction = DIRECTION_DOWN;
	actor->state = STATE_IDLE;
	actor->slideLock = 0;
	if (c->bot)
	{
		actor->aiContext = AIContextNew();
		actor->aiContext->State = AI_STATE_IDLE;
	}
	return i;
}
void ActorDestroy(int id)
{
	TActor *actor = CArrayGet(&gActors, id);
	CASSERT(actor->isInUse, "Destroying in-use actor");
	MapRemoveTileItem(&gMap, &actor->tileItem);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (id == gPlayerIds[i])
		{
			gPlayerIds[i] = -1;
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

int ActorIsImmune(TActor *actor, special_damage_e damage)
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

void ActorTakeHit(
	TActor *actor,
	const special_damage_e damage,
	const bool isHitSoundEnabled,
	const bool isInvulnerable,
	const Vec2i hitLocation)
{
	// Wake up if this is an AI
	if (actor->aiContext)
	{
		actor->flags &= ~FLAGS_SLEEPING;
		actor->aiContext->State = AI_STATE_NONE;
	}
	// Check immune again
	// This can happen if multiple damage events overkill this actor,
	// need to ignore the overkill scores
	if (ActorIsImmune(actor, damage))
	{
		return;
	}
	ActorTakeSpecialDamage(actor, damage);

	// Hit sound
	if (isHitSoundEnabled)
	{
		sound_e sound = SoundGetHit(damage, 1);
		if (!isInvulnerable || sound != SND_KNIFE_FLESH)
		{
			SoundPlayAt(&gSoundDevice, sound, hitLocation);
		}
	}
}

int ActorIsInvulnerable(
	TActor *actor, int flags, int player, campaign_mode_e mode)
{
	if (actor->flags & FLAGS_INVULNERABLE)
	{
		return 1;
	}

	if (!(flags & FLAGS_HURTALWAYS) && !(actor->flags & FLAGS_VICTIM))
	{
		// Same player hits
		if (player >= 0 && actor->pData &&
			&gPlayerDatas[player] == actor->pData)
		{
			return 1;
		}
		// Friendly fire (NPCs)
		if (mode != CAMPAIGN_MODE_DOGFIGHT &&
			!gConfig.Game.FriendlyFire &&
			(player >= 0 || (flags & FLAGS_GOOD_GUY)) &&
			(actor->pData || (actor->flags & FLAGS_GOOD_GUY)))
		{
			return 1;
		}
		// Enemies don't hurt each other
		if (!(player >= 0 || (flags & FLAGS_GOOD_GUY)) &&
			!(actor->pData || (actor->flags & FLAGS_GOOD_GUY)))
		{
			return 1;
		}
	}

	return 0;
}
