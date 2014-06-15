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

#include "game_events.h"


CArray gParticleClasses;
CArray gParticles;

void ParticleClassesInit(CArray *classes)
{
	CArrayInit(classes, sizeof(ParticleClass));
	ParticleClass c;

	memset(&c, 0, sizeof c);
	CSTRDUP(c.Name, "spark");
	c.Pic = PicManagerGetFromOld(
		&gPicManager, cGeneralPics[OFSPIC_SPARK].picIndex);
	CArrayPushBack(classes, &c);
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
const ParticleClass *ParticleClassGet(const CArray *classes, const char *name)
{
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
	return p->Count <= p->Range;
}

static const Pic *GetPic(int id, Vec2i *offset);
int ParticleAdd(
	CArray *particles, const ParticleClass *class,
	const Vec2i fullPos, const int z)
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
	p->Class = class;
	p->Pos = fullPos;
	p->Z = z;
	// TODO: speed and velocity
	/*p->Vel = Vec2iFull2Real(Vec2iScale(
		obj->vel, RAND_INT(b->SpeedLow, b->SpeedHigh)));*/
	p->Range = RAND_INT(class->RangeLow, class->RangeHigh);
	p->isInUse = true;
	p->tileItem.x = p->tileItem.y = -1;
	p->tileItem.kind = KIND_PARTICLE;
	p->tileItem.id = i;
	p->tileItem.getPicFunc = GetPic;
	MapMoveTileItem(&gMap, &p->tileItem, Vec2iFull2Real(fullPos));
	return i;
}
void ParticleDestroy(CArray *particles, const int id)
{
	Particle *p = CArrayGet(particles, id);
	CASSERT(p->isInUse, "Destroying not-in-use particle");
	MapRemoveTileItem(&gMap, &p->tileItem);
	p->isInUse = false;
}

static const Pic *GetPic(int id, Vec2i *offset)
{
	const Particle *p = CArrayGet(&gParticles, id);
	CASSERT(p->isInUse, "Cannot draw non-existent particle");
	offset->x = -p->Class->Pic->size.x / 2;
	offset->y = -p->Class->Pic->size.y / 2 - p->Z;
	return p->Class->Pic;
}
