/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014, 2016 Cong Xu
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

#include <stdbool.h>

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
// Select: draw outline and drag contents to another location
// Add item: add items to the map
typedef enum
{
	BRUSHTYPE_POINT,
	BRUSHTYPE_LINE,
	BRUSHTYPE_BOX,
	BRUSHTYPE_BOX_FILLED,
	BRUSHTYPE_ROOM,
	BRUSHTYPE_ROOM_PAINTER,
	BRUSHTYPE_SELECT,
	BRUSHTYPE_FILL,
	BRUSHTYPE_SET_PLAYER_START,
	BRUSHTYPE_ADD_ITEM,
	BRUSHTYPE_ADD_CHARACTER,
	BRUSHTYPE_ADD_OBJECTIVE,
	BRUSHTYPE_ADD_KEY,
	BRUSHTYPE_SET_KEY,
	BRUSHTYPE_SET_EXIT
} BrushType;

// Encapsulates the drawing brushes and draws tiles to a static mission
// There are main and secondary types corresponding to mouse left and right
// BrushSize is the size of the stroke
// HighlightedTiles are the tiles that are highlighted to show the brush
// stroke
typedef struct
{
	BrushType Type;
	union
	{
		int ItemIndex;
		MapObject *MapObject;
	} u;
	int Index2;
	unsigned short MainType;
	unsigned short SecondaryType;
	unsigned short PaintType;
	int IsActive;
	int IsPainting;
	int BrushSize;
	Vec2i LastPos;
	Vec2i Pos;
	CArray HighlightedTiles;	// of Vec2i
	Vec2i SelectionStart;
	Vec2i SelectionSize;
	int IsMoving;	// for the select tool, whether selecting or moving
	Vec2i DragPos;	// when moving, location that the drag started

	char GuideImage[CDOGS_PATH_MAX];
	bool IsGuideImageNew;
	SDL_Surface *GuideImageSurface;
	Uint8 GuideImageAlpha;
} EditorBrush;

void EditorBrushInit(EditorBrush *b);
void EditorBrushTerminate(EditorBrush *b);

void EditorBrushSetHighlightedTiles(EditorBrush *b);
typedef enum
{
	EDITOR_RESULT_NONE,
	EDITOR_RESULT_CHANGED,
	EDITOR_RESULT_RELOAD,
	// Note: deliberately set so that bit checking works, i.e.
	// er & EDITOR_RESULT_CHANGED, er & EDITOR_RESULT_RELOAD
	EDITOR_RESULT_CHANGED_AND_RELOAD
} EditorResult;
#define EDITOR_RESULT_NEW(_change, _reload)\
	(EditorResult)((!!(_change)) | (!!(_reload) << 1))
EditorResult EditorBrushStartPainting(EditorBrush *b, Mission *m, int isMain);
EditorResult EditorBrushStopPainting(EditorBrush *b, Mission *m);
