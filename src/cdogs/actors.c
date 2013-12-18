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

    Copyright (c) 2013, Cong Xu
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

#include "character.h"
#include "collision.h"
#include "config.h"
#include "drawtools.h"
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
#define REPEL_STRENGTH 18
#define SLIDE_LOCK 50


TActor *gPlayers[MAX_PLAYERS];

TranslationTable tableFlamed;
TranslationTable tableGreen;
TranslationTable tablePoison;
TranslationTable tableGray;
TranslationTable tableBlack;
TranslationTable tableDarker;
TranslationTable tablePurple;


static TActor *actorList = NULL;


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


int GetNumPlayersAlive(void)
{
	int numPlayers = 0;
	int i;
	for (i = 0; i < MAX_PLAYERS; i++)
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
	int i;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (IsPlayerAlive(i))
		{
			return gPlayers[i];
		}
	}
	return NULL;
}

int IsPlayerAlive(int player)
{
	return gPlayers[player] && !gPlayers[player]->dead;
}

Vec2i PlayersGetMidpoint(TActor *players[MAX_PLAYERS])
{
	// for all surviving players, find bounding rectangle, and get center
	Vec2i min;
	Vec2i max;
	PlayersGetBoundingRectangle(players, &min, &max);
	return Vec2iNew((min.x + max.x) / 2, (min.y + max.y) / 2);
}

void PlayersGetBoundingRectangle(
	TActor *players[MAX_PLAYERS], Vec2i *min, Vec2i *max)
{
	int isFirst = 1;
	int i;
	*min = Vec2iZero();
	*max = Vec2iZero();
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (IsPlayerAlive(i))
		{
			TTileItem *p = &players[i]->tileItem;
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

void DrawCharacter(int x, int y, TActor * actor)
{
	direction_e dir = actor->direction;
	direction_e headDir = dir;
	int state = actor->state;
	int headState = state;

	Character *c = actor->character;
	TranslationTable *table = (TranslationTable *) c->table;
	HSV *tint = NULL;
	int f = c->looks.face;
	int b;
	int g = GunGetPic(actor->weapon.gun);
	gunstate_e gunState = actor->weapon.state;

	TOffsetPic body, head, gun;
	TOffsetPic pic1, pic2, pic3;

	int transparent = (actor->flags & FLAGS_SEETHROUGH) != 0;

	if (gunState == GUNSTATE_FIRING || gunState == GUNSTATE_RECOIL)
	{
		headState = STATE_COUNT + gunState - GUNSTATE_FIRING;
	}

	if (actor->flamed)
	{
		table = &tableFlamed;
		tint = &tintRed;
	}
	else if (actor->poisoned)
	{
		table = &tableGreen;
		tint = &tintPoison;
	}
	else if (actor->petrified)
	{
		table = &tableGray;
		tint = &tintGray;
	}
	else if (actor->confused)
	{
		table = &tablePurple;
		tint = &tintPurple;
	}
	else if (transparent)
	{
		table = &tableDarker;
		tint = &tintDarker;
	}

	if (actor->dead) {
		if (actor->dead <= DEATH_MAX) {
			body = cDeathPics[actor->dead - 1];
			if (transparent)
			{
				DrawBTPic(
					x + body.dx, y + body.dy,
					PicManagerGetOldPic(&gPicManager, body.picIndex), tint);
			}
			else
			{
				DrawTTPic(
					x + body.dx, y + body.dy,
					PicManagerGetOldPic(&gPicManager, body.picIndex), table);
			}
		}
		return;
	}

	actor->flags |= FLAGS_VISIBLE;
	// TODO: this means any character wakes up when visible
	actor->flags &= ~FLAGS_SLEEPING;

	if (state == STATE_IDLELEFT)
		headDir = (dir + 7) % 8;
	else if (state == STATE_IDLERIGHT)
		headDir = (dir + 1) % 8;

	if (g < 0)
	{
		b = c->looks.unarmedBody;
	}
	else
	{
		b = c->looks.armedBody;
	}

	body.dx = cBodyOffset[b][dir].dx;
	body.dy = cBodyOffset[b][dir].dy;
	body.picIndex = cBodyPic[b][dir][state];

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

	if (transparent)
	{
		if (pic1.picIndex >= 0)
		{
			DrawBTPic(
				x + pic1.dx, y + pic1.dy,
				PicManagerGetOldPic(&gPicManager, pic1.picIndex), tint);
		}
		if (pic2.picIndex >= 0)
		{
			DrawBTPic(
				x + pic2.dx, y + pic2.dy,
				PicManagerGetOldPic(&gPicManager, pic2.picIndex), tint);
		}
		if (pic3.picIndex >= 0)
		{
			DrawBTPic(
				x + pic3.dx, y + pic3.dy,
				PicManagerGetOldPic(&gPicManager, pic3.picIndex), tint);
		}
	}
	else
	{
		DrawShadow(&gGraphicsDevice, Vec2iNew(x, y), Vec2iNew(8, 6));
		if (pic1.picIndex >= 0)
		{
			Blit(
				x + pic1.dx, y + pic1.dy,
				PicManagerGetOldPic(&gPicManager, pic1.picIndex),
				table, BLIT_TRANSPARENT);
		}
		if (pic2.picIndex >= 0)
		{
			Blit(
				x + pic2.dx, y + pic2.dy,
				PicManagerGetOldPic(&gPicManager, pic2.picIndex),
				table, BLIT_TRANSPARENT);
		}
		if (pic3.picIndex >= 0)
		{
			Blit(
				x + pic3.dx, y + pic3.dy,
				PicManagerGetOldPic(&gPicManager, pic3.picIndex),
				table, BLIT_TRANSPARENT);
		}
	}
}


TActor *AddActor(Character *c, struct PlayerData *p)
{
	TActor *actor;
	CCALLOC(actor, sizeof(TActor));

	actor->soundLock = 0;
	actor->weapon = WeaponCreate(c->gun);
	actor->health = c->maxHealth;
	actor->action = ACTORACTION_MOVING;
	actor->tileItem.kind = KIND_CHARACTER;
	actor->tileItem.data = actor;
	actor->tileItem.drawFunc = (TileItemDrawFunc) DrawCharacter;
	actor->tileItem.w = 7;
	actor->tileItem.h = 5;
	actor->tileItem.flags = TILEITEM_IMPASSABLE | TILEITEM_CAN_BE_SHOT;
	actor->tileItem.actor = actor;
	actor->next = actorList;
	actorList = actor;
	actor->flags = FLAGS_SLEEPING | c->flags;
	actor->character = c;
	actor->pData = p;
	actor->direction = DIRECTION_DOWN;
	actor->state = STATE_IDLE;
	actor->slideLock = 0;
	return actor;
}

TActor *RemoveActor(TActor * actor)
{
	TActor **h = &actorList;

	while (*h && *h != actor)
		h = &((*h)->next);
	if (*h)
	{
		int i;
		*h = actor->next;
		RemoveTileItem(&actor->tileItem);
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (actor == gPlayers[i])
			{
				gPlayers[i] = NULL;
				break;
			}
		}
		CFREE(actor->aiContext);
		CFREE(actor);
		return *h;
	}
	return NULL;
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


static void CheckTrigger(TActor *actor, Vec2i pos)
{
	pos = Vec2iToTile(pos);
	if ((gMap[pos.y][pos.x].flags & MAPTILE_TILE_TRIGGER))
	{
		TriggerAt(pos, actor->flags | gMission.flags);
	}
}

static void PickupObject(TActor * actor, TObject * object)
{
	switch (object->objectIndex) {
	case OBJ_JEWEL:
		Score(actor->pData, 10);
		break;

	case OBJ_KEYCARD_RED:
		gMission.flags |= FLAGS_KEYCARD_RED;
		break;

	case OBJ_KEYCARD_BLUE:
		gMission.flags |= FLAGS_KEYCARD_BLUE;
		break;

	case OBJ_KEYCARD_GREEN:
		gMission.flags |= FLAGS_KEYCARD_GREEN;
		break;

	case OBJ_KEYCARD_YELLOW:
		gMission.flags |= FLAGS_KEYCARD_YELLOW;
		break;
/*
    case OBJ_PUZZLE_1:
    case OBJ_PUZZLE_2:
    case OBJ_PUZZLE_3:
    case OBJ_PUZZLE_4:
    case OBJ_PUZZLE_5:
    case OBJ_PUZZLE_6:
    case OBJ_PUZZLE_7:
    case OBJ_PUZZLE_8:
    case OBJ_PUZZLE_9:
    case OBJ_PUZZLE_10:
      gCampaign.puzzleCount++;
      gCampaign.puzzle |= (1 << (object->objectIndex - OBJ_PUZZLE_1));
      DisplayMessage( gCampaign.setting->puzzle->puzzleMsg);
      Score(actor->pData, 200);
      break;
*/
	}
	CheckMissionObjective(
		&gMission, object->tileItem.flags, OBJECTIVE_COLLECT);
	RemoveObject(object);
	SoundPlayAt(
		&gSoundDevice,
		SND_PICKUP,
		Vec2iNew(actor->tileItem.x, actor->tileItem.y));
}

int MoveActor(TActor * actor, int x, int y)
{
	TTileItem *target;
	TObject *object;
	TActor *otherCharacter;
	Vec2i realPos = Vec2iFull2Real(Vec2iNew(x, y));
	Vec2i size = Vec2iNew(actor->tileItem.w, actor->tileItem.h);
	int isDogfight = gCampaign.Entry.mode == CAMPAIGN_MODE_DOGFIGHT;

	// Check collision with wall; try to limit x and y movement if still in
	// collision in those axes
	if (IsCollisionWithWall(realPos, size))
	{
		Vec2i realXPos, realYPos;
		realYPos = Vec2iFull2Real(Vec2iNew(actor->x, y));
		if (IsCollisionWithWall(realYPos, size))
		{
			y = actor->y;
		}
		realXPos = Vec2iFull2Real(Vec2iNew(x, actor->y));
		if (IsCollisionWithWall(realXPos, size))
		{
			x = actor->x;
		}
		if ((x == actor->x && y == actor->y) ||
		    (x != actor->x && y != actor->y))
			return 0;
	}

	realPos = Vec2iFull2Real(Vec2iNew(x, y));
	target = GetItemOnTileInCollision(
		&actor->tileItem, realPos, TILEITEM_IMPASSABLE,
		CalcCollisionTeam(1, actor),
		isDogfight);
	if (target)
	{
		Vec2i realXPos, realYPos;

		if (actor->pData && target->kind == KIND_CHARACTER)
		{
			otherCharacter = target->data;
			if (otherCharacter
			    && (otherCharacter->flags & FLAGS_PRISONER) !=
			    0) {
				otherCharacter->flags &= ~FLAGS_PRISONER;
				CheckMissionObjective(
					&gMission,
					otherCharacter->tileItem.flags,
					OBJECTIVE_RESCUE);
			}
		}

		if (actor->weapon.gun == GUN_KNIFE && actor->health > 0)
		{
			object = target->kind == KIND_OBJECT ? target->data : NULL;
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
						gGunDescriptions[actor->weapon.gun].SoundLockLength;
				}
				return 0;
			}
		}

		realYPos = Vec2iFull2Real(Vec2iNew(actor->x, y));
		if (GetItemOnTileInCollision(
			&actor->tileItem, realYPos, TILEITEM_IMPASSABLE,
			CalcCollisionTeam(1, actor),
			isDogfight))
		{
			y = actor->y;
		}
		realXPos = Vec2iFull2Real(Vec2iNew(x, actor->y));
		if (GetItemOnTileInCollision(
			&actor->tileItem, realXPos, TILEITEM_IMPASSABLE,
			CalcCollisionTeam(1, actor),
			isDogfight))
		{
			x = actor->x;
		}
		realPos = Vec2iFull2Real(Vec2iNew(x, y));
		if ((x == actor->x && y == actor->y) ||
			(x != actor->x && y != actor->y) ||
			IsCollisionWithWall(realPos, size))
		{
			return 0;
		}
	}

	CheckTrigger(actor, Vec2iFull2Real(Vec2iNew(x, y)));

	if (actor->pData)
	{
		realPos = Vec2iFull2Real(Vec2iNew(x, y));
		target = GetItemOnTileInCollision(
			&actor->tileItem, realPos, TILEITEM_CAN_BE_TAKEN,
			CalcCollisionTeam(1, actor),
			isDogfight);
		if (target && target->kind == KIND_OBJECT)
		{
			PickupObject(actor, target->data);
		}
	}

	actor->x = x;
	actor->y = y;
	MoveTileItem(&actor->tileItem, x >> 8, y >> 8);

	if (IsTileInExit(&actor->tileItem, &gMission))
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

void InjureActor(TActor * actor, int injury)
{
	actor->health -= injury;
	if (actor->health <= 0) {
		actor->stateCounter = 0;
		PlayRandomScreamAt(Vec2iNew(actor->tileItem.x, actor->tileItem.y));
		if (actor->pData)
		{
			SoundPlayAt(
				&gSoundDevice,
				SND_HAHAHA,
				Vec2iNew(actor->tileItem.x, actor->tileItem.y));
		}
		CheckMissionObjective(
			&gMission, actor->tileItem.flags, OBJECTIVE_KILL);
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
	Vec2i muzzlePosition = Vec2iNew(actor->x, actor->y);
	Vec2i tilePosition = Vec2iNew(actor->tileItem.x, actor->tileItem.y);
	if (!WeaponCanFire(&actor->weapon))
	{
		return;
	}
	if (GunHasMuzzle(actor->weapon.gun))
	{
		Vec2i muzzleOffset = GunGetMuzzleOffset(
			actor->weapon.gun,
			actor->direction,
			actor->character->looks.armedBody);
		muzzlePosition = Vec2iAdd(muzzlePosition, muzzleOffset);
	}
	WeaponFire(
		&actor->weapon,
		actor->direction,
		muzzlePosition,
		tilePosition,
		actor->flags,
		actor->pData ? actor->pData->playerIndex : -1);
	Score(actor->pData, -GunGetCost(actor->weapon.gun));
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

int ActorTryMove(TActor *actor, int cmd, int hasShot, int ticks, Vec2i *pos)
{
	int canMoveWhenShooting =
		gConfig.Game.MoveWhenShooting ||
		!hasShot ||
		(gConfig.Game.SwitchMoveStyle == SWITCHMOVE_STRAFE &&
		actor->flags & FLAGS_SPECIAL_USED);
	int willMove =
		!actor->petrified &&
		(cmd & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN)) &&
		canMoveWhenShooting;
	if (willMove)
	{
		int moveAmount = actor->character->speed * ticks;
		if (cmd & CMD_LEFT)
		{
			pos->x -= moveAmount;
		}
		else if (cmd & CMD_RIGHT)
		{
			pos->x += moveAmount;
		}
		if (cmd & CMD_UP)
		{
			pos->y -= moveAmount;
		}
		else if (cmd & CMD_DOWN)
		{
			pos->y += moveAmount;
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

void CommandActor(TActor * actor, int cmd, int ticks)
{
	Vec2i movePos = Vec2iNew(actor->x, actor->y);
	int shallMove = NO;

	if (actor->dx || actor->dy)
	{
		int i;
		shallMove = 1;

		movePos.x += actor->dx * ticks;
		movePos.y += actor->dy * ticks;

		for (i = 0; i < ticks; i++)
		{
			if (actor->dx > 0)
			{
				actor->dx = MAX(0, actor->dx - 32);
			}
			else if (actor->dx < 0)
			{
				actor->dx = MIN(0, actor->dx + 32);
			}
			if (actor->dy > 0)
			{
				actor->dy = MAX(0, actor->dy - 32);
			}
			else if (actor->dy < 0)
			{
				actor->dy = MIN(0, actor->dy + 32);
			}
		}
	}

	actor->lastCmd = cmd;

	if (actor->confused)
		cmd = ((cmd & 5) << 1) | ((cmd & 10) >> 1) | (cmd & 0xF0);

	if (actor->health > 0)
	{
		int hasChangedDirection, hasShot, hasMoved;
		hasChangedDirection = ActorTryChangeDirection(actor, cmd);
		hasShot = ActorTryShoot(actor, cmd);
		hasMoved = ActorTryMove(actor, cmd, hasShot, ticks, &movePos);
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
		if (hasMoved)
		{
			shallMove = 1;
		}
	}

	if (shallMove)
	{
		MoveActor(actor, movePos.x, movePos.y);
	}
}

void SlideActor(TActor *actor, int cmd)
{
	int dx, dy;
	
	// Check that actor can slide
	if (actor->slideLock > 0)
	{
		return;
	}

	if (actor->petrified)
		return;

	if (actor->confused)
		cmd = ((cmd & 5) << 1) | ((cmd & 10) >> 1) | (cmd & 0xF0);

	if ((cmd & CMD_LEFT) != 0)
		dx = -5 * 256;
	else if ((cmd & CMD_RIGHT) != 0)
		dx = 5 * 256;
	else
		dx = 0;
	if ((cmd & CMD_UP) != 0)
		dy = -4 * 256;
	else if ((cmd & CMD_DOWN) != 0)
		dy = 4 * 256;
	else
		dy = 0;

	actor->dx = dx;
	actor->dy = dy;

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

void UpdateAllActors(int ticks)
{
	TActor *actor = actorList;
	while (actor) {
		UpdateActorState(actor, ticks);
		if (actor->dead > DEATH_MAX)
		{
			AddObject(
				actor->x, actor->y,
				0, 0,
				&cBloodPics[rand() % BLOOD_MAX],
				0,
				TILEITEM_IS_WRECK);
			actor = RemoveActor(actor);
		}
		else
		{
			// Find actors that are on the same team and colliding,
			// and repel them
			if (gConfig.Game.AllyCollision == ALLYCOLLISION_REPEL)
			{
				Vec2i realPos = Vec2iScaleDiv(
					Vec2iNew(actor->x, actor->y), 256);
				TTileItem *collidingItem = GetItemOnTileInCollision(
					&actor->tileItem, realPos, TILEITEM_IMPASSABLE,
					COLLISIONTEAM_NONE,
					gCampaign.Entry.mode == CAMPAIGN_MODE_DOGFIGHT);
				if (collidingItem && collidingItem->kind == KIND_CHARACTER)
				{
					TActor *collidingActor = collidingItem->actor;
					if (CalcCollisionTeam(1, collidingActor) ==
						CalcCollisionTeam(1, actor))
					{
						Vec2i v = Vec2iNew(
							actor->x - collidingActor->x,
							actor->y - collidingActor->y);
						if (Vec2iEqual(v, Vec2iZero()))
						{
							v = Vec2iNew(1, 0);
						}
						v = Vec2iScale(Vec2iNorm(v), REPEL_STRENGTH);
						actor->dx += v.x;
						actor->dy += v.y;
						collidingActor->dx -= v.x;
						collidingActor->dy -= v.y;
					}
				}
			}
			actor = actor->next;
		}
	}
}

TActor *ActorList(void)
{
	return actorList;
}

void KillAllActors(void)
{
	TActor *actor;
	while (actorList) {
		actor = actorList;
		actorList = actorList->next;
		RemoveActor(actor);
	}
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
	Vec2i hitVector,
	int power,
	special_damage_e damage,
	int isHitSoundEnabled,
	int isInvulnerable,
	Vec2i hitLocation)
{
	assert(!ActorIsImmune(actor, damage));
	ActorTakeSpecialDamage(actor, damage);

	if (gConfig.Game.ShotsPushback)
	{
		actor->dx += (power * hitVector.x) / 25;
		actor->dy += (power * hitVector.y) / 25;
	}

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
