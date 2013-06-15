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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "pics.h"
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


TActor *gPlayer1 = NULL;
TActor *gPlayer2 = NULL;
TActor *gPrisoner = NULL;

TranslationTable tableFlamed;
TranslationTable tableGreen;
TranslationTable tablePoison;
TranslationTable tableGray;
TranslationTable tableBlack;
TranslationTable tableDarker;
TranslationTable tablePurple;

void SetShade(TranslationTable * table, int start, int end, int shade);

struct CharacterDescription characterDesc[CHARACTER_COUNT];


static TActor *actorList = NULL;


static int transitionTable[STATE_COUNT] = {
	0,
	STATE_IDLE,
	STATE_IDLE,
	STATE_WALKING_2,
	STATE_WALKING_3,
	STATE_WALKING_4,
	STATE_WALKING_1,
	STATE_RECOIL,
	STATE_SHOOTING
};

static int delayTable[STATE_COUNT] = {
	90,
	60,
	60,
	8,
	8,
	8,
	8,
	8,
	8
};


typedef unsigned char ColorShade[10];

static ColorShade colorShades[SHADE_COUNT] = {
	{52, 53, 54, 55, 56, 57, 58, 59, 60, 61},
	{2, 3, 4, 5, 6, 7, 8, 9, 9, 9},
	{68, 69, 70, 71, 72, 73, 74, 75, 76, 77},
	{84, 85, 86, 87, 88, 89, 90, 91, 92, 93},
	{100, 101, 102, 103, 104, 105, 106, 107, 107, 107},
	{116, 117, 118, 119, 120, 121, 122, 123, 124, 125},
	{132, 133, 134, 135, 136, 137, 138, 139, 140, 141},
	{32, 33, 34, 35, 36, 37, 38, 39, 40, 41},
	{36, 37, 38, 39, 40, 41, 42, 43, 44, 45},
	{41, 42, 43, 44, 45, 46, 47, 47, 47, 47},
	{144, 145, 146, 147, 148, 149, 150, 151, 151, 151},
	{4, 5, 6, 7, 8, 9, 9, 9, 9, 9},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{16, 17, 18, 19, 20, 21, 22, 23, 24, 25}
};

unsigned char BestMatch(int r, int g, int b);


void SetRandomTintTable(TranslationTable *table, int tint)
{
	// Generate three random numbers between 0-1, and divide each by 1/3rd the sum.
	// The resulting three scaled numbers should add up to 1.
	double r_scalar = rand() * 1.0 / RAND_MAX;
	double g_scalar = rand() * 1.0 / RAND_MAX;
	double b_scalar = rand() * 1.0 / RAND_MAX;
	int i;
	double scale_factor = r_scalar + g_scalar + b_scalar;
	r_scalar /= scale_factor;
	g_scalar /= scale_factor;
	b_scalar /= scale_factor;
	for (i = 0; i < 256; i++)
	{
		unsigned char f = (unsigned char)floor(
			0.4 * gPalette[i].red +
			0.49 * gPalette[i].green +
			0.11 * gPalette[i].blue);
		(*table)[i] = BestMatch(
			(unsigned char)(r_scalar * f * tint / 256),
			(unsigned char)(g_scalar * f * tint / 256),
			(unsigned char)(b_scalar * f * tint / 256));
	}
}


void DrawCharacter(int x, int y, TActor * actor)
{
	int dir = actor->direction, state = actor->state;
	int headDir = dir;

	struct CharacterDescription *c = &characterDesc[actor->character];
	TranslationTable *table = (TranslationTable *) c->table;
	int f = c->facePic;
	int b;
	int g = GunGetPic(actor->weapon.gun);
	gunstate_e gunState = GUNSTATE_READY;

	TOffsetPic body, head, gun;
	TOffsetPic pic1, pic2, pic3;

	int transparent = (actor->flags & FLAGS_SEETHROUGH) != 0;

	if (actor->flamed)
		table = &tableFlamed;
	else if (actor->poisoned)
		table = &tableGreen;
	else if (actor->petrified)
		table = &tableGray;
	else if (actor->confused)
		table = &tablePurple;
	else if (transparent)
		table = &tableDarker;

	if (actor->dead) {
		if (actor->dead <= DEATH_MAX) {
			body = cDeathPics[actor->dead - 1];
			if (transparent)
				DrawBTPic(x + body.dx, y + body.dy,
					  gPics[body.picIndex], table,
					  gRLEPics[body.picIndex]);
			else
				DrawTTPic(x + body.dx, y + body.dy,
					  gPics[body.picIndex], table,
					  gRLEPics[body.picIndex]);
		}
		return;
	}

	actor->flags |= FLAGS_VISIBLE;
	actor->flags &= ~FLAGS_SLEEPING;

	if (state == STATE_IDLELEFT)
		headDir = (dir + 7) % 8;
	else if (state == STATE_IDLERIGHT)
		headDir = (dir + 1) % 8;

	if (g < 0)
		b = c->unarmedBodyPic;
	else {
		b = c->armedBodyPic;
		if (state == STATE_SHOOTING)
		{
			gunState = GUNSTATE_FIRING;
		}
		else if (state == STATE_RECOIL)
		{
			gunState = GUNSTATE_RECOIL;
		}
	}

	body.dx = cBodyOffset[b][dir].dx;
	body.dy = cBodyOffset[b][dir].dy;
	body.picIndex = cBodyPic[b][dir][state];

	head.dx = cNeckOffset[b][dir].dx + cHeadOffset[f][headDir].dx;
	head.dy = cNeckOffset[b][dir].dy + cHeadOffset[f][headDir].dy;
	head.picIndex = cHeadPic[f][headDir][state];

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

	switch (actor->direction) {
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
		// should never get here
		return;
	}

	if (transparent) {
		if (pic1.picIndex >= 0)
			DrawBTPic(x + pic1.dx, y + pic1.dy,
				  gPics[pic1.picIndex], table,
				  gRLEPics[pic1.picIndex]);
		if (pic2.picIndex >= 0)
			DrawBTPic(x + pic2.dx, y + pic2.dy,
				  gPics[pic2.picIndex], table,
				  gRLEPics[pic2.picIndex]);
		if (pic3.picIndex >= 0)
			DrawBTPic(x + pic3.dx, y + pic3.dy,
				  gPics[pic3.picIndex], table,
				  gRLEPics[pic3.picIndex]);
	} else if (table) {
		if (pic1.picIndex >= 0)
			DrawTTPic(x + pic1.dx, y + pic1.dy,
				  gPics[pic1.picIndex], table,
				  gRLEPics[pic1.picIndex]);
		if (pic2.picIndex >= 0)
			DrawTTPic(x + pic2.dx, y + pic2.dy,
				  gPics[pic2.picIndex], table,
				  gRLEPics[pic2.picIndex]);
		if (pic3.picIndex >= 0)
			DrawTTPic(x + pic3.dx, y + pic3.dy,
				  gPics[pic3.picIndex], table,
				  gRLEPics[pic3.picIndex]);
	} else {
		if (pic1.picIndex >= 0)
			DrawTPic(x + pic1.dx, y + pic1.dy,
				 gPics[pic1.picIndex], NULL);
		if (pic2.picIndex >= 0)
			DrawTPic(x + pic2.dx, y + pic2.dy,
				 gPics[pic2.picIndex], NULL);
		if (pic3.picIndex >= 0)
			DrawTPic(x + pic3.dx, y + pic3.dy,
				 gPics[pic3.picIndex], NULL);
	}
}


TActor *AddActor(int character)
{
	TActor *actor;
	CCALLOC(actor, sizeof(TActor));

	actor->soundLock = 0;
	actor->weapon = WeaponCreate(characterDesc[character].defaultGun);
	actor->health = characterDesc[character].maxHealth;
	actor->tileItem.kind = KIND_CHARACTER;
	actor->tileItem.data = actor;
	actor->tileItem.drawFunc = (TileItemDrawFunc) DrawCharacter;
	actor->tileItem.w = 7;
	actor->tileItem.h = 5;
	actor->tileItem.flags = TILEITEM_IMPASSABLE | TILEITEM_CAN_BE_SHOT;
	actor->next = actorList;
	actorList = actor;
	actor->flags = FLAGS_SLEEPING | characterDesc[character].flags;
	actor->character = character;
	actor->direction = DIRECTION_DOWN;
	actor->state = STATE_IDLE;
	return actor;
}

TActor *RemoveActor(TActor * actor)
{
	TActor **h = &actorList;

	while (*h && *h != actor)
		h = &((*h)->next);
	if (*h) {
		*h = actor->next;
		RemoveTileItem(&actor->tileItem);
		if (actor == gPlayer1)
			gPlayer1 = NULL;
		else if (actor == gPlayer2)
			gPlayer2 = NULL;
		CFREE(actor);
		return *h;
	}
	return NULL;
}

void SetStateForActor(TActor * actor, int state)
{
	actor->state = state;
	if (state == STATE_RECOIL)
	{
		// This is to make sure the player stays frozen after firing the gun
		// TODO: rethink this; makes sniper gun enemies freeze for too long
		actor->stateCounter = actor->weapon.lock;
	}
	else
	{
		actor->stateCounter = delayTable[state];
	}
}

void UpdateActorState(TActor * actor, int ticks)
{
	WeaponUpdate(&actor->weapon, ticks);

	if (actor->health > 0) {
		if (actor->flamed)
			actor->flamed--;
		if (actor->poisoned) {
			if ((actor->poisoned & 7) == 0)
				InjureActor(actor, 1);
			actor->poisoned--;
		}
		if (actor->petrified) {
			actor->petrified -= ticks;
			if (actor->petrified < 0)
				actor->petrified = 0;
		}
		if (actor->confused) {
			actor->confused -= ticks;
			if (actor->confused < 0)
				actor->confused = 0;
		}
	}

	if (actor->stateCounter) {
		actor->stateCounter -= ticks;
		if (actor->stateCounter > 0)
			return;
		actor->stateCounter = 0;
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
		SoundPlayAt(SND_FOOTSTEP, actor->tileItem.x, actor->tileItem.y);
		actor->soundLock = SOUND_LOCK_FOOTSTEP;
	}

	if (actor->state == STATE_IDLE && !actor->petrified)
		SetStateForActor(actor, ((rand() & 1) != 0 ?
					 STATE_IDLELEFT :
					 STATE_IDLERIGHT));
	else
		SetStateForActor(actor, transitionTable[actor->state]);

	// Sound lock
	actor->soundLock -= ticks;
	if (actor->soundLock < 0)
	{
		actor->soundLock = 0;
	}
}


static void CheckTrigger(TActor * actor, int x, int y)
{
	x /= TILE_WIDTH;
	y /= TILE_HEIGHT;
	if ((Map(x, y).flags & TILE_TRIGGER) != 0) {
//  if ((actor->flags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY)) != 0)
		TriggerAt(x, y, actor->flags | gMission.flags);
//  else
//    TriggerAt( x, y, actor->flags);
	}
}

static void PickupObject(TActor * actor, TObject * object)
{
	switch (object->objectIndex) {
	case OBJ_JEWEL:
		Score(actor->flags, 10);
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
      Score( actor->flags, 200);
      break;
*/
	}
	CheckMissionObjective(object->tileItem.flags);
	RemoveObject(object);
	SoundPlayAt(SND_PICKUP, actor->tileItem.x, actor->tileItem.y);
}

int MoveActor(TActor * actor, int x, int y)
{
	TTileItem *target;
	TObject *object;
	TActor *otherCharacter;

	if (CheckWall(x, y, actor->tileItem.w, actor->tileItem.h)) {
		if (CheckWall
		    (actor->x, y, actor->tileItem.w, actor->tileItem.h))
			y = actor->y;
		if (CheckWall
		    (x, actor->y, actor->tileItem.w, actor->tileItem.h))
			x = actor->x;
		if ((x == actor->x && y == actor->y) ||
		    (x != actor->x && y != actor->y))
			return 0;
	}

	target =
	    CheckTileItemCollision(&actor->tileItem, x >> 8, y >> 8,
				   TILEITEM_IMPASSABLE);
	if (target) {
		if ((actor->flags & FLAGS_PLAYERS) != 0
		    && target->kind == KIND_CHARACTER) {
			otherCharacter = target->data;
			if (otherCharacter
			    && (otherCharacter->flags & FLAGS_PRISONER) !=
			    0) {
				otherCharacter->flags &= ~FLAGS_PRISONER;
				CheckMissionObjective(otherCharacter->
						      tileItem.flags);
			}
		}

		if (actor->weapon.gun == GUN_KNIFE && actor->health > 0)
		{
			object = target->kind == KIND_OBJECT ? target->data : NULL;
			if (!object || (object->flags & OBJFLAG_DANGEROUS) == 0)
			{
				DamageSomething(0, 0, 2, actor->flags, target, 0);
				return 0;
			}
		}

		if (CheckTileItemCollision
		    (&actor->tileItem, actor->x >> 8, y >> 8,
		     TILEITEM_IMPASSABLE))
			y = actor->y;
		if (CheckTileItemCollision
		    (&actor->tileItem, x >> 8, actor->y >> 8,
		     TILEITEM_IMPASSABLE))
			x = actor->x;
		if ((x == actor->x && y == actor->y) ||
		    (x != actor->x && y != actor->y) ||
		    CheckWall(x, y, actor->tileItem.w, actor->tileItem.h))
			return 0;
	}

	CheckTrigger(actor, x >> 8, y >> 8);

	if ((actor->flags & FLAGS_PLAYERS) != 0) {
		target =
		    CheckTileItemCollision(&actor->tileItem, x >> 8,
					   y >> 8, TILEITEM_CAN_BE_TAKEN);
		if (target && target->kind == KIND_OBJECT)
			PickupObject(actor, target->data);
	}

	actor->x = x;
	actor->y = y;
	MoveTileItem(&actor->tileItem, x >> 8, y >> 8);
	return 1;
}

void PlayRandomScreamAt(int x, int y)
{
	#define SCREAM_COUNT 4
	static sound_e screamTable[SCREAM_COUNT] =
		{ SND_KILL, SND_KILL2, SND_KILL3, SND_KILL4 };
	static int screamIndex = 0;
	SoundPlayAt(screamTable[screamIndex], x, y);
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
		PlayRandomScreamAt(actor->tileItem.x, actor->tileItem.y);
		if ((actor->flags & FLAGS_PLAYERS) != 0)
		{
			SoundPlayAt(SND_HAHAHA, actor->tileItem.x, actor->tileItem.y);
		}
		CheckMissionObjective(actor->tileItem.flags);
	}
}

void Score(int flags, int points)
{
	if (flags & FLAGS_PLAYER1) {
		gPlayer1Data.score += points;
		gPlayer1Data.totalScore += points;
	} else if (flags & FLAGS_PLAYER2) {
		gPlayer2Data.score += points;
		gPlayer2Data.totalScore += points;
	}
}

Vector2i GetMuzzleOffset(TActor *actor)
{
	int b, g, d = actor->direction;
	Vector2i position;

	b = characterDesc[actor->character].armedBodyPic;
	g = GunGetPic(actor->weapon.gun);
	position.x =
		cGunHandOffset[b][d].dx +
		cGunPics[g][d][GUNSTATE_FIRING].dx +
		cMuzzleOffset[g][d].dx;
	position.y =
		cGunHandOffset[b][d].dy +
		cGunPics[g][d][GUNSTATE_FIRING].dy +
		cMuzzleOffset[g][d].dy + BULLET_Z;
	position.x *= 256;
	position.y *= 256;
	return position;
}

void Shoot(TActor *actor)
{
	Vector2i muzzlePosition;
	Vector2i tilePosition;
	if (!WeaponCanFire(&actor->weapon))
	{
		return;
	}
	muzzlePosition.x = actor->x;
	muzzlePosition.y = actor->y;
	if (GunHasMuzzle(actor->weapon.gun))
	{
		Vector2i muzzleOffset = GetMuzzleOffset(actor);
		muzzlePosition.x += muzzleOffset.x;
		muzzlePosition.y += muzzleOffset.y;
	}
	tilePosition.x = actor->tileItem.x;
	tilePosition.y = actor->tileItem.y;
	WeaponFire(
		&actor->weapon, actor->direction, muzzlePosition, tilePosition, actor->flags);
	Score(actor->flags, -GunGetScore(actor->weapon.gun));
}

void CommandActor(TActor * actor, int cmd)
{
	int x = actor->x, y = actor->y;
	int shallMove = NO;
	int resetDir = NO;

	if (actor->dx || actor->dy) {
		shallMove = YES;
		resetDir = YES;

		x += actor->dx;
		y += actor->dy;

		if (actor->dx > 0)
			actor->dx -= 32;
		else if (actor->dx < 0)
			actor->dx += 32;
		if (abs(actor->dx) < 32)
			actor->dx = 0;
		if (actor->dy > 0)
			actor->dy -= 32;
		else if (actor->dy < 0)
			actor->dy += 32;
		if (abs(actor->dy) < 32)
			actor->dy = 0;
	}

	actor->lastCmd = cmd;

	if (actor->confused)
		cmd = ((cmd & 5) << 1) | ((cmd & 10) >> 1) | (cmd & 0xF0);

	if (actor->health > 0) {
		if (!actor->petrified && (cmd & CMD_BUTTON1) != 0) {
			if (actor->state != STATE_SHOOTING &&
			    actor->state != STATE_RECOIL)
				SetStateForActor(actor, STATE_SHOOTING);
			if (cmd &
			    (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN))
				actor->direction = CmdToDirection(cmd);
			Shoot(actor);
		} else if (!actor->petrified
			   && (cmd &
			       (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN))
			   != 0) {
			shallMove = YES;

			if (cmd & CMD_LEFT)
				x -= characterDesc[actor->character].speed;
			else if (cmd & CMD_RIGHT)
				x += characterDesc[actor->character].speed;
			if (cmd & CMD_UP)
				y -= characterDesc[actor->character].speed;
			else if (cmd & CMD_DOWN)
				y += characterDesc[actor->character].speed;

			if (actor->state != STATE_WALKING_1 &&
			    actor->state != STATE_WALKING_2 &&
			    actor->state != STATE_WALKING_3 &&
			    actor->state != STATE_WALKING_4)
				SetStateForActor(actor, STATE_WALKING_1);
			actor->direction = CmdToDirection(cmd);
		} else {
			if (actor->state != STATE_IDLE &&
			    actor->state != STATE_IDLELEFT &&
			    actor->state != STATE_IDLERIGHT)
				SetStateForActor(actor, STATE_IDLE);
		}
	}

	if (shallMove)
		MoveActor(actor, x, y);

	if (resetDir) {
		if (actor->health > 0 &&
		    (cmd & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN)) !=
		    0)
			actor->direction = CmdToDirection(cmd);
	}
}

void SlideActor(TActor * actor, int cmd)
{
	int dx, dy;

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
}

void UpdateAllActors(int ticks)
{
	TActor *actor = actorList;
	while (actor) {
		UpdateActorState(actor, ticks);
		if (actor->dead > DEATH_MAX) {
			AddObject(actor->x, actor->y, 0, 0, &cBloodPics[rand() % BLOOD_MAX], 0, 0);
			actor = RemoveActor(actor);
		} else
			actor = actor->next;
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

unsigned char BestMatch(int r, int g, int b)
{
	int d, dMin = 0;
	int i;
	int best = -1;

	for (i = 0; i < 256; i++) {
		d = (r - gPalette[i].red) * (r - gPalette[i].red) +
		    (g - gPalette[i].green) * (g - gPalette[i].green) +
		    (b - gPalette[i].blue) * (b - gPalette[i].blue);
		if (best < 0 || d < dMin) {
			best = i;
			dMin = d;
		}
	}
	return (unsigned char)best;
}

void SetCharacterColors(TranslationTable * t, int arms, int body, int legs,
			int skin, int hair)
{
	SetShade(t, BODY_START, BODY_END, body);
	SetShade(t, ARMS_START, ARMS_END, arms);
	SetShade(t, LEGS_START, LEGS_END, legs);
	SetShade(t, SKIN_START, SKIN_END, skin);
	SetShade(t, HAIR_START, HAIR_END, hair);
}

void SetCharacter(int index, int face, int skin, int hair, int body,
		  int arms, int legs)
{
	characterDesc[index].facePic = face;
	SetCharacterColors(&characterDesc[index].table, arms, body, legs, skin, hair);
}

void BuildTranslationTables(void)
{
	int i;
	unsigned char f;

	for (i = 0; i < 256; i++)
	{
		f = (unsigned char)floor(
			0.3 * gPalette[i].red +
			0.59 * gPalette[i].green +
			0.11 * gPalette[i].blue);
		tableFlamed[i] = BestMatch(f, 0, 0);
	}
	for (i = 0; i < 256; i++)
	{
		f = (unsigned char)floor(
			0.4 * gPalette[i].red +
			0.49 * gPalette[i].green +
			0.11 * gPalette[i].blue);
		tableGreen[i] = BestMatch(0, 2 * f / 3, 0);
	}
	for (i = 0; i < 256; i++)
	{
		tablePoison[i] = BestMatch(
			gPalette[i].red + 5,
			gPalette[i].green + 15,
			gPalette[i].blue + 5);
	}
	for (i = 0; i < 256; i++)
	{
		f = (unsigned char)floor(
			0.4 * gPalette[i].red +
			0.49 * gPalette[i].green +
			0.11 * gPalette[i].blue);
		tableGray[i] = BestMatch(f, f, f);
	}
	for (i = 0; i < 256; i++)
	{
		tableBlack[i] = BestMatch(0, 0, 0);
	}
	for (i = 0; i < 256; i++)
	{
		f = (unsigned char)floor(
			0.4 * gPalette[i].red +
			0.49 * gPalette[i].green +
			0.11 * gPalette[i].blue);
		tablePurple[i] = BestMatch(f, 0, f);
	}
	for (i = 0; i < 256; i++)
	{
		tableDarker[i] = BestMatch(
			(200 * gPalette[i].red) / 256,
			(200 * gPalette[i].green) / 256,
			(200 * gPalette[i].blue) / 256);
	}
}

void InitializeTranslationTables(void)
{
	int i, f;

	for (i = 0; i < CHARACTER_COUNT; i++)
		for (f = 0; f < 256; f++)
			characterDesc[i].table[f] = (f & 0xFF);

}

void SetShade(TranslationTable * table, int start, int end, int shade)
{
	int i;

	for (i = start; i <= end; i++)
		(*table)[i] = colorShades[shade][i - start];
}
