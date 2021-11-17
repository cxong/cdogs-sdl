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

	Copyright (c) 2013-2017, 2019-2021 Cong Xu
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

#include <assert.h>
#include <stdbool.h>
#include <stdio.h> /* for stderr */
#include <stdlib.h>
#include <string.h>

#include "color.h"
#include "sys_specifics.h"

// Global variables so their address can be taken (passed into void * funcs)
extern bool gTrue;
extern bool gFalse;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifdef _MSC_VER
#define CHALT() __debugbreak()
#else
#define CHALT()
#endif

#define CASSERT(_x, _errmsg)                                                  \
	{                                                                         \
		volatile bool isOk = _x;                                              \
		if (!isOk)                                                            \
		{                                                                     \
			static char _buf[1024];                                           \
			sprintf(                                                          \
				_buf, "In %s %d:%s: " _errmsg " (" #_x ")", __FILE__,         \
				__LINE__, __func__);                                          \
			CHALT();                                                          \
			assert(_x);                                                       \
		}                                                                     \
	}

// Even though malloc(0) may return NULL, we don't account for it for
// simplicity and to allow code linters to work better
#define _CCHECKALLOC(_func, _var, _size)                                      \
	{                                                                         \
		if (_var == NULL)                                                     \
		{                                                                     \
			exit(1);                                                          \
		}                                                                     \
	}

#define CMALLOC(_var, _size)                                                  \
	{                                                                         \
		_var = malloc(_size);                                                 \
		_CCHECKALLOC("CMALLOC", _var, (_size))                                \
	}
#define CCALLOC(_var, _size)                                                  \
	{                                                                         \
		_var = calloc(1, _size);                                              \
		_CCHECKALLOC("CCALLOC", _var, (_size))                                \
	}
#define CREALLOC(_var, _size)                                                 \
	{                                                                         \
		_var = realloc(_var, _size);                                          \
		_CCHECKALLOC("CREALLOC", _var, (_size))                               \
	}
#define CSTRDUP(_var, _str)                                                   \
	{                                                                         \
		CMALLOC(_var, strlen(_str) + 1);                                      \
		strcpy(_var, _str);                                                   \
	}

#define CFREE(_var)                                                           \
	{                                                                         \
		free(_var);                                                           \
	}

#define UNUSED(expr) (void)(expr);

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#define CLAMP(v, _min, _max) MAX((_min), MIN((_max), (v)))
#define CLAMP_OPPOSITE(v, _min, _max)                                         \
	((v) > (_max) ? (_min) : ((v) < (_min) ? (_max) : (v)))
#define SIGN(x) ((x) != 0 ? (x) / abs(x) : 1)
#define SQUARED(x) ((x) * (x))
#define SWAP(x, y, T)                                                         \
	do                                                                        \
	{                                                                         \
		T _tmp = x;                                                           \
		x = y;                                                                \
		y = _tmp;                                                             \
	} while (0)

const char *StrGetFileExt(const char *filename);

void PathGetDirname(char *buf, const char *path);
// Given /path/to/file, return file
const char *PathGetBasename(const char *path);
void PathGetWithoutExtension(char *buf, const char *path);
void PathGetBasenameWithoutExtension(char *buf, const char *path);
void RealPath(const char *src, char *dest);
void RelPath(char *buf, const char *to, const char *from);
void FixPathSeparator(char *dst, const char *src);
char *CDogsGetCWD(char *buf);
void RelPathFromCWD(char *buf, const char *to);
void GetDataFilePath(char *buf, const char *path);

double Round(double x);

double ToDegrees(double radians);

struct vec2 CalcClosestPointOnLineSegmentToPoint(
	const struct vec2 l1, const struct vec2 l2, const struct vec2 p);

typedef enum
{
	INPUT_DEVICE_UNSET,
	INPUT_DEVICE_KEYBOARD,
	INPUT_DEVICE_JOYSTICK,

	// Fake device used for co-op AI
	INPUT_DEVICE_AI,

	INPUT_DEVICE_COUNT
} input_device_e;

const char *InputDeviceName(const int d, const int deviceIndex);

typedef enum
{
	ALLYCOLLISION_NORMAL,
	ALLYCOLLISION_REPEL,
	ALLYCOLLISION_NONE
} AllyCollision;
const char *AllyCollisionStr(int a);
int StrAllyCollision(const char *str);

char *IntStr(int i);
char *PercentStr(int p);
char *Div8Str(int i);
void CamelToTitle(char *buf, const char *src);
bool StrEndsWith(const char *str, const char *suffix);
int Stricmp(const char *a, const char *b);
int CompareIntsAsc(const void *v1, const void *v2);
int CompareIntsDesc(const void *v1, const void *v2);
bool IntsEqual(const void *v1, const void *v2);

// Helper macros for defining type/str conversion funcs
#define T2S(_type, _str)                                                      \
	case _type:                                                               \
		return _str;
#define S2T(_type, _str)                                                      \
	if (strcmp(s, _str) == 0)                                                 \
	{                                                                         \
		return _type;                                                         \
	}

#define RAND_INT(_low, _high)                                                 \
	((_low) == (_high) ? (_low) : (_low) + (rand() % ((_high) - (_low))))
#define RAND_FLOAT(_low, _high) (float)RAND_DOUBLE(_low, _high)
#define RAND_DOUBLE(_low, _high)                                              \
	((_low) + ((double)rand() / RAND_MAX * ((_high) - (_low))))
#define RAND_BOOL() (RAND_INT(0, 1) == 0)

typedef enum
{
	BODY_PART_HEAD,
	BODY_PART_HAIR,
	BODY_PART_BODY,
	BODY_PART_LEGS,
	BODY_PART_GUN_R,
	BODY_PART_GUN_L,
	BODY_PART_COUNT
} BodyPart;

BodyPart StrBodyPart(const char *s);

typedef enum
{
	PLACEMENT_ACCESS_ANY,		// place anywhere
	PLACEMENT_ACCESS_LOCKED,	// place in locked rooms
	PLACEMENT_ACCESS_NOT_LOCKED // don't place in locked rooms
} PlacementAccessFlags;

int Pulse256(const int t);
