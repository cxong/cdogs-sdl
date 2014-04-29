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

	// Check if closest player is too far away, and follow him/her if so
	// Also calculate the squad number, so we have a formation
	// i.e. bigger squad number members follow at a further distance
	TActor *closestPlayer = NULL;
	TActor *closestEnemy;
	int minDistance2 = -1;
	int distanceTooFarFromPlayer = 8;
	int minEnemyDistance;
	int i;
	int squadNumber = 0;
	for (i = 0; i < gOptions.numPlayers; i++)
	{
		if (!IsPlayerAlive(i))
		{
			continue;
		}
		if (gPlayerDatas[i].inputDevice != INPUT_DEVICE_AI)
		{
			TActor *p = CArrayGet(&gActors, gPlayerIds[i]);
			int distance2 = DistanceSquared(
				Vec2iFull2Real(actor->Pos), Vec2iFull2Real(p->Pos));
			if (!closestPlayer || distance2 < minDistance2)
			{
				minDistance2 = distance2;
				closestPlayer = p;
			}
		}
		else if (actor->pData->playerIndex > i)
		{
			squadNumber++;
		}
	}
	// If player is exiting, we want to be very close to the player
	if (closestPlayer && closestPlayer->action == ACTORACTION_EXITING)
	{
		distanceTooFarFromPlayer = 2;
	}
	if (closestPlayer &&
		minDistance2 > distanceTooFarFromPlayer*distanceTooFarFromPlayer*16*16)
	{
		int cmd = AIGoto(actor, Vec2iFull2Real(closestPlayer->Pos));
		TObject *o;
		// Try to slide if there is a clear path and we are far enough away
		if ((cmd & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN)) &&
			AIHasClearPath(
			Vec2iFull2Real(actor->Pos), Vec2iFull2Real(closestPlayer->Pos)) &&
			minDistance2 > 7*7*16*16)
		{
			cmd |= CMD_BUTTON2;
		}
		// If running into safe object, and we're being blocked, shoot at it
		o = AIGetObjectRunningInto(actor, cmd);
		if (o && !(o->flags & OBJFLAG_DANGEROUS) &&
			Vec2iEqual(actor->Pos, actor->LastPos))
		{
			cmd = AIGoto(actor, Vec2iNew(o->tileItem.x, o->tileItem.y)) |
				CMD_BUTTON1;
		}
		return cmd;
	}

	// Check if closest enemy is close enough, and visible
	closestEnemy = AIGetClosestVisibleEnemy(actor->Pos, actor->flags, 1);
	if (closestEnemy)
	{
		minEnemyDistance = CHEBYSHEV_DISTANCE(
			actor->Pos.x, actor->Pos.y,
			closestEnemy->Pos.x, closestEnemy->Pos.y);
		// Also only engage if there's a clear shot
		if (minEnemyDistance > 0 && minEnemyDistance < ((12 * 16) << 8) &&
			AIHasClearShot(
			Vec2iFull2Real(actor->Pos), Vec2iFull2Real(closestEnemy->Pos)))
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
	if (closestPlayer &&
		minDistance2 > 4*4*16*16/3/3*(squadNumber+1)*(squadNumber+1))
	{
		int cmd = AIGoto(actor, Vec2iFull2Real(closestPlayer->Pos));
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

gun_e AICoopSelectWeapon(int player, int weapons[GUN_COUNT])
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
		if (weapons[preferredWeapon])
		{
			return preferredWeapon;
		}
	}

	// Preferred weapons unavailable;
	// give up and select most preferred weapon anyway
	return preferredWeapons[player][0];
}
