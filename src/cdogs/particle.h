/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014-2015, Cong Xu
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
#pragma once

#include <json/json.h>

#include "pic.h"
#include "tile.h"

typedef struct
{
	char *Name;
	const Pic *Pic;
	const NamedSprites *Sprites;
	// -1 is infinite range
	int RangeLow;
	int RangeHigh;
	int TicksPerFrame;
	color_t Mask;
	int GravityFactor;
	bool HitsWalls;
	bool Bounces;
	bool WallBounces;
} ParticleClass;
typedef struct
{
	CArray Classes;	// of ParticleClass
	CArray CustomClasses;	// of ParticleClass
} ParticleClasses;
extern ParticleClasses gParticleClasses;

typedef struct
{
	const ParticleClass *Class;
	// Coordinates are in full
	Vec2i Pos;
	int Z;
	double Angle;
	int DZ;
	double Spin;
	int Count;
	int Range;
	TTileItem tileItem;
	bool isInUse;
} Particle;
extern CArray gParticles;	// of Particle

typedef struct
{
	const ParticleClass *Class;
	Vec2i FullPos;
	int Z;
	Vec2i Vel;
	double Angle;
	int DZ;
	double Spin;
} AddParticle;

void ParticleClassesInit(ParticleClasses *classes, const char *filename);
void ParticleClassesLoadJSON(CArray *classes, json_t *root);
void ParticleClassesTerminate(ParticleClasses *classes);
void ParticleClassesClear(CArray *classes);
const ParticleClass *StrParticleClass(
	const ParticleClasses *classes, const char *name);

void ParticlesInit(CArray *particles);
void ParticlesTerminate(CArray *particles);
void ParticlesUpdate(CArray *particles, const int ticks);

int ParticleAdd(CArray *particles, const AddParticle add);
void ParticleDestroy(CArray *particles, const int id);
