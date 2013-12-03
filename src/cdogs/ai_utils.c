/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003-2007 Lucas Martin-King

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

    This file incorporates work covered by the following copyright and
    permission notice:

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
#include "ai_utils.h"

TActor *AIGetClosestPlayer(Vec2i pos)
{
	int i;
	int minDistance = -1;
	TActor *closestPlayer = NULL;
	for (i = 0; i < gOptions.numPlayers; i++)
	{
		if (IsPlayerAlive(i))
		{
			TActor *p = gPlayers[i];
			int distance = CHEBYSHEV_DISTANCE(pos.x, pos.y, p->x, p->y);
			if (!closestPlayer || distance < minDistance)
			{
				closestPlayer = p;
				minDistance = distance;
			}
		}
	}
	return closestPlayer;
}

TActor *AIGetClosestEnemy(Vec2i from, int flags, int isPlayer)
{
	// Search all the actors and find the closest one that is an enemy
	TActor *a;
	TActor *closestEnemy = NULL;
	int minDistance = -1;
	for (a = ActorList(); a; a = a->next)
	{
		int isEnemy = 0;
		int distance;
		// Never target invulnerables or victims
		if (a->flags & (FLAGS_INVULNERABLE | FLAGS_VICTIM))
		{
			continue;
		}
		if (a->pData || (a->flags & FLAGS_GOOD_GUY))
		{
			// target is good guy / player, check if we are bad
			if (!isPlayer && !(flags & FLAGS_GOOD_GUY))
			{
				isEnemy = 1;
			}
		}
		else
		{
			// target is bad guy, check if we are good
			if (isPlayer || (flags & FLAGS_GOOD_GUY))
			{
				isEnemy = 1;
			}
		}
		if (isEnemy)
		{
			distance = CHEBYSHEV_DISTANCE(from.x, from.y, a->x, a->y);
			if (!closestEnemy || distance < minDistance)
			{
				minDistance = distance;
				closestEnemy = a;
			}
		}
	}
	return closestEnemy;
}

Vec2i AIGetClosestPlayerPos(Vec2i pos)
{
	TActor *closestPlayer = AIGetClosestPlayer(pos);
	if (closestPlayer)
	{
		return Vec2iNew(closestPlayer->x, closestPlayer->y);
	}
	else
	{
		return pos;
	}
}

int AIReverseDirection(int cmd)
{
	if (cmd & (CMD_LEFT | CMD_RIGHT))
	{
		cmd ^= CMD_LEFT | CMD_RIGHT;
	}
	if (cmd & (CMD_UP | CMD_DOWN))
	{
		cmd ^= CMD_UP | CMD_DOWN;
	}
	return cmd;
}

int AIGoto(TActor *actor, Vec2i p)
{
	int cmd = 0;
	p.x >>= 8;
	p.y >>= 8;

	if ((actor->x >> 8) < p.x - 1)		cmd |= CMD_RIGHT;
	else if ((actor->x >> 8) > p.x + 1)	cmd |= CMD_LEFT;

	if ((actor->y >> 8) < p.y - 1)		cmd |= CMD_DOWN;
	else if ((actor->y >> 8) > p.y + 1)	cmd |= CMD_UP;

	return cmd;
}

int AIHunt(TActor *actor)
{
	int cmd = 0;
	int dx, dy;
	Vec2i targetPos = Vec2iNew(actor->x, actor->y);
	if (!(actor->pData || (actor->flags & FLAGS_GOOD_GUY)))
	{
		targetPos = AIGetClosestPlayerPos(Vec2iNew(actor->x, actor->y));
	}

	if (actor->flags & FLAGS_VISIBLE)
	{
		TActor *a = AIGetClosestEnemy(
			Vec2iNew(actor->x, actor->y), actor->flags, !!actor->pData);
		if (a)
		{
			targetPos.x = a->x;
			targetPos.y = a->y;
		}
	}

	dx = abs(targetPos.x - actor->x);
	dy = abs(targetPos.y - actor->y);

	if (2 * dx > dy)
	{
		if (actor->x < targetPos.x)			cmd |= CMD_RIGHT;
		else if (actor->x > targetPos.x)	cmd |= CMD_LEFT;
	}
	if (2 * dy > dx)
	{
		if (actor->y < targetPos.y)			cmd |= CMD_DOWN;
		else if (actor->y > targetPos.y)	cmd |= CMD_UP;
	}
	// If it's a coward, reverse directions...
	if (actor->flags & FLAGS_RUNS_AWAY)
	{
		cmd = AIReverseDirection(cmd);
	}

	return cmd;
}
