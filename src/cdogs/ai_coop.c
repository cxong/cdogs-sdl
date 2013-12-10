/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

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
#include "ai_coop.h"

#include "ai_utils.h"

int AICoopGetCmd(TActor *actor)
{
	// Use decision tree to command the AI
	// - If too far away from nearest player
	//   - Go to nearest player
	// - else
	//   - If closest enemy is close enough
	//     - Attack enemy
	//   - else
	//     - Go to nearest player

	// TODO: this means that AI is helpless when human players dead
	// Ambitious goal: AI able to complete mission on its own?

	// Check if closest player is too far away, and follow him/her if so
	TActor *closestPlayer = NULL;
	TActor *closestEnemy;
	int minDistance = -1;
	int minEnemyDistance;
	int i;
	for (i = 0; i < gOptions.numPlayers; i++)
	{
		if (IsPlayerAlive(i) && gPlayerDatas[i].inputDevice != INPUT_DEVICE_AI)
		{
			TActor *p = gPlayers[i];
			int distance = CHEBYSHEV_DISTANCE(actor->x, actor->y, p->x, p->y);
			if (!closestPlayer || distance < minDistance)
			{
				minDistance = distance;
				closestPlayer = p;
			}
		}
	}
	if (closestPlayer && minDistance > ((8 * 16) << 8))
	{
		int cmd = AIGoto(
			actor,
			Vec2iFull2Real(Vec2iNew(closestPlayer->x, closestPlayer->y)));
		TObject *o;
		// Try to slide if there is a clear path and we are far enough away
		if ((cmd & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN)) &&
			AIHasClearLine(
			Vec2iFull2Real(Vec2iNew(actor->x, actor->y)),
			Vec2iFull2Real(Vec2iNew(closestPlayer->x, closestPlayer->y))) &&
			minDistance > ((7 * 16) << 8))
		{
			cmd |= CMD_BUTTON2;
		}
		// If running into safe object, shoot at it
		o = AIGetObjectRunningInto(actor, cmd);
		if (o && !(o->flags & OBJFLAG_DANGEROUS))
		{
			cmd = AIGoto(actor, Vec2iNew(o->tileItem.x, o->tileItem.y)) |
				CMD_BUTTON1;
		}
		return cmd;
	}

	// Check if closest enemy is close enough, and visible
	closestEnemy = AIGetClosestVisibleEnemy(
		Vec2iNew(actor->x, actor->y), actor->flags, 1);
	if (closestEnemy)
	{
		minEnemyDistance = CHEBYSHEV_DISTANCE(
			actor->x, actor->y, closestEnemy->x, closestEnemy->y);
		// Also only engage if there's a clear shot
		if (minEnemyDistance > 0 && minEnemyDistance < ((12 * 16) << 8) &&
			AIHasClearLine(
			Vec2iFull2Real(Vec2iNew(actor->x, actor->y)),
			Vec2iFull2Real(Vec2iNew(closestEnemy->x, closestEnemy->y))))
		{
			int cmd = AIHunt(actor);
			// only fire if gun is ready
			if (actor->weapon.lock <= 0)
			{
				cmd |= CMD_BUTTON1;
			}
			return cmd;
		}
	}

	// Otherwise, just go towards the closest player as long as we don't
	// run into them
	if (closestPlayer && minDistance > ((2 * 16) << 8))
	{
		int cmd = AIGoto(
			actor,
			Vec2iFull2Real(Vec2iNew(closestPlayer->x, closestPlayer->y)));
		// If running into safe object, shoot at it
		TObject *o = AIGetObjectRunningInto(actor, cmd);
		if (o && !(o->flags & OBJFLAG_DANGEROUS))
		{
			cmd = AIGoto(actor, Vec2iNew(o->tileItem.x, o->tileItem.y)) |
				CMD_BUTTON1;
		}
		return cmd;
	}

	return 0;
}

gun_e AICoopSelectWeapon(
	int player, gun_e *availableWeapons, int numAvailableWeapons)
{
	// Weapon preferences, for different player indices
#define NUM_PREFERRED_WEAPONS 5
	gun_e preferredWeapons[][NUM_PREFERRED_WEAPONS] =
	{
		{ GUN_MG, GUN_SHOTGUN, GUN_POWERGUN, GUN_SNIPER, GUN_FLAMER },
		{ GUN_FLAMER, GUN_SHOTGUN, GUN_MG, GUN_POWERGUN, GUN_SNIPER },
		{ GUN_SHOTGUN, GUN_MG, GUN_POWERGUN, GUN_FLAMER, GUN_SNIPER },
		{ GUN_POWERGUN, GUN_SNIPER, GUN_MG, GUN_SHOTGUN, GUN_FLAMER }
	};
	int i;

	// First try to select an available weapon
	for (i = 0; i < NUM_PREFERRED_WEAPONS; i++)
	{
		gun_e preferredWeapon = preferredWeapons[player][i];
		int j;
		for (j = 0; j < numAvailableWeapons; j++)
		{
			if (preferredWeapon == availableWeapons[j])
			{
				return preferredWeapon;
			}
		}
	}

	// Preferred weapons unavailable;
	// give up and select most preferred weapon anway
	return preferredWeapons[player][0];
}
