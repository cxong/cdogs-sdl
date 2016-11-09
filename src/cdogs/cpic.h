/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2016, Cong Xu
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

#include "defs.h"
#include "grafx.h"
#include "pic.h"

typedef struct
{
	Pic pic;
	char *name;
} NamedPic;
typedef struct
{
	CArray pics;	// of Pic
	char *name;
} NamedSprites;

typedef enum
{
	PICTYPE_NORMAL,
	PICTYPE_DIRECTIONAL,
	PICTYPE_ANIMATED,
	PICTYPE_ANIMATED_RANDOM
} PicType;
PicType StrPicType(const char *s);
// Abstract drawable pic, can draw multiple types of pics
typedef struct
{
	PicType Type;
	union
	{
		const Pic *Pic;
		const CArray *Sprites;	// of Pic
		struct
		{
			const CArray *Sprites;	// of Pic
			int Count;
			int Frame;
			int TicksPerFrame;
		} Animated;
	} u;
	// Special draw options
	bool UseMask;
	union
	{
		color_t Mask;
		HSV Tint;
	} u1;
} CPic;
typedef struct
{
	direction_e Dir;
	Vec2i Offset;
} CPicDrawContext;
typedef CPicDrawContext (*GetDrawContextFunc)(const int);

void NamedPicFree(NamedPic *n);

void NamedSpritesInit(NamedSprites *ns, const char *name);
void NamedSpritesFree(NamedSprites *ns);

void CPicLoadJSON(CPic *p, json_t *node);
void CPicLoadNormal(CPic *p, json_t *node);
bool CPicIsLoaded(const CPic *p);
Vec2i CPicGetSize(const CPic *p);
// Copy everything except frame
void CPicCopyPic(CPic *dest, const CPic *src);
void CPicUpdate(CPic *p, const int ticks);
const Pic *CPicGetPic(const CPic *p, const int idx);
void CPicDraw(
	GraphicsDevice *g, const CPic *p,
	const Vec2i pos, const CPicDrawContext *context);
