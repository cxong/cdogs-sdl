/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003 Lucas Martin-King

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

    Copyright (c) 2013-2014, Cong Xu
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
#ifndef __EDITOR_UI
#define __EDITOR_UI

#include "ui_object.h"

#include <cdogs/gamedata.h>

#include "editor_brush.h"


#define YC_CAMPAIGNTITLE    0
#define YC_MISSIONINDEX     1
#define YC_MISSIONTITLE     2
#define YC_MISSIONLOOKS     4
#define YC_MISSIONDESC      5
#define YC_CHARACTERS       6
#define YC_SPECIALS         7
#define YC_ITEMS            9
#define YC_OBJECTIVES       10

#define XC_CAMPAIGNTITLE    0
#define XC_AUTHOR           1
#define XC_CAMPAIGNDESC     2

#define XC_MISSIONTITLE     0
#define XC_MUSICFILE        1

#define XC_WALL             0
#define XC_FLOOR            1
#define XC_ROOM             2
#define XC_DOORS            3
#define XC_KEYS             4
#define XC_EXIT             5
#define XC_COLOR1           6
#define XC_COLOR2           7
#define XC_COLOR3           8
#define XC_COLOR4           9

#define XC_TYPE             0
#define XC_INDEX            1
#define XC_REQUIRED         2
#define XC_TOTAL            3
#define XC_FLAGS            4


#define YC_APPEARANCE 0
#define YC_ATTRIBUTES 1
#define YC_FLAGS      2
#define YC_WEAPON     3

#define XC_FACE       0
#define XC_SKIN       1
#define XC_HAIR       2
#define XC_BODY       3
#define XC_ARMS       4
#define XC_LEGS       5

#define XC_SPEED      0
#define XC_HEALTH     1
#define XC_MOVE       2
#define XC_TRACK      3
#define XC_SHOOT      4
#define XC_DELAY      5

#define XC_ASBESTOS      0
#define XC_IMMUNITY      1
#define XC_SEETHROUGH    2
#define XC_RUNS_AWAY     3
#define XC_SNEAKY        4
#define XC_GOOD_GUY      5
#define XC_SLEEPING      6
#define XC_PRISONER      7
#define XC_INVULNERABLE  8
#define XC_FOLLOWER      9
#define XC_PENALTY       10
#define XC_VICTIM        11
#define XC_AWAKE         12


void DisplayFlag(
	GraphicsDevice *g, Vec2i pos, const char *s, int isOn, int isHighlighted);

UIObject *CreateMainObjs(CampaignOptions *co, EditorBrush *brush, Vec2i size);

UIObject *CreateCharEditorObjs(void);

#endif
