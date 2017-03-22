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

    Copyright (c) 2013-2014, 2016-2017 Cong Xu
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
#include "draw/draw_buffer.h"
#include "gamedata.h"
#include "grafx_bg.h"

typedef struct
{
	const Pic *Head;
	Vec2i HeadOffset;
	const Pic *Body;
	Vec2i BodyOffset;
	const Pic *Legs;
	Vec2i LegsOffset;
	const Pic *Gun;
	Vec2i GunOffset;
	// In draw order
	const Pic *OrderedPics[BODY_PART_COUNT];
	Vec2i OrderedOffsets[BODY_PART_COUNT];
	const CharColors *Colors;
	bool IsDead;
	bool IsDying;
	bool IsTransparent;
	HSV *Tint;
	color_t *Mask;
	const CharSprites *Sprites;
} ActorPics;

void DrawCharacterSimple(
	Character *c, const Vec2i pos, const direction_e d,
	const bool hilite, const bool showGun);
void DrawHead(const Character *c, const direction_e dir, const Vec2i pos);

void DrawChatters(DrawBuffer *b, const Vec2i offset);

const Pic *GetHeadPic(
	const CharacterClass *c, const direction_e dir, const gunstate_e gunState);
ActorPics GetCharacterPics(
	Character *c, const direction_e dir,
	const ActorAnimation anim, const int frame,
	const NamedSprites *gunPics, const gunstate_e gunState,
	const bool isTransparent, HSV *tint, color_t *mask,
	const int deadPic);
ActorPics GetCharacterPicsFromActor(TActor *a);
void DrawActorPics(const ActorPics *pics, const Vec2i pos);
void DrawLaserSight(
	const ActorPics *pics, const TActor *a, const Vec2i picPos);
void DrawActorHighlight(
	const ActorPics *pics, const Vec2i pos, const color_t color);
