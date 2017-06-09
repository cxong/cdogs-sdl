/*
C-Dogs SDL
A port of the legendary (and fun) action/arcade cdogs.
Copyright (c) 2016, Cong Xu
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
#include "emitter.h"

#include "game_events.h"


void EmitterInit(
	Emitter *em, const ParticleClass *p, const Vec2i offset,
	const int minSpeed, const int maxSpeed, const int minDZ, const int maxDZ,
	double minRotation, double maxRotation)
{
	memset(em, 0, sizeof *em);
	em->p = p;
	em->offset = offset;
	em->minSpeed = minSpeed;
	em->maxSpeed = maxSpeed;
	em->minDZ = minDZ;
	em->maxDZ = maxDZ;
	em->minRotation = minRotation;
	em->maxRotation = maxRotation;
}
void EmitterStart(
	Emitter *em, const Vec2i fullPos, const int z, const Vec2i vel)
{
	const Vec2i p = Vec2iAdd(fullPos, Vec2iReal2Full(em->offset));

	// TODO: single event multiple particles
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PARTICLE);
	e.u.AddParticle.FullPos = p;
	e.u.AddParticle.Z = z * Z_FACTOR;
	e.u.AddParticle.Class = em->p;
	const int speed = RAND_INT(em->minSpeed, em->maxSpeed);
	const Vec2i baseVel = Vec2iFromPolar(speed, RAND_DOUBLE(0, PI * 2));
	e.u.AddParticle.Vel = Vec2iAdd(vel, baseVel);
	e.u.AddParticle.Angle = RAND_DOUBLE(0, PI * 2);
	e.u.AddParticle.DZ = RAND_INT(em->minDZ, em->maxDZ);
	e.u.AddParticle.Spin = RAND_DOUBLE(em->minRotation, em->maxRotation);
	GameEventsEnqueue(&gGameEvents, e);
}
