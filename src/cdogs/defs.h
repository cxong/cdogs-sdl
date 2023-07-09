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

	Copyright (c) 2014-2015, 2017-2018, 2021 Cong Xu
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

#include "vector.h"

// Defines
#define BODY_UNARMED 0
#define BODY_ARMED 1
#define BODY_COUNT 2

#define MAX_BARRELS 2

// Commands
#define CMD_LEFT 1
#define CMD_RIGHT 2
#define CMD_UP 4
#define CMD_DOWN 8
#define CMD_BUTTON1 16
#define CMD_BUTTON2 32
#define CMD_MAP 64
#define CMD_ESC 128
#define CMD_GRENADE 256

// Command macros
#define Left(x) (((x)&CMD_LEFT) != 0)
#define Right(x) (((x)&CMD_RIGHT) != 0)
#define Up(x) (((x)&CMD_UP) != 0)
#define Down(x) (((x)&CMD_DOWN) != 0)
#define Button1(x) (((x)&CMD_BUTTON1) != 0)
#define Button2(x) (((x)&CMD_BUTTON2) != 0)
#define AnyButton(x) (((x) & (CMD_BUTTON1 | CMD_BUTTON2 | CMD_GRENADE)) != 0)
#define CMD_DIRECTIONS (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN)
#define CMD_HAS_DIRECTION(x) ((x)&CMD_DIRECTIONS)

// Reverse directions for command
int CmdGetReverse(int cmd);

// Directions
typedef enum
{
	DIRECTION_UP,
	DIRECTION_UPRIGHT,
	DIRECTION_RIGHT,
	DIRECTION_DOWNRIGHT,
	DIRECTION_DOWN,
	DIRECTION_DOWNLEFT,
	DIRECTION_LEFT,
	DIRECTION_UPLEFT,
	DIRECTION_COUNT
} direction_e;

typedef enum
{
	SPECIAL_NONE,
	SPECIAL_FLAME,
	SPECIAL_POISON,
	SPECIAL_PETRIFY,
	SPECIAL_CONFUSE
} special_damage_e;
special_damage_e StrSpecialDamage(const char *s);

#define Z_FACTOR 16 // the number of increments used for Z

extern int cmd2dir[16];
extern int dir2cmd[8];
extern float dir2radians[8];

#define CmdToDirection(c) cmd2dir[(c)&15]
#define DirectionToCmd(d) dir2cmd[(d)&7]

struct vec2 Vec2FromRadiansScaled(const float radians);
struct vec2 Vec2FromRadians(const float radians);
struct vec2i Vec2iFromDirection(const direction_e d);
direction_e RadiansToDirection(const double r);
direction_e DirectionOpposite(const direction_e d);
direction_e DirectionMirrorX(const direction_e d);
direction_e DirectionRotate(const direction_e d, const int dClockwise);

// Quarter of a full pos, for movement
#define EPSILON_POS (1.0f / 256.0f / 4.0f)
