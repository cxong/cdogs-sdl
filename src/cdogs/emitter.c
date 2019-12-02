/*
C-Dogs SDL
A port of the legendary (and fun) action/arcade cdogs.
Copyright (c) 2016-2017, 2019 Cong Xu
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
	Emitter *em, const ParticleClass *p, const struct vec2 offset,
	const float minSpeed, const float maxSpeed,
	const int minDZ, const int maxDZ,
	const double minRotation, const double maxRotation,
	const int ticksPerEmit)
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
	em->ticksPerEmit = ticksPerEmit;
}
void EmitterStart(Emitter *em, const AddParticle *data)
{
	const struct vec2 p = svec2_add(data->Pos, em->offset);

	// TODO: single event multiple particles
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PARTICLE);
	e.u.AddParticle = *data;
	e.u.AddParticle.Pos = p;
	e.u.AddParticle.Z *= Z_FACTOR;
	if (e.u.AddParticle.Class == NULL)
	{
		e.u.AddParticle.Class = em->p;
	}
	const float speed = RAND_FLOAT(em->minSpeed, em->maxSpeed);
	const struct vec2 baseVel = svec2_rotate(
		svec2(0, speed), RAND_FLOAT(0, MPI * 2));
	e.u.AddParticle.Vel = svec2_add(data->Vel, baseVel);
	if (isnan(data->Angle))
	{
		e.u.AddParticle.Angle = RAND_FLOAT(0, MPI * 2);
	}
	e.u.AddParticle.DZ = RAND_FLOAT(em->minDZ, em->maxDZ);
	e.u.AddParticle.Spin = RAND_DOUBLE(em->minRotation, em->maxRotation);
	GameEventsEnqueue(&gGameEvents, e);
}

void EmitterUpdate(Emitter *em, const AddParticle *data, const int ticks)
{
	if (em->ticksPerEmit > 0)
	{
		em->counter -= ticks;
		if (em->counter <= 0)
		{
			EmitterStart(em, data);
			em->counter += em->ticksPerEmit;
		}
	}
}
