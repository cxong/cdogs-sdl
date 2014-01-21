/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014, Cong Xu
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
#ifndef __EDITOR_BRUSH
#define __EDITOR_BRUSH

#include <cdogs/c_array.h>
#include <cdogs/mission.h>
#include <cdogs/vector.h>

// Point: draw as long as mouse is down; to smooth input draws a line from
//        last known position to current position
// Line: draw line from mouse down position to mouse up position
// Box: draw box outline from mouse down position to mouse up position
// Box filled: like box but with filled interior
// Room: special type of box; the outline is always wall and the interior is
//       always room
typedef enum
{
	BRUSHTYPE_POINT,
	BRUSHTYPE_LINE,
	BRUSHTYPE_BOX,
	BRUSHTYPE_BOX_FILLED,
	BRUSHTYPE_ROOM
} BrushType;
const char *BrushTypeStr(BrushType t);
BrushType StrBrushType(const char *s);

// Encapsulates the drawing brushes and draws tiles to a static mission
// There are main and secondary types corresponding to mouse left and right
// BrushSize is the size of the stroke
// HighlightedTiles are the tiles that are highlighted to show the brush
// stroke
typedef struct
{
	BrushType Type;
	unsigned short MainType;
	unsigned short SecondaryType;
	unsigned short PaintType;
	int IsActive;
	int IsPainting;
	int BrushSize;
	Vec2i LastPos;
	Vec2i Pos;
	CArray HighlightedTiles;	// of Vec2i
} EditorBrush;

void EditorBrushInit(EditorBrush *b);
void EditorBrushTerminate(EditorBrush *b);

void EditorBrushSetHighlightedTiles(EditorBrush *b);
int EditorBrushStartPainting(EditorBrush *b, Mission *m, int isMain);
int EditorBrushStopPainting(EditorBrush *b, Mission *m);

#endif
