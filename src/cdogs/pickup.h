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

#include "actors.h"
#include "pic.h"
#include "pickup_class.h"
#include "tile.h"

// Pickups are game objects that players can collect, and which have special
// effects
typedef struct
{
	int UID;
	const PickupClass *class;
	TTileItem tileItem;
	bool IsRandomSpawned;
	bool isInUse;
	bool PickedUp;
	// For spawned pickups, the UID of the spawner (-1 otherwise)
	int SpawnerUID;
} Pickup;

extern CArray gPickups;	// of Pickup

void PickupsInit(void);
void PickupsTerminate(void);
int PickupsGetNextUID(void);
void PickupAdd(const NAddPickup ap);
void PickupDestroy(const int uid);

void PickupPickup(TActor *a, Pickup *p, const bool pickupAll);
// Check if the pickup needs to be picked up manually
bool PickupIsManual(const Pickup *p);

Pickup *PickupGetByUID(const int uid);
