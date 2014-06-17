/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014, Cong Xu
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
#include "particle.h"

#include "collision.h"
#include "game_events.h"
#include "json_utils.h"


CArray gParticleClasses;
CArray gParticles;

#define VERSION 1

static void LoadParticleClass(ParticleClass *c, json_t *node);
void ParticleClassesInit(CArray *classes, const char *filename)
{
	CArrayInit(classes, sizeof(ParticleClass));

	FILE *f = fopen(filename, "r");
	json_t *root = NULL;
	if (f == NULL)
	{
		printf("Error: cannot load particles file %s\n", filename);
		goto bail;
	}
	enum json_error e = json_stream_parse(f, &root);
	if (e != JSON_OK)
	{
		printf("Error parsing particles file %s\n", filename);
		goto bail;
	}
	int version;
	LoadInt(&version, root, "Version");
	if (version > VERSION || version <= 0)
	{
		CASSERT(false, "cannot read particles file version");
		goto bail;
	}

	json_t *particlesNode = json_find_first_label(root, "Particles")->child;
	for (json_t *child = particlesNode->child; child; child = child->next)
	{
		ParticleClass c;
		LoadParticleClass(&c, child);
		CArrayPushBack(classes, &c);
	}

bail:
	json_free_value(&root);
	if (f)
	{
		fclose(f);
	}
}
void ParticleClassesTerminate(CArray *classes)
{
	for (int i = 0; i < (int)classes->size; i++)
	{
		ParticleClass *c = CArrayGet(classes, i);
		CFREE(c->Name);
	}
	CArrayTerminate(classes);
}
static void LoadParticleClass(ParticleClass *c, json_t *node)
{
	memset(c, 0, sizeof *c);
	c->Mask = colorWhite;
	char *tmp;

	c->Name = GetString(node, "Name");
	if (json_find_first_label(node, "Sprites"))
	{
		tmp = GetString(node, "Sprites");
		c->Sprites = PicManagerGetSprites(&gPicManager, tmp);
		CFREE(tmp);
	}
	if (json_find_first_label(node, "Pic"))
	{
		tmp = GetString(node, "Pic");
		c->Pic = PicManagerGetPic(&gPicManager, tmp);
		CFREE(tmp);
	}
	if (c->Pic == NULL && json_find_first_label(node, "OldPic"))
	{
		int oldPic;
		LoadInt(&oldPic, node, "OldPic");
		c->Pic = PicManagerGetFromOld(&gPicManager, oldPic);
	}
	if (json_find_first_label(node, "Mask"))
	{
		tmp = GetString(node, "Mask");
		c->Mask = StrColor(tmp);
		CFREE(tmp);
	}
	if (json_find_first_label(node, "Range"))
	{
		LoadInt(&c->RangeLow, node, "Range");
		c->RangeHigh = c->RangeLow;
	}
	if (json_find_first_label(node, "RangeLow"))
	{
		LoadInt(&c->RangeLow, node, "RangeLow");
	}
	if (json_find_first_label(node, "RangeHigh"))
	{
		LoadInt(&c->RangeHigh, node, "RangeHigh");
	}
	c->RangeLow = MIN(c->RangeLow, c->RangeHigh);
	c->RangeHigh = MAX(c->RangeLow, c->RangeHigh);
	LoadBool(&c->Falling, node, "Falling");
	LoadBool(&c->HitsWalls, node, "HitsWalls");
}

const ParticleClass *ParticleClassGet(const CArray *classes, const char *name)
{
	if (name == NULL || strlen(name) == 0)
	{
		return NULL;
	}
	for (int i = 0; i < (int)classes->size; i++)
	{
		ParticleClass *c = CArrayGet(classes, i);
		if (strcmp(c->Name, name) == 0)
		{
			return c;
		}
	}
	CASSERT(false, "Cannot find particle class");
	return NULL;
}

void ParticlesInit(CArray *particles)
{
	CArrayInit(particles, sizeof(Particle));
	CArrayReserve(particles, 256);
}
void ParticlesTerminate(CArray *particles)
{
	for (int i = 0; i < (int)particles->size; i++)
	{
		Particle *p = CArrayGet(particles, i);
		if (p->isInUse)
		{
			ParticleDestroy(particles, i);
		}
	}
	CArrayTerminate(particles);
}

static bool ParticleUpdate(Particle *p, const int ticks);
void ParticlesUpdate(CArray *particles, const int ticks)
{
	for (int i = 0; i < (int)particles->size; i++)
	{
		Particle *p = CArrayGet(particles, i);
		if (!p->isInUse)
		{
			continue;
		}
		if (!ParticleUpdate(p, ticks))
		{
			GameEvent e;
			e.Type = GAME_EVENT_PARTICLE_REMOVE;
			e.u.ParticleRemoveId = i;
			GameEventsEnqueue(&gGameEvents, e);
		}
	}
}


static bool ParticleUpdate(Particle *p, const int ticks)
{
	p->Count += ticks;
	const Vec2i startPos = p->Pos;
	for (int i = 0; i < ticks; i++)
	{
		p->Pos = Vec2iAdd(p->Pos, p->Vel);
		p->Z += p->DZ;
		if (p->Class->Falling)
		{
			if (p->Z <= 0)
			{
				p->Z = 0;
				p->DZ = -p->DZ / 2;
			}
			else
			{
				p->DZ--;
			}
			if (p->DZ == 0 && p->Z == 0)
			{
				p->Vel = Vec2iZero();
				p->Spin = 0;
			}
		}
	}
	if (p->Class->HitsWalls)
	{
		p->Pos = GetWallBounceFullPos(startPos, p->Pos, &p->Vel);
	}
	const Vec2i realPos = Vec2iFull2Real(p->Pos);
	if (!MapIsTileIn(&gMap, Vec2iToTile(realPos)))
	{
		// Out of map; destroy
		p->Count = p->Range;
	}
	else
	{
		MapMoveTileItem(&gMap, &p->tileItem, realPos);
	}
	if (p->Spin != 0)
	{
		p->Angle += p->Spin;
		if (p->Angle > 2 * PI)
		{
			p->Angle -= PI * 2;
		}
		if (p->Angle < 0)
		{
			p->Angle += PI * 2;
		}
	}
	return p->Count <= p->Range;
}

static void DrawParticle(const Vec2i pos, const TileItemDrawFuncData *data);
int ParticleAdd(CArray *particles, const AddParticle add)
{
	// Find an empty slot in list
	Particle *p = NULL;
	int i;
	for (i = 0; i < (int)particles->size; i++)
	{
		Particle *pSlot = CArrayGet(particles, i);
		if (!pSlot->isInUse)
		{
			p = pSlot;
			break;
		}
	}
	// If no empty slots, add a new one
	if (p == NULL)
	{
		Particle pNew;
		memset(&pNew, 0, sizeof pNew);
		CArrayPushBack(particles, &pNew);
		i = (int)particles->size - 1;
		p = CArrayGet(particles, i);
	}
	memset(p, 0, sizeof *p);
	p->Class = add.Class;
	p->Pos = add.FullPos;
	p->Z = add.Z;
	p->Angle = add.Angle;
	p->Vel = add.Vel;
	p->DZ = add.DZ;
	p->Spin = add.Spin;
	p->Range = RAND_INT(add.Class->RangeLow, add.Class->RangeHigh);
	p->isInUse = true;
	p->tileItem.x = p->tileItem.y = -1;
	p->tileItem.kind = KIND_PARTICLE;
	p->tileItem.id = i;
	p->tileItem.drawFunc = DrawParticle;
	p->tileItem.drawData.MobObjId = i;
	MapMoveTileItem(&gMap, &p->tileItem, Vec2iFull2Real(add.FullPos));
	return i;
}
void ParticleDestroy(CArray *particles, const int id)
{
	Particle *p = CArrayGet(particles, id);
	CASSERT(p->isInUse, "Destroying not-in-use particle");
	MapRemoveTileItem(&gMap, &p->tileItem);
	p->isInUse = false;
}

static void DrawParticle(const Vec2i pos, const TileItemDrawFuncData *data)
{
	const Particle *p = CArrayGet(&gParticles, data->MobObjId);
	CASSERT(p->isInUse, "Cannot draw non-existent particle");
	const Pic *pic;
	if (p->Class->Sprites)
	{
		pic = CArrayGet(
			&p->Class->Sprites->pics, (int)RadiansToDirection(p->Angle));
	}
	else
	{
		pic = p->Class->Pic;
	}
	CASSERT(pic != NULL, "particle picture not found");
	Vec2i picPos = Vec2iMinus(pos, Vec2iScaleDiv(pic->size, 2));
	picPos.y -= p->Z / 16;
	BlitMasked(&gGraphicsDevice, pic, picPos, p->Class->Mask, true);
}
