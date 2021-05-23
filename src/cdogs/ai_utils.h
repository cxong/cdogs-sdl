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

    Copyright (c) 2013-2015, 2017, 2021 Cong Xu
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

// Utility functions for AIs; contains basic behaviour

#include "actors.h"
#include "objs.h"

TActor *AIGetClosestPlayer(const struct vec2 pos);
const TActor *AIGetClosestEnemy(
	const struct vec2 from, const TActor *a, const int flags);
const TActor *AIGetClosestVisibleEnemy(
	const TActor *from, const bool isPlayer);
struct vec2 AIGetClosestPlayerPos(const struct vec2 pos);
int AIReverseDirection(int cmd);
bool AIHasClearView(const TActor *a, const struct vec2 to, const int sightRange);
bool AIHasClearShot(const struct vec2 from, const struct vec2 to);
bool AIHasClearPath(
	const struct vec2 from, const struct vec2 to, const bool ignoreObjects);
bool AIHasPath(
	const struct vec2 from, const struct vec2 to, const bool ignoreObjects);
TObject *AIGetObjectRunningInto(TActor *a, int cmd);
// AI is facing something within a 90 degree arc
bool AIIsFacing(const TActor *a, const struct vec2 target, const direction_e d);
// AI can see something in view or in periphery
bool AICanSee(const TActor *a, const struct vec2 target, const direction_e d);

// Find path to target
// destroyObjects - if true, ignore obstructing objects
//                - if false, will pathfind around them
int AIGoto(const TActor *actor, const struct vec2 p, const bool ignoreObjects);
int AIGotoDirect(const struct vec2 a, const struct vec2 p);
int AIHunt(const TActor *actor, const struct vec2 targetPos);
int AIAttack(const TActor *a, const struct vec2 targetPos);
int AIHuntClosest(TActor *actor);
int AIRetreatFrom(const TActor *actor, const struct vec2 from);
// Like Hunt but biases towards 8 axis movement
int AITrack(const TActor *actor, const struct vec2 targetPos);
int AIMoveAwayFromLine(
	const struct vec2 pos, const struct vec2 lineStart, const direction_e lineD);

// Pathfinding helper functions
bool IsTileWalkable(Map *map, const struct vec2i pos);
bool IsTileWalkableAroundObjects(Map *map, const struct vec2i pos);
