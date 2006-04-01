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

 objs.c - object functions (fun) 

*/

#include <string.h>
#include <stdlib.h>
#include "map.h"
#include "blit.h"
#include "pics.h"
#include "defs.h"
#include "objs.h"
#include "sounds.h"
#include "actors.h"
#include "gamedata.h"
#include "mission.h"
#include "game.h"


#define FLAMED_COUNT        10
#define POISONED_COUNT       8
#define MAX_POISONED_COUNT 140
#define PETRIFIED_COUNT     95
#define CONFUSED_COUNT     700


static TMobileObject *mobObjList = NULL;
static TObject *objList = NULL;

void Fire(int x, int y, int flags);
void Gas(int x, int y, int flags, int special);
int HitItem(TMobileObject * obj, int x, int y, int special);


// Draw functions

void DrawObject(int x, int y, const TObject * obj)
{
	TOffsetPic *pic = obj->pic;

	if (pic)
		DrawTPic(x + pic->dx,
			 y + pic->dy,
			 gPics[pic->picIndex],
			 gCompiledPics[pic->picIndex]);
}

/*
void DrawPuzzlePiece( int x, int y, const TObject *obj )
{
  TOffsetPic *pic = obj->pic;
  int w, h;

  if (pic)
  {
    w = PicWidth( gPics[ pic->picIndex]);
    h = PicHeight( gPics[ pic->picIndex]);

    DrawTTPic( x - w/2 + 1,
               y - h/2 + 1,
               gPics[ pic->picIndex],
               tableBlack);
    DrawTPic( x - w/2,
              y - h/2,
              gPics[ pic->picIndex],
              gCompiledPics[ pic->picIndex]);
  }
}
*/

void DrawBullet(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_BULLET];
	DrawTPic(x + pic->dx,
		 y + pic->dy - obj->z,
		 gPics[pic->picIndex], gCompiledPics[pic->picIndex]);
}

void DrawBrownBullet(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_SNIPERBULLET];
	DrawTPic(x + pic->dx,
		 y + pic->dy - obj->z,
		 gPics[pic->picIndex], gCompiledPics[pic->picIndex]);
}

void DrawPetrifierBullet(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_MOLOTOV];
	DrawBTPic(x + pic->dx,
		  y + pic->dy - obj->z,
		  gPics[pic->picIndex],
		  tableDarker, gRLEPics[pic->picIndex]);
}

void DrawSeeker(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_SNIPERBULLET];
	DrawTTPic(x + pic->dx,
		  y + pic->dy - obj->z,
		  gPics[pic->picIndex],
		  tableFlamed, gRLEPics[pic->picIndex]);
}

void DrawMine(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_MINE];
	DrawTPic(x + pic->dx,
		 y + pic->dy - obj->z,
		 gPics[pic->picIndex], gCompiledPics[pic->picIndex]);
}

void DrawDynamite(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_DYNAMITE];
	DrawTPic(x + pic->dx,
		 y + pic->dy - obj->z,
		 gPics[pic->picIndex], gCompiledPics[pic->picIndex]);
}

void DrawMolotov(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_MOLOTOV];
	if (obj->z > 0)
		y -= obj->z / 16;
	DrawTPic(x + pic->dx,
		 y + pic->dy,
		 gPics[pic->picIndex], gCompiledPics[pic->picIndex]);
}

void DrawFlame(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cFlamePics[obj->state & 3];
	DrawTPic(x + pic->dx,
		 y + pic->dy - obj->z,
		 gPics[pic->picIndex], gCompiledPics[pic->picIndex]);
}

void DrawLaserBolt(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cBeamPics[obj->state];
	DrawTPic(x + pic->dx,
		 y + pic->dy - obj->z,
		 gPics[pic->picIndex], gCompiledPics[pic->picIndex]);
}

void DrawBrightBolt(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cBrightBeamPics[obj->state];
	DrawTPic(x + pic->dx,
		 y + pic->dy - obj->z,
		 gPics[pic->picIndex], gCompiledPics[pic->picIndex]);
}

void DrawSpark(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_SPARK];
	DrawTPic(x + pic->dx,
		 y + pic->dy - obj->z,
		 gPics[pic->picIndex], gCompiledPics[pic->picIndex]);
}

void DrawGrenade(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGrenadePics[(obj->count / 2) & 3];
	if (obj->z > 0)
		y -= obj->z / 16;
	DrawTPic(x + pic->dx,
		 y + pic->dy,
		 gPics[pic->picIndex], gCompiledPics[pic->picIndex]);
}

void DrawGasCloud(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cFireBallPics[8 + (obj->state & 3)];
	DrawBTPic(x + pic->dx,
		  y + pic->dy,
		  gPics[pic->picIndex],
		  obj->z ? tablePurple : tablePoison,
		  gRLEPics[pic->picIndex]);
}

void DrawFireball(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	if (obj->count < obj->state)
		return;
	pic = &cFireBallPics[(obj->count - obj->state) / 4];
	if (obj->z > 0)
		y -= obj->z / 4;
	DrawTPic(x + pic->dx,
		 y + pic->dy,
		 gPics[pic->picIndex], gCompiledPics[pic->picIndex]);
}

void BogusDraw(int x, int y, void *data)
{
	if (data)
		x = y;		// Just to get rid of warning...
}


// Prototype this as it is used by DamageSomething()
TMobileObject *AddFireBall(int flags);


static void TrackKills(TActor * victim, int flags)
{
	if (flags & FLAGS_PLAYER1) {
		if ((victim->
		     flags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY |
			      FLAGS_PENALTY)) != 0)
			gPlayer1Data.friendlies++;
		else
			gPlayer1Data.kills++;
	} else if (flags & FLAGS_PLAYER2) {
		if ((victim->
		     flags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY |
			      FLAGS_PENALTY)) != 0)
			gPlayer2Data.friendlies++;
		else
			gPlayer2Data.kills++;
	}
}

// The damage function!

int DamageSomething(int dx, int dy, int power, int flags,
		    TTileItem * target, int special)
{
	TActor *actor;
	TObject *object;

	if (!target)
		return NO;

	switch (target->kind) {
	case KIND_CHARACTER:
		actor = target->data;

		if ((flags & FLAGS_HURTALWAYS) == 0 &&
		    (flags & FLAGS_PLAYERS) != 0 &&
		    (flags & FLAGS_PLAYERS) ==
		    (actor->flags & FLAGS_PLAYERS))
			return NO;

		if (special == SPECIAL_FLAME
		    && (actor->flags & FLAGS_ASBESTOS) != 0)
			break;
		if (special == SPECIAL_POISON
		    && (actor->flags & FLAGS_IMMUNITY) != 0)
			break;
		if (special == SPECIAL_CONFUSE
		    && (actor->flags & FLAGS_IMMUNITY) != 0)
			break;

		if (actor->health > 0) {
			if (special == SPECIAL_FLAME)
				actor->flamed = FLAMED_COUNT;
			else if (special == SPECIAL_POISON) {
				if (actor->poisoned < MAX_POISONED_COUNT)
					actor->poisoned += POISONED_COUNT;
			} else if (special == SPECIAL_PETRIFY
				   && !actor->petrified)
				actor->petrified = PETRIFIED_COUNT;
			else if (special == SPECIAL_CONFUSE)
				actor->confused = CONFUSED_COUNT;

			actor->dx += (power * dx) / 25;
			actor->dy += (power * dy) / 25;

			if ((actor->flags & FLAGS_INVULNERABLE) != 0)
				break;

			if ((flags & FLAGS_HURTALWAYS) == 0
			    && (actor->flags & FLAGS_VICTIM) == 0) {
				if ((flags & FLAGS_PLAYERS & actor->
				     flags) != 0)
					break;
				if (!gCampaign.dogFight &&
				    !gOptions.playersHurt &&
				    (flags &
				     (FLAGS_PLAYERS | FLAGS_GOOD_GUY)) != 0
				    && (actor->
					flags & (FLAGS_PLAYERS |
						 FLAGS_GOOD_GUY)) != 0)
					break;
				if ((flags &
				     (FLAGS_PLAYERS | FLAGS_GOOD_GUY)) == 0
				    && (actor->
					flags & (FLAGS_PLAYERS |
						 FLAGS_GOOD_GUY)) == 0)
					break;
			}
			InjureActor(actor, power);
			if (actor->health <= 0)
				TrackKills(actor, flags);
			if ((flags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY)) != 0
			    &&
			    ((actor->
			      flags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY)) ==
			     0)) {
				if ((actor->flags & FLAGS_PENALTY) != 0)
					Score(flags, -3 * power);
				else
					Score(flags, power);
			}
		}
		break;

	case KIND_OBJECT:
		object = target->data;
		if (object->structure > 0) {
			object->structure -= power;
			if (object->structure <= 0) {
				if (CheckMissionObjective
				    (object->tileItem.flags))
					Score(flags, 50);
				if (object->flags & OBJFLAG_QUAKE)
					ShakeScreen(70);
				if (object->flags & OBJFLAG_EXPLOSIVE){
					AddExplosion(object->tileItem.
						     x << 8,
						     object->tileItem.
						     y << 8, flags);
				} else if (object->flags & OBJFLAG_FLAMMABLE) {
					Fire(object->tileItem.x << 8,
					     object->tileItem.y << 8,
					     flags);
				} else if (object->flags & OBJFLAG_POISONOUS)
					Gas(object->tileItem.x << 8,
					    object->tileItem.y << 8, flags,
					    SPECIAL_POISON);
				else if (object->flags & OBJFLAG_CONFUSING)
					Gas(object->tileItem.x << 8,
					    object->tileItem.y << 8, flags,
					    SPECIAL_CONFUSE);
				else {
					TMobileObject *obj =
					    AddFireBall(0);
					obj->count = 10;
					obj->power = 0;
					obj->x = object->tileItem.x << 8;
					obj->y = object->tileItem.y << 8;
					PlaySoundAt(object->tileItem.x,
						    object->tileItem.y,
						    SND_BANG);
				}
				if (object->wreckedPic) {
					object->structure = 0;
					object->tileItem.flags = 0;
					object->pic = object->wreckedPic;
				} else
					RemoveObject(object);
			}
		}
		break;

	case KIND_PIC:
	case KIND_MOBILEOBJECT:
		break;
	}
	return YES;
}


// Update functions


int UpdateMobileObject(TMobileObject * obj)
{
	obj->count++;
	if (obj->count > obj->range)
		return 0;
	return 1;
}

void Frag(int x, int y, int flags)
{
	int i;

	flags |= FLAGS_HURTALWAYS;
	for (i = 0; i < 16; i++)
		AddBullet(x, y, i * 16, SHOTGUN_SPEED, SHOTGUN_RANGE, 40,
			  flags);
	PlaySoundAt(x >> 8, y >> 8, SND_BANG);
}

int UpdateMolotovFlame(TMobileObject * obj)
{
	int x, y;

	obj->count++;
	if (obj->count > obj->range)
		return 0;

	if ((obj->count & 3) == 0)
		obj->state = rand();

	obj->z += obj->dz / 2;
	if (obj->z <= 0)
		obj->z = 0;
	else
		obj->dz--;

	x = obj->x + obj->dx;
	y = obj->y + obj->dy;

	if (obj->dx > 0)
		obj->dx -= 4;
	else if (obj->dx < 0)
		obj->dx += 4;

	if (obj->dy > 0)
		obj->dy -= 3;
	else if (obj->dy < 0)
		obj->dy += 3;

	HitItem(obj, x, y, SPECIAL_FLAME);

	if (!HitWall(x >> 8, y >> 8)) {
		obj->x = x;
		obj->y = y;
		MoveTileItem(&obj->tileItem, x >> 8, y >> 8);
		return 1;
	} else
		return 1;
}

// Prototype to simplify matters (I don't want this in the .h file)
TMobileObject *AddMobileObject(void);

TMobileObject *AddMolotovFlame(int x, int y, int flags)
{
	TMobileObject *obj;

	obj = AddMobileObject();
	obj->updateFunc = UpdateMolotovFlame;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawFlame;
	obj->tileItem.w = 5;
	obj->tileItem.h = 5;
	obj->kind = MOBOBJ_FIREBALL;
	obj->range = (FLAME_RANGE + rand() % 8) * 4;
	obj->flags = flags;
	obj->power = 2;
	obj->x = x;
	obj->y = y;
	obj->dx = 16 * (rand() % 32) - 256;
	obj->dy = 12 * (rand() % 32) - 192;
	obj->dz = 4 + rand() % 4;
	return obj;
}

void Fire(int x, int y, int flags)
{
	int i;

	flags |= FLAGS_HURTALWAYS;
	for (i = 0; i < 16; i++)
		AddMolotovFlame(x, y, flags);
	PlaySoundAt(x >> 8, y >> 8, SND_BANG);
}

int UpdateGasCloud(TMobileObject * obj)
{
	int x, y;

	obj->count++;
	if (obj->count > obj->range)
		return 0;

	if ((obj->count & 3) == 0)
		obj->state = rand();

	x = obj->x + obj->dx;
	y = obj->y + obj->dy;

	if (obj->dx > 0)
		obj->dx -= 4;
	else if (obj->dx < 0)
		obj->dx += 4;

	if (obj->dy > 0)
		obj->dy -= 3;
	else if (obj->dy < 0)
		obj->dy += 3;

	HitItem(obj, x, y, obj->z ? SPECIAL_CONFUSE : SPECIAL_POISON);

	if (!HitWall(x >> 8, y >> 8)) {
		obj->x = x;
		obj->y = y;
		MoveTileItem(&obj->tileItem, x >> 8, y >> 8);
		return 1;
	} else
		return 1;
}

void AddGasCloud(int x, int y, int angle, int speed, int range, int flags,
		 int special)
{
	TMobileObject *obj;

	obj = AddMobileObject();
	obj->updateFunc = UpdateGasCloud;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawGasCloud;
	obj->tileItem.w = 10;
	obj->tileItem.h = 10;
	obj->kind = MOBOBJ_FIREBALL;
	obj->range = range;
	obj->flags = flags;
	obj->power = 0;
	obj->z = (special == SPECIAL_CONFUSE);
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->dx = (speed * obj->dx) / 256;
	obj->dy = (speed * obj->dy) / 256;
	obj->x = x + 6 * obj->dx;
	obj->y = y + 6 * obj->dy;
}

void Gas(int x, int y, int flags, int special)
{
	int i;

	flags |= FLAGS_HURTALWAYS;
	for (i = 0; i < 8; i++)
		AddGasCloud(x, y, rand() & 255, (256 + rand()) & 255,
			    ((48 - rand()) % 8) * 4 - 1, flags, special);
	PlaySoundAt(x >> 8, y >> 8, SND_BANG);
}

int UpdateGrenade(TMobileObject * obj)
{
	int x, y;

	obj->count++;
	if (obj->count > obj->range) {
		switch (obj->kind) {
		case MOBOBJ_GRENADE:
			AddExplosion(obj->x, obj->y, obj->flags);
			break;

		case MOBOBJ_FRAGGRENADE:
			Frag(obj->x, obj->y, obj->flags);
			break;

		case MOBOBJ_MOLOTOV:
			Fire(obj->x, obj->y, obj->flags);
			break;

		case MOBOBJ_GASBOMB:
			Gas(obj->x, obj->y, obj->flags, SPECIAL_POISON);
			break;

		case MOBOBJ_GASBOMB2:
			Gas(obj->x, obj->y, obj->flags, SPECIAL_CONFUSE);
			break;
		}
		return 0;
	}

	x = obj->x + obj->dx;
	y = obj->y + obj->dy;

	obj->z += obj->dz;
	if (obj->z <= 0) {
		obj->z = 0;
		obj->dz = -obj->dz / 2;
	} else
		obj->dz--;

	if (!HitWall(x >> 8, y >> 8)) {
		obj->x = x;
		obj->y = y;
	} else if (!HitWall(obj->x >> 8, y >> 8)) {
		obj->y = y;
		obj->dx = -obj->dx;
	} else if (!HitWall(x >> 8, obj->y >> 8)) {
		obj->x = x;
		obj->dy = -obj->dy;
	} else {
		obj->dx = -obj->dx;
		obj->dy = -obj->dy;
		return 1;
	}
	MoveTileItem(&obj->tileItem, obj->x >> 8, obj->y >> 8);
	return 1;
}

int UpdateMolotov(TMobileObject * obj)
{
	int x, y;

	obj->count++;
	if (obj->count > obj->range) {
		Fire(obj->x, obj->y, obj->flags);
		return 0;
	}

	x = obj->x + obj->dx;
	y = obj->y + obj->dy;

	obj->z += obj->dz;
	if (obj->z <= 0) {
		Fire(obj->x, obj->y, obj->flags);
		return 0;
	} else
		obj->dz--;

	if (!HitWall(x >> 8, y >> 8)) {
		obj->x = x;
		obj->y = y;
	} else {
		Fire(obj->x, obj->y, obj->flags);
		return 0;
	}
	MoveTileItem(&obj->tileItem, obj->x >> 8, obj->y >> 8);
	return 1;
}

int UpdateSpark(TMobileObject * obj)
{
	obj->count++;
	if (obj->count > obj->range)
		return 0;
	return 1;
}

int HitItem(TMobileObject * obj, int x, int y, int special)
{
	TTileItem *tile;

	tile =
	    CheckTileItemCollision(&obj->tileItem, x >> 8, y >> 8,
				   TILEITEM_CAN_BE_SHOT);
	return DamageSomething(obj->dx, obj->dy, obj->power, obj->flags,
			       tile, special);
}

int InternalUpdateBullet(TMobileObject * obj, int special)
{
	int x, y;

	obj->count++;
	if (obj->count > obj->range)
		return 0;

	x = obj->x + obj->dx;
	y = obj->y + obj->dy;

	if (HitItem(obj, x, y, special)) {
		obj->count = 0;
		obj->range = 0;
		obj->tileItem.drawFunc = (TileItemDrawFunc)DrawSpark;
		obj->updateFunc = UpdateSpark;
		return 1;
	}
	if (!HitWall(x >> 8, y >> 8)) {
		obj->x = x;
		obj->y = y;
		MoveTileItem(&obj->tileItem, x >> 8, y >> 8);
		return 1;
	} else {
		obj->count = 0;
		obj->range = 0;
		obj->tileItem.drawFunc = (TileItemDrawFunc)DrawSpark;
		obj->updateFunc = UpdateSpark;
		return 1;
	}
}

int UpdateBullet(TMobileObject * obj)
{
	return InternalUpdateBullet(obj, 0);
}

int UpdatePetrifierBullet(TMobileObject * obj)
{
	return InternalUpdateBullet(obj, SPECIAL_PETRIFY);
}

int UpdateSeeker(TMobileObject * obj)
{
	return InternalUpdateBullet(obj, 0);
}

int UpdateBrownBullet(TMobileObject * obj)
{
	if (InternalUpdateBullet(obj, 0)) {
		obj->dx += ((rand() % 3) - 1) * 128;
		obj->dy += ((rand() % 3) - 1) * 128;
		return 1;
	}
	return 0;
}

int UpdateTriggeredMine(TMobileObject * obj)
{
	obj->count++;
	if (obj->count >= obj->range) {
		AddExplosion(obj->x, obj->y, obj->flags);
		return 0;
	}
	return 1;
}

int UpdateActiveMine(TMobileObject * obj)
{
	TTileItem *i;
	int tx, ty, dx, dy;
	TTile *tile;

	obj->count++;
	if ((obj->count & 3) != 0)
		return 1;

	tx = (obj->x >> 8) / TILE_WIDTH;
	ty = (obj->y >> 8) / TILE_HEIGHT;

	if (tx == 0 || ty == 0 || tx >= XMAX - 1 || ty >= YMAX - 1)
		//return NULL;
		return 0;

	for (dy = -1; dy <= 1; dy++)
		for (dx = -1; dx <= 1; dx++) {
			tile = &Map(tx + dx, ty + dy);
			i = tile->things;
			while (i) {
				if (i->kind == KIND_CHARACTER) {
					obj->updateFunc =
					    UpdateTriggeredMine;
					obj->count = 0;
					obj->range = 5;
					PlaySoundAt(obj->tileItem.x,
						    obj->tileItem.y,
						    SND_HAHAHA);
					return 1;
				}
				i = i->next;
			}
		}
	return 1;
}

int UpdateDroppedMine(TMobileObject * obj)
{
	obj->count++;
	if (obj->count >= obj->range)
		obj->updateFunc = UpdateActiveMine;
	return 1;
}

int UpdateFlame(TMobileObject * obj)
{
	int x, y;

	obj->count++;
	if (obj->count > obj->range)
		return 0;

	if ((obj->count & 3) == 0)
		obj->state = rand();

	x = obj->x + obj->dx;
	y = obj->y + obj->dy;

	if (HitItem(obj, x, y, SPECIAL_FLAME)) {
		obj->count = obj->range;
		obj->x = x;
		obj->y = y;
		MoveTileItem(&obj->tileItem, x >> 8, y >> 8);
		return 1;
	}

	if (!HitWall(x >> 8, y >> 8)) {
		obj->x = x;
		obj->y = y;
		MoveTileItem(&obj->tileItem, x >> 8, y >> 8);
		return 1;
	} else
		return 0;
}

int UpdateExplosion(TMobileObject * obj)
{
	int x, y;

	obj->count++;
	if (obj->count < 0)
		return 1;
	if (obj->count > obj->range)
		return 0;

	x = obj->x + obj->dx;
	y = obj->y + obj->dy;
	obj->z += obj->dz;
	obj->dz--;

	HitItem(obj, x, y, 0);

	if (!HitWall(x >> 8, y >> 8)) {
		obj->x = x;
		obj->y = y;
		MoveTileItem(&obj->tileItem, x >> 8, y >> 8);
		return 1;
	}
	return 0;
}

void UpdateMobileObjects(void)
{
	TMobileObject *obj = mobObjList;
	int remove = 0;

	while (obj) {
		if ((*(obj->updateFunc)) (obj) == 0) {
			obj->range = 0;
			remove = 1;
		}
		obj = obj->next;
	}
	if (remove) {
		TMobileObject **h = &mobObjList;

		while (*h) {
			if ((*h)->range == 0) {
				obj = *h;
				*h = obj->next;
				RemoveTileItem(&obj->tileItem);
				free(obj);
			} else
				h = &((*h)->next);
		}
	}
}

TMobileObject *AddMobileObject(void)
{
	TMobileObject *obj = malloc(sizeof(TMobileObject));

	memset(obj, 0, sizeof(TMobileObject));
	obj->tileItem.kind = KIND_MOBILEOBJECT;
	obj->tileItem.data = obj;
	obj->next = mobObjList;
	obj->tileItem.drawFunc = (TileItemDrawFunc)BogusDraw;
	obj->updateFunc = UpdateMobileObject;
	mobObjList = obj;
	return obj;
}

void AddGrenade(int x, int y, int angle, int flags, int kind)
{
	TMobileObject *obj;

	obj = AddMobileObject();
	if (kind == MOBOBJ_MOLOTOV) {
		obj->updateFunc = UpdateMolotov;
		obj->tileItem.drawFunc = (TileItemDrawFunc)DrawMolotov;
	} else {
		obj->updateFunc = UpdateGrenade;
		obj->tileItem.drawFunc = (TileItemDrawFunc)DrawGrenade;
	}
	obj->kind = kind;
	obj->x = x;
	obj->y = y;
	obj->z = 0;
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->dx = (GRENADE_SPEED * obj->dx) / 256;
	obj->dy = (GRENADE_SPEED * obj->dy) / 256;
	obj->dz = 24;
	obj->range = 100;
	obj->flags = flags;
}

void AddBullet(int x, int y, int angle, int speed, int range, int power,
	       int flags)
{
	TMobileObject *obj;

	obj = AddMobileObject();
	obj->updateFunc = UpdateBullet;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawBullet;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->dx = (speed * obj->dx) / 256;
	obj->dy = (speed * obj->dy) / 256;
	obj->x = x;
	obj->y = y;
	obj->range = range;
	obj->power = power;
	obj->flags = flags;
}

void AddRapidBullet(int x, int y, int angle, int speed, int range,
		    int power, int flags)
{
	TMobileObject *obj;

	obj = AddMobileObject();
	obj->updateFunc = UpdateBullet;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawBrownBullet;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->dx = (speed * obj->dx) / 256;
	obj->dy = (speed * obj->dy) / 256;
	obj->x = x;
	obj->y = y;
	obj->range = range;
	obj->power = power;
	obj->flags = flags;
}

void AddSniperBullet(int x, int y, int direction, int flags)
{
	TMobileObject *obj;

	obj = AddMobileObject();
	obj->updateFunc = UpdateBullet;
	obj->tileItem.drawFunc =  (TileItemDrawFunc)DrawBrightBolt;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(dir2angle[direction], &obj->dx, &obj->dy);
	obj->dx = (SNIPER_SPEED * obj->dx) / 256;
	obj->dy = (SNIPER_SPEED * obj->dy) / 256;
	obj->x = x;
	obj->y = y;
	obj->state = direction;
	obj->range = SNIPER_RANGE;
	obj->power = SNIPER_POWER;
	obj->flags = flags;
}

void AddBrownBullet(int x, int y, int angle, int speed, int range,
		    int power, int flags)
{
	TMobileObject *obj;

	obj = AddMobileObject();
	obj->updateFunc = UpdateBrownBullet;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawBrownBullet;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->dx = (speed * obj->dx) / 256;
	obj->dy = (speed * obj->dy) / 256;
	obj->x = x;
	obj->y = y;
	obj->range = range;
	obj->power = power;
	obj->flags = flags;
}

void AddFlame(int x, int y, int angle, int flags)
{
	TMobileObject *obj;

	obj = AddMobileObject();
	obj->updateFunc = UpdateFlame;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawFlame;
	obj->tileItem.w = 5;
	obj->tileItem.h = 5;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->x = x + 4 * obj->dx;
	obj->y = y + 7 * obj->dy;
	obj->dx = (FLAME_SPEED * obj->dx) / 256;
	obj->dy = (FLAME_SPEED * obj->dy) / 256;
	obj->range = FLAME_RANGE;
	obj->power = FLAME_POWER;
	obj->flags = flags;
}

void AddLaserBolt(int x, int y, int direction, int flags)
{
	TMobileObject *obj;

	obj = AddMobileObject();
	obj->updateFunc = UpdateBullet;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawLaserBolt;
	obj->tileItem.w = 2;
	obj->tileItem.h = 2;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(dir2angle[direction], &obj->dx, &obj->dy);
	obj->dx = (LASER_SPEED * obj->dx) / 256;
	obj->dy = (LASER_SPEED * obj->dy) / 256;
	obj->x = x;
	obj->y = y;
	obj->range = LASER_RANGE;
	obj->state = direction;
	obj->flags = flags;
	obj->power = LASER_POWER;
}

void AddPetrifierBullet(int x, int y, int angle, int speed, int range,
			int flags)
{
	TMobileObject *obj;

	obj = AddMobileObject();
	obj->updateFunc = UpdatePetrifierBullet;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawPetrifierBullet;
	obj->tileItem.w = 5;
	obj->tileItem.h = 5;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->dx = (speed * obj->dx) / 256;
	obj->dy = (speed * obj->dy) / 256;
	obj->x = x + 4 * obj->dx;
	obj->y = y + 7 * obj->dy;
	obj->range = range;
	obj->flags = flags;
	obj->power = 0;
}

void AddHeatseeker(int x, int y, int angle, int speed, int range,
		   int power, int flags)
{
	TMobileObject *obj;

	obj = AddMobileObject();
	obj->updateFunc = UpdateSeeker;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawSeeker;
	obj->tileItem.w = 3;
	obj->tileItem.h = 3;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->dx = (speed * obj->dx) / 256;
	obj->dy = (speed * obj->dy) / 256;
	obj->dz = speed;
	obj->x = x;
	obj->y = y;
	obj->range = range;
	obj->flags = flags;
	obj->power = power;
}

void AddProximityMine(int x, int y, int flags)
{
	TMobileObject *obj;

	obj = AddMobileObject();
	obj->updateFunc = UpdateDroppedMine;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawMine;
	obj->kind = MOBOBJ_BULLET;
	obj->x = x;
	obj->y = y;
	obj->range = 140;
	obj->flags = flags;
	MoveTileItem(&obj->tileItem, obj->x >> 8, obj->y >> 8);
}

void AddDynamite(int x, int y, int flags)
{
	TMobileObject *obj;

	obj = AddMobileObject();
	obj->updateFunc = UpdateTriggeredMine;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawDynamite;
	obj->kind = MOBOBJ_BULLET;
	obj->x = x;
	obj->y = y;
	obj->range = 210;
	obj->flags = flags;
	MoveTileItem(&obj->tileItem, obj->x >> 8, obj->y >> 8);
}

TMobileObject *AddFireBall(int flags)
{
	TMobileObject *obj;

	obj = AddMobileObject();
	obj->updateFunc = UpdateExplosion;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawFireball;
	obj->tileItem.w = 7;
	obj->tileItem.h = 5;
	obj->kind = MOBOBJ_FIREBALL;
	obj->range = FIREBALL_MAX * 4 - 1;
	obj->flags = flags;
	obj->power = FIREBALL_POWER;
	return obj;
}

void AddExplosion(int x, int y, int flags)
{
	TMobileObject *obj;
	int i;

	flags |= FLAGS_HURTALWAYS;
	for (i = 0; i < 8; i++) {
		obj = AddFireBall(flags);
		GetVectorsForAngle(i * 32, &obj->dx, &obj->dy);
		obj->x = x + 2 * obj->dx;
		obj->y = y + 2 * obj->dy;
		obj->dz = 0;
	}
	for (i = 0; i < 8; i++) {
		obj = AddFireBall(flags);
		GetVectorsForAngle(i * 32 + 16, &obj->dx, &obj->dy);
		obj->x = x + obj->dx;
		obj->y = y + obj->dy;
		obj->dx *= 3;
		obj->dy *= 3;
		obj->dx /= 4;
		obj->dy /= 4;
		obj->dz = 8;
		obj->count = -8;
	}
	for (i = 0; i < 8; i++) {
		obj = AddFireBall(flags);
		obj->x = x;
		obj->y = y;
		obj->z = 0;
		GetVectorsForAngle(i * 32, &obj->dx, &obj->dy);
		obj->dx /= 2;
		obj->dy /= 2;
		obj->dz = 11;
		obj->count = -16;
	}
	ShakeScreen(15);
	PlaySoundAt(x >> 8, y >> 8, SND_EXPLOSION);
}

void KillAllMobileObjects(void)
{
	TMobileObject *o;

	while (mobObjList) {
		o = mobObjList;
		mobObjList = mobObjList->next;
		RemoveTileItem(&o->tileItem);
		free(o);
	}
}

void InternalAddObject(int x, int y, int w, int h,
		       TOffsetPic * pic, TOffsetPic * wreckedPic,
		       int structure, int index, int objFlags,
		       int tileFlags)
{
	TObject *o = malloc(sizeof(TObject));
	memset(o, 0, sizeof(TObject));
	o->pic = pic;
	o->wreckedPic = wreckedPic;
	o->objectIndex = index;
	o->structure = structure;
	o->flags = objFlags;
	o->tileItem.flags = tileFlags;
	o->tileItem.kind = KIND_OBJECT;
	o->tileItem.data = o;
	o->tileItem.drawFunc = (TileItemDrawFunc)DrawObject;
	o->tileItem.w = w;
	o->tileItem.h = h;
	MoveTileItem(&o->tileItem, x >> 8, y >> 8);
	o->next = objList;
	objList = o;
}

void AddObject(int x, int y, int w, int h,
	       TOffsetPic * pic, int index, int tileFlags)
{
	InternalAddObject(x, y, w, h, pic, NULL, 0, index, 0, tileFlags);
}

void AddDestructibleObject(int x, int y, int w, int h,
			   TOffsetPic * pic, TOffsetPic * wreckedPic,
			   int structure, int objFlags, int tileFlags)
{
	InternalAddObject(x, y, w, h, pic, wreckedPic, structure, 0,
			  objFlags, tileFlags);
}

void RemoveObject(TObject * obj)
{
	TObject **h = &objList;

	while (*h && *h != obj)
		h = &((*h)->next);
	if (*h) {
		*h = obj->next;
		RemoveTileItem(&obj->tileItem);
		free(obj);
	}
}

void KillAllObjects(void)
{
	TObject *o;

	while (objList) {
		o = objList;
		objList = objList->next;
		RemoveTileItem(&o->tileItem);
		free(o);
	}
}
