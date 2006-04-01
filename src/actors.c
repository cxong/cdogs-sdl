/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

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

-------------------------------------------------------------------------------

 actors.c - character related functions

*/

#include <stdlib.h>
#include <string.h>
#include "actors.h"
#include "pics.h"
#include "sounds.h"
#include "defs.h"
#include "objs.h"
#include "gamedata.h"
#include "triggers.h"
#include "hiscores.h"
#include "mission.h"
#include "game.h"


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

struct GunDescription gunDesc[] = {
	{GUNPIC_KNIFE, "Knife"},
	{GUNPIC_BLASTER, "Machine gun"},
	{-1, "Grenades"},
	{GUNPIC_BLASTER, "Flamer"},
	{GUNPIC_BLASTER, "Shotgun"},
	{GUNPIC_BLASTER, "Powergun"},
	{-1, "Shrapnel bombs"},
	{-1, "Molotovs"},
	{GUNPIC_BLASTER, "Sniper gun"},
	{-1, "Prox. mine"},
	{-1, "Dynamite"},
	{-1, "Chemo bombs"},
	{GUNPIC_BLASTER, "Petrify gun"},
	{GUNPIC_BLASTER, "Browny gun"},
	{-1, "Confusion bombs"},
	{GUNPIC_BLASTER, "Chemo gun"}
};


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

#define SCREAM_COUNT   4
static int screamTable[SCREAM_COUNT] =
    { SND_KILL, SND_KILL2, SND_KILL3, SND_KILL4 };
static int scream = 0;


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


void DrawCharacter(int x, int y, TActor * actor)
{
	int dir = actor->direction, state = actor->state;
	int headDir = dir;

	struct CharacterDescription *c = &characterDesc[actor->character];
	TranslationTable *table = (TranslationTable *) c->table;
	int f = c->facePic;
	int b;
	int g = gunDesc[actor->gun].gunPic;
	int gunState = 0;

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
			gunState = GUNSTATE_FIRING;
		else if (state == STATE_RECOIL)
			gunState = GUNSTATE_RECOIL;
		else
			gunState = GUNSTATE_READY;

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
	TActor *actor = malloc(sizeof(TActor));

	memset(actor, 0, sizeof(TActor));
	actor->gun = characterDesc[character].defaultGun;
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
		free(actor);
		return *h;
	}
	return NULL;
}

void SetStateForActor(TActor * actor, int state)
{
	actor->state = state;
	if (state == STATE_RECOIL)
		actor->stateCounter = actor->gunLock;
	else
		actor->stateCounter = delayTable[state];
}

void UpdateActorState(TActor * actor, int ticks)
{
	if (actor->gunLock) {
		actor->gunLock -= ticks;
		if (actor->gunLock < 0)
			actor->gunLock = 0;
	}
	if (actor->sndLock) {
		actor->sndLock -= ticks;
		if (actor->sndLock < 0)
			actor->sndLock = 0;
	}

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

	if (actor->state == STATE_IDLE && !actor->petrified)
		SetStateForActor(actor, ((rand() & 1) != 0 ?
					 STATE_IDLELEFT :
					 STATE_IDLERIGHT));
	else
		SetStateForActor(actor, transitionTable[actor->state]);
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
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_PICKUP);
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

		if (actor->gun == GUN_KNIFE && actor->health > 0) {
			object =
			    (target->kind ==
			     KIND_OBJECT ? target->data : NULL);
			if (!object
			    || (object->flags & OBJFLAG_DANGEROUS) == 0) {
				DamageSomething(0, 0, 2, actor->flags,
						target, 0);
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

void InjureActor(TActor * actor, int injury)
{
	actor->health -= injury;
	if (actor->health <= 0) {
		actor->stateCounter = 0;
		PlaySoundAt(actor->tileItem.x, actor->tileItem.y,
			    screamTable[scream]);
		scream++;
		if (scream >= SCREAM_COUNT)
			scream = 0;
		if ((actor->flags & FLAGS_PLAYERS) != 0)
			PlaySoundAt(actor->tileItem.x, actor->tileItem.y,
				    SND_HAHAHA);
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

void LaunchGrenade(TActor * actor)
{
	int angle;

	angle = dir2angle[actor->direction];
	AddGrenade(actor->x, actor->y, angle, actor->flags,
		   MOBOBJ_GRENADE);
	actor->gunLock = 30;
	Score(actor->flags, -20);
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_LAUNCH);
}

void LaunchFragGrenade(TActor * actor)
{
	int angle;

	angle = dir2angle[actor->direction];
	AddGrenade(actor->x, actor->y, angle, actor->flags,
		   MOBOBJ_FRAGGRENADE);
	actor->gunLock = 30;
	Score(actor->flags, -20);
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_LAUNCH);
}

void LaunchMolotov(TActor * actor)
{
	int angle;

	angle = dir2angle[actor->direction];
	AddGrenade(actor->x, actor->y, angle, actor->flags,
		   MOBOBJ_MOLOTOV);
	actor->gunLock = 30;
	Score(actor->flags, -20);
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_LAUNCH);
}

void LaunchGasBomb(TActor * actor)
{
	int angle;

	angle = dir2angle[actor->direction];
	AddGrenade(actor->x, actor->y, angle, actor->flags,
		   MOBOBJ_GASBOMB);
	actor->gunLock = 30;
	Score(actor->flags, -10);
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_LAUNCH);
}

void GetMuzzle(TActor * actor, int *dx, int *dy)
{
	int b, g, d = actor->direction;

	b = characterDesc[actor->character].armedBodyPic;
	g = gunDesc[actor->gun].gunPic;
	*dx = cGunHandOffset[b][d].dx +
	    cGunPics[g][d][GUNSTATE_FIRING].dx + cMuzzleOffset[g][d].dx;
	*dy = cGunHandOffset[b][d].dy +
	    cGunPics[g][d][GUNSTATE_FIRING].dy +
	    cMuzzleOffset[g][d].dy + BULLET_Z;
}

void MachineGun(TActor * actor)
{
	int angle;
	int d = actor->direction;
	int dx, dy;

	angle = dir2angle[d];
	angle += (rand() & 7) - 4;
	if (angle < 0)
		angle += 256;

	GetMuzzle(actor, &dx, &dy);
	AddBullet(actor->x + 256 * dx, actor->y + 256 * dy, angle,
		  MG_SPEED, MG_RANGE, MG_POWER, actor->flags);
	actor->gunLock = 6;
	Score(actor->flags, -1);
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_MACHINEGUN);
}

void Flamer(TActor * actor)
{
	int angle;
	int d = actor->direction;
	int dx, dy;

	angle = dir2angle[d];
	GetMuzzle(actor, &dx, &dy);
	AddFlame(actor->x + 256 * dx, actor->y + 256 * dy, angle,
		 actor->flags);
	actor->gunLock = 6;
	Score(actor->flags, -1);
	if (!actor->sndLock) {
		PlaySoundAt(actor->tileItem.x, actor->tileItem.y,
			    SND_FLAMER);
		actor->sndLock = 48;
	}
}

void ShotGun(TActor * actor)
{
	int i, angle;
	int d = actor->direction;
	int dx, dy;

	GetMuzzle(actor, &dx, &dy);

	angle = dir2angle[d];
	angle -= 16;
	for (i = 0; i <= 32; i += 8, angle += 8)
		AddBullet(actor->x + 256 * dx, actor->y + 256 * dy,
			  (angle > 0 ? angle : angle + 256),
			  SHOTGUN_SPEED, SHOTGUN_RANGE, SHOTGUN_POWER,
			  actor->flags);

	Score(actor->flags, -5);
	actor->gunLock = 50;
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_SHOTGUN);
}

void PowerGun(TActor * actor)
{
	int d = actor->direction;
	int dx, dy;

	GetMuzzle(actor, &dx, &dy);
	AddLaserBolt(actor->x + 256 * dx, actor->y + 256 * dy, d,
		     actor->flags);
	actor->gunLock = 20;
	Score(actor->flags, -2);
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_POWERGUN);
}

void SniperGun(TActor * actor)
{
	int d = actor->direction;
	int dx, dy;

	GetMuzzle(actor, &dx, &dy);
	AddSniperBullet(actor->x + 256 * dx, actor->y + 256 * dy, d,
			actor->flags);
	actor->gunLock = 100;
	Score(actor->flags, -5);
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_LASER);
}

void BrownGun(TActor * actor)
{
	int angle;
	int d = actor->direction;
	int dx, dy;

	angle = dir2angle[d];
	GetMuzzle(actor, &dx, &dy);
	AddBrownBullet(actor->x + 256 * dx, actor->y + 256 * dy, angle,
		       768, 45, 15, actor->flags);
	actor->gunLock = 30;
	Score(actor->flags, -7);
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_POWERGUN);
}

void Petrifier(TActor * actor)
{
	int angle;
	int d = actor->direction;
	int dx, dy;

	angle = dir2angle[d];
	GetMuzzle(actor, &dx, &dy);
	AddPetrifierBullet(actor->x + 256 * dx, actor->y + 256 * dy, angle,
			   768, 45, actor->flags);
	actor->gunLock = 100;
	Score(actor->flags, -7);
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_LASER);
}

void GasGun(TActor * actor)
{
	int angle;
	int d = actor->direction;
	int dx, dy;

	angle = dir2angle[d];
	GetMuzzle(actor, &dx, &dy);
	AddGasCloud(actor->x + 256 * dx, actor->y + 256 * dy, angle,
		    384, 35, actor->flags, SPECIAL_POISON);
	actor->gunLock = 6;
	Score(actor->flags, -1);
	if (!actor->sndLock) {
		PlaySoundAt(actor->tileItem.x, actor->tileItem.y,
			    SND_FLAMER);
		actor->sndLock = 48;
	}
}

void ConfuseBomb(TActor * actor)
{
	int angle;

	angle = dir2angle[actor->direction];
	AddGrenade(actor->x, actor->y, angle, actor->flags,
		   MOBOBJ_GASBOMB2);
	actor->gunLock = 30;
	Score(actor->flags, -10);
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_LAUNCH);
}

void Heatseeker(TActor * actor)
{
	int angle;
	int d = actor->direction;
	int dx, dy;

	angle = dir2angle[d];
	GetMuzzle(actor, &dx, &dy);
	AddHeatseeker(actor->x + 256 * dx, actor->y + 256 * dy, angle,
		      512, 60, 20, actor->flags);
	actor->gunLock = 30;
	Score(actor->flags, -7);
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_LAUNCH);
}

void PulseRifle(TActor * actor)
{
	int angle;
	int d = actor->direction;
	int dx, dy;

	angle = dir2angle[d];
	angle += (rand() & 7) - 4;
	if (angle < 0)
		angle += 256;

	GetMuzzle(actor, &dx, &dy);
	AddRapidBullet(actor->x + 256 * dx, actor->y + 256 * dy, angle,
		       1280, 25, 7, actor->flags);
	actor->gunLock = 4;
	Score(actor->flags, -1);
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_MINIGUN);
}

void Mine(TActor * actor)
{
	AddProximityMine(actor->x, actor->y, actor->flags);
	actor->gunLock = 100;
	Score(actor->flags, -10);
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_HAHAHA);
}

void Dynamite(TActor * actor)
{
	AddDynamite(actor->x, actor->y, actor->flags);
	actor->gunLock = 100;
	Score(actor->flags, -5);
	PlaySoundAt(actor->tileItem.x, actor->tileItem.y, SND_HAHAHA);
}

void Shoot(TActor * actor)
{
	if (actor->gunLock)
		return;

	switch (actor->gun) {
	case GUN_MG:
		MachineGun(actor);
		break;

	case GUN_GRENADE:
		LaunchGrenade(actor);
		break;

	case GUN_FLAMER:
		Flamer(actor);
		break;

	case GUN_SHOTGUN:
		ShotGun(actor);
		break;

	case GUN_POWERGUN:
		PowerGun(actor);
		break;

	case GUN_FRAGGRENADE:
		LaunchFragGrenade(actor);
		break;

	case GUN_MOLOTOV:
		LaunchMolotov(actor);
		break;

	case GUN_SNIPER:
		SniperGun(actor);
		break;

	case GUN_GASBOMB:
		LaunchGasBomb(actor);
		break;

	case GUN_PETRIFY:
		Petrifier(actor);
		break;

	case GUN_BROWN:
		BrownGun(actor);
		break;

	case GUN_CONFUSEBOMB:
		ConfuseBomb(actor);
		break;

	case GUN_GASGUN:
		GasGun(actor);
		break;

	case GUN_MINE:
		Mine(actor);
		break;

	case GUN_DYNAMITE:
		Dynamite(actor);
		break;
	}
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
/*
    The infamous turn-and-shoot maneuver...

    if (!actor->petrified && (cmd & (CMD_BUTTON1 | CMD_BUTTON4)) == CMD_BUTTON4)
    {
      actor->direction = ((actor->direction + 4) & 7);
      if (actor->state != STATE_SHOOTING &&
          actor->state != STATE_RECOIL)
        SetStateForActor( actor, STATE_SHOOTING);
      Shoot( actor);
    }
*/
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
			AddObject(actor->x, actor->y, 0, 0,
				 &cBloodPics[rand() % BLOOD_MAX], 0, 0);
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

static int BestMatch(int r, int g, int b)
{
	int d, i, best = -1, dMin = 0;

	for (i = 0; i < 256; i++) {
		d = (r - gPalette[i].red) * (r - gPalette[i].red) +
		    (g - gPalette[i].green) * (g - gPalette[i].green) +
		    (b - gPalette[i].blue) * (b - gPalette[i].blue);
		if (best < 0 || d < dMin) {
			best = i;
			dMin = d;
		}
	}
	return best;
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
	SetCharacterColors(&characterDesc[index].table, arms, body, legs,
			   skin, hair);
}

void BuildTranslationTables(void)
{
	int i, f;

	for (i = 0; i < 256; i++) {
		f = 0.3 * gPalette[i].red + 0.59 * gPalette[i].green +
		    0.11 * gPalette[i].blue;
		tableFlamed[i] = BestMatch(f, 0, 0);
	}
	for (i = 0; i < 256; i++) {
		f = 0.4 * gPalette[i].red + 0.49 * gPalette[i].green +
		    0.11 * gPalette[i].blue;
		tableGreen[i] = BestMatch(0, 2 * f / 3, 0);
	}
	for (i = 0; i < 256; i++) {
		tablePoison[i] = BestMatch(gPalette[i].red + 5,
					   gPalette[i].green + 15,
					   gPalette[i].blue + 5);
	}
	for (i = 0; i < 256; i++) {
		f = 0.4 * gPalette[i].red + 0.49 * gPalette[i].green +
		    0.11 * gPalette[i].blue;
		tableGray[i] = BestMatch(f, f, f);
	}
	for (i = 0; i < 256; i++) {
		tableBlack[i] = BestMatch(0, 0, 0);
	}
	for (i = 0; i < 256; i++) {
		f = 0.4 * gPalette[i].red + 0.49 * gPalette[i].green +
		    0.11 * gPalette[i].blue;
		tablePurple[i] = BestMatch(f, 0, f);
	}
	for (i = 0; i < 256; i++) {
		tableDarker[i] = BestMatch((200 * gPalette[i].red) / 256,
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
