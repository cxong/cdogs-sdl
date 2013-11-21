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
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "pics.h"
#include "draw.h"
#include "blit.h"
#include "pic_manager.h"
#include "text.h"


void FixBuffer(DrawBuffer *buffer)
{
	int x, y;
	Tile *tile, *tileBelow;

	tile = &buffer->tiles[0][0];
	tileBelow = &buffer->tiles[0][0] + X_TILES;
	for (y = 0; y < Y_TILES - 1; y++)
	{
		for (x = 0; x < buffer->width; x++, tile++, tileBelow++)
		{
			if (!(tile->flags & (MAPTILE_IS_WALL | MAPTILE_OFFSET_PIC)) &&
				(tileBelow->flags & MAPTILE_IS_WALL))
			{
				tile->pic = &picNone;
			}
			else if ((tile->flags & MAPTILE_IS_WALL) &&
				(tileBelow->flags & MAPTILE_IS_WALL))
			{
				tile->flags |= MAPTILE_DELAY_DRAW;
			}
		}
		tile += X_TILES - buffer->width;
		tileBelow += X_TILES - buffer->width;
	}

	tile = &buffer->tiles[0][0];
	for (y = 0; y < Y_TILES; y++)
	{
		for (x = 0; x < buffer->width; x++, tile++)
		{
			if (!(tile->flags & MAPTILE_IS_VISIBLE))
			{
				tile->things = NULL;
				tile->flags |= MAPTILE_OUT_OF_SIGHT;
			}
			else
			{
				MapMarkAsVisited(
					Vec2iNew(x + buffer->xStart, y + buffer->yStart));
				tile->flags &= ~MAPTILE_OUT_OF_SIGHT;
				tile->isVisited = 1;
			}
		}
		tile += X_TILES - buffer->width;
	}
}

static void SetVisible(DrawBuffer *buffer, int x, int y)
{
	Tile *tile = &buffer->tiles[0][0] + y*X_TILES + x;
	tile->flags |= MAPTILE_IS_VISIBLE;
}

// Set current tile to visible if the last tile is visible and
// is not an obstruction
static void SetLineOfSight(
	DrawBuffer *buffer, int x, int y, int dx, int dy)
{
	Tile *dTile = &buffer->tiles[0][0] + (y+dy)*X_TILES + (x+dx);
	if ((dTile->flags & MAPTILE_IS_VISIBLE) &&
		!(dTile->flags & MAPTILE_NO_SEE))
	{
		SetVisible(buffer, x, y);
	}
}

// Calculate line of sight using raycasting
// Start from tiles that are sources of light (i.e. visible)
// Then going outwards, mark tiles as visible unless obstructions
// e.g. walls are encountered, and leave the rest unlit
void LineOfSight(Vec2i center, DrawBuffer *buffer)
{
	int x, y;
	Vec2i centerTile;
	int sightRange2 = 0;
	
	if (gConfig.Game.SightRange > 0)
	{
		sightRange2 = gConfig.Game.SightRange * gConfig.Game.SightRange;
	}

	centerTile.x = center.x / TILE_WIDTH - buffer->xStart;
	centerTile.y = center.y / TILE_HEIGHT - buffer->yStart;
	
	// First mark center tile and all adjacent tiles as visible
	// +-+-+-+
	// |V|V|V|
	// +-+-+-+
	// |V|C|V|
	// +-+-+-+
	// |V|V|V|  (C=center, V=visible)
	// +-+-+-+
	for (x = centerTile.x - 1; x < centerTile.x + 2; x++)
	{
		for (y = centerTile.y - 1; y < centerTile.y + 2; y++)
		{
			SetVisible(buffer, x, y);
		}
	}

	// Going outwards, mark tiles as visible if the inner tile
	// is visible and is not an obstruction
	y = centerTile.y;
	for (x = centerTile.x - 2; x >= 0; x--)
	{
		if (sightRange2 > 0 &&
			DistanceSquared(centerTile, Vec2iNew(x, y)) >= sightRange2)
		{
			break;
		}
		SetLineOfSight(buffer, x, y, 1, 0);
	}
	for (x = centerTile.x + 2; x < buffer->width; x++)
	{
		if (sightRange2 > 0 &&
			DistanceSquared(centerTile, Vec2iNew(x, y)) >= sightRange2)
		{
			break;
		}
		SetLineOfSight(buffer, x, y, -1, 0);
	}
	for (y = centerTile.y - 1; y >= 0; y--)
	{
		x = centerTile.x;
		if (sightRange2 > 0 &&
			DistanceSquared(centerTile, Vec2iNew(x, y)) >= sightRange2)
		{
			break;
		}
		SetLineOfSight(buffer, x, y, 0, 1);
		for (x = centerTile.x - 1; x >= 0; x--)
		{
			if (sightRange2 > 0 &&
				DistanceSquared(centerTile, Vec2iNew(x, y)) >= sightRange2)
			{
				break;
			}
			SetLineOfSight(buffer, x, y, 1, 1);
		}
		for (x = centerTile.x + 1; x < buffer->width; x++)
		{
			if (sightRange2 > 0 &&
				DistanceSquared(centerTile, Vec2iNew(x, y)) >= sightRange2)
			{
				break;
			}
			SetLineOfSight(buffer, x, y, -1, 1);
		}
	}
	for (y = centerTile.y + 1; y < Y_TILES; y++)
	{
		x = centerTile.x;
		if (sightRange2 > 0 &&
			DistanceSquared(centerTile, Vec2iNew(x, y)) >= sightRange2)
		{
			break;
		}
		SetLineOfSight(buffer, x, y, 0, -1);
		for (x = centerTile.x - 1; x >= 0; x--)
		{
			if (sightRange2 > 0 &&
				DistanceSquared(centerTile, Vec2iNew(x, y)) >= sightRange2)
			{
				break;
			}
			SetLineOfSight(buffer, x, y, 1, -1);
		}
		for (x = centerTile.x + 1; x < buffer->width; x++)
		{
			if (sightRange2 > 0 &&
				DistanceSquared(centerTile, Vec2iNew(x, y)) >= sightRange2)
			{
				break;
			}
			SetLineOfSight(buffer, x, y, -1, -1);
		}
	}
}


// Three types of tile drawing, based on line of sight:
// Unvisited: black
// Out of sight: dark, or if fog disabled, black
// In sight: full color
static color_t GetTileLOSMask(Tile *tile)
{
	if (!tile->isVisited)
	{
		return colorBlack;
	}
	if (tile->flags & MAPTILE_OUT_OF_SIGHT)
	{
		if (gConfig.Game.Fog)
		{
			color_t mask = { 96, 96, 96, 255 };
			return mask;
		}
		else
		{
			return colorBlack;
		}
	}
	return colorWhite;
}

void DrawWallColumn(int y, Vec2i pos, Tile *tile)
{
	while (y >= 0 && (tile->flags & MAPTILE_IS_WALL))
	{
		BlitMasked(
			&gGraphicsDevice,
			tile->pic,
			pos,
			GetTileLOSMask(tile),
			0);
		pos.y -= TILE_HEIGHT;
		tile -= X_TILES;
		y--;
	}
}


void DrawFloor(DrawBuffer *b, Vec2i offset);
void DrawDebris(DrawBuffer *b, Vec2i offset);
void DrawWallsAndThings(DrawBuffer *b, Vec2i offset);

void DrawBufferDraw(DrawBuffer *b, Vec2i offset)
{
	// First draw the floor tiles (which do not obstruct anything)
	DrawFloor(b, offset);
	// Then draw debris (wrecks)
	DrawDebris(b, offset);
	// Now draw walls and (non-wreck) things in proper order
	DrawWallsAndThings(b, offset);
}

void DrawFloor(DrawBuffer *b, Vec2i offset)
{
	int x, y;
	Vec2i pos;
	Tile *tile = &b->tiles[0][0];
	for (y = 0, pos.y = b->dy + offset.y;
		 y < Y_TILES;
		 y++, pos.y += TILE_HEIGHT)
	{
		for (x = 0, pos.x = b->dx + offset.x;
			 x < b->width;
			 x++, tile++, pos.x += TILE_WIDTH)
		{
			if (tile->pic != NULL && PicIsNotNone(tile->pic) &&
				!(tile->flags & (MAPTILE_IS_WALL | MAPTILE_OFFSET_PIC)))
			{
				BlitMasked(
					&gGraphicsDevice,
					tile->pic,
					pos,
					GetTileLOSMask(tile),
					0);
			}
		}
		tile += X_TILES - b->width;
	}
}

void AddItemToDisplayList(TTileItem * t, TTileItem **list);

void DrawDebris(DrawBuffer *b, Vec2i offset)
{
	int x, y;
	Tile *tile = &b->tiles[0][0];
	for (y = 0; y < Y_TILES; y++)
	{
		TTileItem *displayList = NULL;
		TTileItem *t;
		for (x = 0; x < b->width; x++, tile++)
		{
			for (t = tile->things; t; t = t->next)
			{
				if (t->flags & TILEITEM_IS_WRECK)
				{
					AddItemToDisplayList(t, &displayList);
				}
			}
		}
		for (t = displayList; t; t = t->nextToDisplay)
		{
			(*(t->drawFunc))(
				t->x - b->xTop + offset.x, t->y - b->yTop + offset.y, t->data);
		}
		tile += X_TILES - b->width;
	}
}

void DrawWallsAndThings(DrawBuffer *b, Vec2i offset)
{
	int x, y;
	Vec2i pos;
	Tile *tile = &b->tiles[0][0];
	pos.y = b->dy + cWallOffset.dy + offset.y;
	for (y = 0; y < Y_TILES; y++, pos.y += TILE_HEIGHT)
	{
		TTileItem *displayList = NULL;
		TTileItem *t;
		pos.x = b->dx + cWallOffset.dx + offset.x;
		for (x = 0; x < b->width; x++, tile++, pos.x += TILE_WIDTH)
		{
			if (tile->flags & MAPTILE_IS_WALL)
			{
				if (!(tile->flags & MAPTILE_DELAY_DRAW))
				{
					DrawWallColumn(y, pos, tile);
				}
			}
			else if (tile->flags & MAPTILE_OFFSET_PIC)
			{
				// Drawing doors
				BlitMasked(
					&gGraphicsDevice,
					&tile->picAlt,
					pos,
					GetTileLOSMask(tile),
					0);
			}
			for (t = tile->things; t; t = t->next)
			{
				if (!(t->flags & TILEITEM_IS_WRECK))
				{
					AddItemToDisplayList(t, &displayList);
				}
			}
		}
		for (t = displayList; t; t = t->nextToDisplay)
		{
			(*(t->drawFunc))(
				t->x - b->xTop + offset.x, t->y - b->yTop + offset.y, t->data);
		}
		tile += X_TILES - b->width;
	}
}

void AddItemToDisplayList(TTileItem * t, TTileItem **list)
{
	TTileItem *l;
	TTileItem *lPrev;
	t->nextToDisplay = NULL;
	for (lPrev = NULL, l = *list; l; lPrev = l, l = l->nextToDisplay)
	{
		// draw in Y order
		if (l->y >= t->y)
		{
			break;
		}
	}
	t->nextToDisplay = l;
	if (lPrev)
	{
		lPrev->nextToDisplay = t;
	}
	else
	{
		*list = t;
	}
}

void DrawCharacterSimple(
	Character *c, Vec2i pos,
	direction_e dir, int state,
	int gunPic, gunstate_e gunState, TranslationTable *table)
{
	TOffsetPic body, head, gun;
	TOffsetPic pic1, pic2, pic3;
	direction_e headDir = dir;
	int headState = state;
	int bodyType = c->looks.armedBody;
	if (gunState == GUNSTATE_FIRING || gunState == GUNSTATE_RECOIL)
	{
		headState = STATE_COUNT + gunState - GUNSTATE_FIRING;
	}
	if (state == STATE_IDLELEFT)
	{
		headDir = (direction_e)((dir + 7) % 8);
	}
	else if (state == STATE_IDLERIGHT)
	{
		headDir = (direction_e)((dir + 1) % 8);
	}

	if (gunPic < 0)
	{
		bodyType = c->looks.unarmedBody;
	}

	body.dx = cBodyOffset[bodyType][dir].dx;
	body.dy = cBodyOffset[bodyType][dir].dy;
	body.picIndex = cBodyPic[bodyType][dir][state];

	head.dx =
		cNeckOffset[bodyType][headDir].dx +
		cHeadOffset[c->looks.face][headDir].dx;
	head.dy =
		cNeckOffset[bodyType][headDir].dy +
		cHeadOffset[c->looks.face][headDir].dy;
	head.picIndex = cHeadPic[c->looks.face][headDir][headState];

	gun.picIndex = -1;
	if (gunPic >= 0)
	{
		gun.dx =
		    cGunHandOffset[bodyType][dir].dx +
		    cGunPics[gunPic][dir][gunState].dx;
		gun.dy =
		    cGunHandOffset[bodyType][dir].dy +
		    cGunPics[gunPic][dir][gunState].dy;
		gun.picIndex = cGunPics[gunPic][dir][gunState].picIndex;
	}

	switch (dir)
	{
	case DIRECTION_UP:
	case DIRECTION_UPRIGHT:
		pic1 = gun;
		pic2 = head;
		pic3 = body;
		break;

	case DIRECTION_RIGHT:
	case DIRECTION_DOWNRIGHT:
	case DIRECTION_DOWN:
	case DIRECTION_DOWNLEFT:
		pic1 = body;
		pic2 = head;
		pic3 = gun;
		break;

	case DIRECTION_LEFT:
	case DIRECTION_UPLEFT:
		pic1 = gun;
		pic2 = body;
		pic3 = head;
		break;
	default:
		assert(0 && "invalid direction");
		return;
	}

	if (pic1.picIndex >= 0)
	{
		Blit(
			pos.x + pic1.dx, pos.y + pic1.dy,
			PicManagerGetOldPic(&gPicManager, pic1.picIndex),
			table, BLIT_TRANSPARENT);
	}
	if (pic2.picIndex >= 0)
	{
		Blit(
			pos.x + pic2.dx, pos.y + pic2.dy,
			PicManagerGetOldPic(&gPicManager, pic2.picIndex),
			table, BLIT_TRANSPARENT);
	}
	if (pic3.picIndex >= 0)
	{
		Blit(
			pos.x + pic3.dx, pos.y + pic3.dy,
			PicManagerGetOldPic(&gPicManager, pic3.picIndex),
			table, BLIT_TRANSPARENT);
	}
}

void DisplayPlayer(int x, const char *name, Character *c, int editingName)
{
	Vec2i pos = Vec2iNew(x, gGraphicsDevice.cachedConfig.ResolutionHeight / 10);
	Vec2i playerPos = Vec2iAdd(pos, Vec2iNew(20, 36));

	if (editingName)
	{
		char s[22];
		sprintf(s, "%c%s%c", '\020', name, '\021');
		CDogsTextStringAt(pos.x, pos.y, s);
	}
	else
	{
		CDogsTextStringAt(pos.x, pos.y, name);
	}

	DrawCharacterSimple(
		c, playerPos,
		DIRECTION_DOWN, STATE_IDLE, -1, GUNSTATE_READY, &c->table);
}

void DisplayCharacter(int x, int y, Character *c, int hilite, int showGun)
{
	DrawCharacterSimple(
		c, Vec2iNew(x, y),
		DIRECTION_DOWN, STATE_IDLE, -1, GUNSTATE_READY, &c->table);
	if (hilite)
	{
		CDogsTextGoto(x - 8, y - 16);
		CDogsTextChar('\020');
		if (showGun)
		{
			CDogsTextGoto(x - 8, y + 8);
			CDogsTextString(gGunDescriptions[c->gun].gunName);
		}
	}
}
