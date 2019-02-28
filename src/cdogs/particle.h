/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014-2015, 2017-2019 Cong Xu
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
#include "thing.h"

typedef enum
{
	PARTICLE_PIC,
	PARTICLE_TEXT
} ParticleType;
ParticleType StrParticleType(const char *s);

typedef struct
{
	char *Name;
	ParticleType Type;
	union
	{
		CPic Pic;
		color_t TextColor;
	} u;
	// -1 is infinite range
	int RangeLow;
	int RangeHigh;
	int GravityFactor;
	bool HitsWalls;
	bool Bounces;
	bool WallBounces;
	bool ZDarken;	// darken as the particle falls to the ground
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
	union
	{
		CPic Pic;
		char *Text;
	} u;
	struct vec2 Pos;
	int Z;
	double Angle;
	int DZ;
	double Spin;
	int Count;
	int Range;
	Thing thing;
	bool isInUse;
} Particle;
extern CArray gParticles;	// of Particle

typedef struct
{
	const ParticleClass *Class;
	struct vec2 Pos;
	int Z;
	struct vec2 Vel;
	double Angle;
	int DZ;
	double Spin;
	struct vec2 DrawScale;
	color_t Mask;
	char Text[128];
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
