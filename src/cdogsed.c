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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <SDL.h>

#include <cdogs/actors.h>
#include <cdogs/automap.h>
#include <cdogs/config.h>
#include <cdogs/draw.h>
#include <cdogs/drawtools.h>
#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/grafx.h>
#include <cdogs/keyboard.h>
#include <cdogs/mission.h>
#include <cdogs/objs.h>
#include <cdogs/palette.h>
#include <cdogs/pic_manager.h>
#include <cdogs/text.h>
#include <cdogs/triggers.h>
#include <cdogs/utils.h>

#include "charsed.h"
#include "editor_ui.h"
#include "ui_object.h"


#define TH  8


// Mouse click areas:
UICollection sMainObjs;
UICollection sCampaignObjs;
UICollection sMissionObjs;
UICollection sWeaponObjs;
UICollection sMapItemObjs;
UICollection sObjectiveObjs;
UICollection sCharacterObjs;

// zero-terminated list of UI objects to detect clicks for
static UICollection *sObjs1 = NULL;
static UICollection *sObjs2 = NULL;

int TryGetClickObj(Vec2i pos, UIObject **out)
{
	return
		(sObjs1 && UITryGetObject(sObjs1, pos, out)) ||
		(sObjs2 && UITryGetObject(sObjs2, pos, out));
}



// Globals

struct Mission *currentMission;
static char lastFile[CDOGS_FILENAME_MAX];



// Code...

void DisplayCDogsText(int x, int y, const char *text, int hilite, int editable)
{
	CDogsTextGoto(x, y);
	if (editable) {
		if (hilite)
			CDogsTextCharWithTable('\020', &tableFlamed);
		else
			CDogsTextChar('\020');
	}

	if (hilite && !editable)
		CDogsTextStringWithTable(text, &tableFlamed);
	else
		CDogsTextString(text);

	if (editable) {
		if (hilite)
			CDogsTextCharWithTable('\021', &tableFlamed);
		else
			CDogsTextChar('\021');
	}
}

void DrawObjectiveInfo(int idx, int y, int xc)
{
	TOffsetPic pic;
	TranslationTable *table = NULL;
	int i;
	const char *typeCDogsText;
	char s[50];
	Character *cd;
	CharacterStore *store = &gCampaign.Setting.characters;
	struct MissionObjective *mo = &currentMission->objectives[idx];

	switch (mo->type)
	{
	case OBJECTIVE_KILL:
		typeCDogsText = "Kill";
		if (store->specialCount == 0)
		{
			cd = &store->players[0];
		}
		else
		{
			cd = CharacterStoreGetSpecial(store, 0);
		}
		i = cd->looks.face;
		table = &cd->table;
		pic.picIndex = cHeadPic[i][DIRECTION_DOWN][STATE_IDLE];
		pic.dx = cHeadOffset[i][DIRECTION_DOWN].dx;
		pic.dy = cHeadOffset[i][DIRECTION_DOWN].dy;
		break;
	case OBJECTIVE_RESCUE:
		typeCDogsText = "Rescue";
		if (store->prisonerCount == 0)
		{
			cd = &store->players[0];
		}
		else
		{
			cd = CharacterStoreGetPrisoner(store, 0);
		}
		i = cd->looks.face;
		table = &cd->table;
		pic.picIndex = cHeadPic[i][DIRECTION_DOWN][STATE_IDLE];
		pic.dx = cHeadOffset[i][DIRECTION_DOWN].dx;
		pic.dy = cHeadOffset[i][DIRECTION_DOWN].dy;
		break;
	case OBJECTIVE_COLLECT:
		typeCDogsText = "Collect";
		i = gMission.objectives[idx].pickupItem;
		pic = cGeneralPics[i];
		break;
	case OBJECTIVE_DESTROY:
		typeCDogsText = "Destroy";
		i = gMission.objectives[idx].blowupObject->pic;
		pic = cGeneralPics[i];
		break;
	case OBJECTIVE_INVESTIGATE:
		typeCDogsText = "Explore";
		pic.dx = pic.dy = 0;
		pic.picIndex = -1;
		break;
	default:
		typeCDogsText = "???";
		i = gMission.objectives[idx].pickupItem;
		pic = cGeneralPics[i];
		break;
	}

	DisplayCDogsText(20, y, typeCDogsText, xc == XC_TYPE, 0);

	if (pic.picIndex >= 0)
	{
		DrawTTPic(
			75 + pic.dx, y + 8 + pic.dy,
			PicManagerGetOldPic(&gPicManager, pic.picIndex), table);
	}

	sprintf(s, "%d", mo->required);
	DisplayCDogsText(90, y, s, xc == XC_REQUIRED, 0);
	sprintf(s, "out of %d", mo->count);
	DisplayCDogsText(110, y, s, xc == XC_TOTAL, 0);

	if (!mo->flags)
	{
		DrawTextStringMasked(
			"normal", &gGraphicsDevice, Vec2iNew(150, y), colorGray);
	}
	else
	{
		sprintf(s, "%s %s %s %s %s",
			(mo->flags & OBJECTIVE_HIDDEN) ? "hidden" : "",
			(mo->flags & OBJECTIVE_POSKNOWN) ? "pos.known" : "",
			(mo->flags & OBJECTIVE_HIACCESS) ? "access" : "",
			(mo->flags & OBJECTIVE_UNKNOWNCOUNT) ? "no-count" : "",
			(mo->flags & OBJECTIVE_NOACCESS) ? "no-access" : "");
		DrawTextStringMasked(
			s, &gGraphicsDevice, Vec2iNew(150, y),
			xc == XC_FLAGS ? colorRed : colorWhite);
	}
}

static void ShowWeaponStatus(int x, int y, int weapon, int xc)
{
	DisplayFlag(
		x,
		y,
		gGunDescriptions[weapon].name,
		(currentMission->weaponSelection & (1 << weapon)) != 0,
		xc == weapon);
}

static void ListWeapons(int y, int xc)
{
	ShowWeaponStatus(10, y, 0, xc);
	ShowWeaponStatus(10, y + TH, 1, xc);
	ShowWeaponStatus(10, y + 2 * TH, 2, xc);
	ShowWeaponStatus(10, y + 3 * TH, 3, xc);

	ShowWeaponStatus(100, y, 4, xc);
	ShowWeaponStatus(100, y + TH, 5, xc);
	ShowWeaponStatus(100, y + 2 * TH, 6, xc);
	ShowWeaponStatus(100, y + 3 * TH, 7, xc);

	ShowWeaponStatus(190, y, 8, xc);
	ShowWeaponStatus(190, y + TH, 9, xc);
	ShowWeaponStatus(190, y + 2 * TH, 10, xc);
}

static void DrawEditableTextWithEmptyHint(
	Vec2i pos, char *text, char *hint, int isHighlighted)
{
	int isEmptyText = strlen(text) == 0;
	color_t bracketMask = isHighlighted ? colorRed : colorWhite;
	color_t textMask = isEmptyText ? colorGray : colorWhite;
	if (isEmptyText)
	{
		text = hint;
	}
	pos = DrawTextCharMasked('\020', &gGraphicsDevice, pos, bracketMask);
	pos = DrawTextStringMasked(text, &gGraphicsDevice, pos, textMask);
	pos = DrawTextCharMasked('\021', &gGraphicsDevice, pos, bracketMask);
}

static void DisplayMapItem(int x, int y, TMapObject * mo, int density, int hilite)
{
	char s[10];

	const TOffsetPic *pic = &cGeneralPics[mo->pic];
	DrawTPic(
		x + pic->dx, y + pic->dy,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));

	if (hilite) {
		CDogsTextGoto(x - 8, y - 4);
		CDogsTextChar('\020');
	}
	sprintf(s, "%d", density);
	CDogsTextGoto(x - 8, y + 5);
	CDogsTextString(s);
}

static void DrawStyleArea(
	Vec2i pos,
	const char *name,
	GraphicsDevice *device,
	PicPaletted *pic,
	int index, int count,
	int isHighlighted)
{
	char buf[16];
	DisplayCDogsText(pos.x, pos.y, name, isHighlighted, 0);
	DrawPic(
		pos.x, pos.y + TH,
		pic);
	// Display style index and count, right aligned
	sprintf(buf, "%d/%d", index + 1, count);
	DrawTextStringMasked(
		buf,
		device,
		Vec2iNew(pos.x + 28 - TextGetStringWidth(buf), pos.y + 25),
		colorGray);
}

static Vec2i GetMouseTile(GraphicsDevice *g, EventHandlers *e)
{
	int w = g->cachedConfig.ResolutionWidth;
	int h = g->cachedConfig.ResolutionHeight;
	Vec2i mapSize = Vec2iNew(
		currentMission->mapWidth * TILE_WIDTH,
		currentMission->mapHeight * TILE_HEIGHT);
	Vec2i mapPos = Vec2iNew((w - mapSize.x) / 2, (h - mapSize.y) / 2);
	return Vec2iNew(
		(e->mouse.currentPos.x - mapPos.x) / TILE_WIDTH,
		(e->mouse.currentPos.y - mapPos.y) / TILE_HEIGHT);
}

static void SwapCursorTile(Vec2i mouseTile)
{
	static Vec2i cursorTilePos = { -1, -1 };
	static Tile cursorTile;
	Tile *t;

	// Convert the tile coordinates to map tile coordinates
	// The map is centered, i.e. edges are empty
	// TODO: refactor map to use clearer coordinates
	mouseTile.x += (XMAX - currentMission->mapWidth) / 2;
	mouseTile.y += (YMAX - currentMission->mapHeight) / 2;

	// Draw the cursor tile by replacing it with the map tile at the
	// cursor position
	// If moving to a new tile, restore the last tile,
	// and swap with the new tile
	if (cursorTilePos.x >= 0 && cursorTilePos.y >= 0)
	{
		// restore
		memcpy(
			MapGetTile(&gMap, cursorTilePos),
			&cursorTile,
			sizeof cursorTile);
	}
	// swap
	cursorTilePos = mouseTile;
	t = MapGetTile(&gMap, cursorTilePos);
	memcpy(&cursorTile, t, sizeof cursorTile);
	// Set cursor tile properties
	t->pic = PicManagerGetFromOld(
		&gPicManager, cWallPics[currentMission->wallStyle][WALL_SINGLE]);
	t->picAlt = picNone;
	t->flags = MAPTILE_IS_WALL;
	t->isVisited = 1;
	t->things = NULL;
}

static void DisplayMission(int xc, int yc, int y)
{
	char s[128];
	struct EditorInfo ei;

	y += CDogsTextHeight() + 3;
	DrawEditableTextWithEmptyHint(
		Vec2iNew(25, y),
		currentMission->title, "(Mission title)",
		yc == YC_MISSIONTITLE && xc == XC_MISSIONTITLE);

	y += CDogsTextHeight() + 2;

	sprintf(s, "Width: %d", currentMission->mapWidth);
	DisplayCDogsText(20, y, s, yc == YC_MISSIONPROPS && xc == XC_WIDTH, 0);

	sprintf(s, "Height: %d", currentMission->mapHeight);
	DisplayCDogsText(60, y, s, yc == YC_MISSIONPROPS && xc == XC_HEIGHT, 0);

	sprintf(s, "Walls: %d", currentMission->wallCount);
	DisplayCDogsText(100, y, s, yc == YC_MISSIONPROPS && xc == XC_WALLCOUNT, 0);

	sprintf(s, "Len: %d", currentMission->wallLength);
	DisplayCDogsText(140, y, s, yc == YC_MISSIONPROPS && xc == XC_WALLLENGTH, 0);

	sprintf(s, "Rooms: %d", currentMission->roomCount);
	DisplayCDogsText(180, y, s, yc == YC_MISSIONPROPS && xc == XC_ROOMCOUNT, 0);

	sprintf(s, "Sqr: %d", currentMission->squareCount);
	DisplayCDogsText(220, y, s, yc == YC_MISSIONPROPS && xc == XC_SQRCOUNT, 0);

	sprintf(s, "Dens: %d", currentMission->baddieDensity);
	DisplayCDogsText(260, y, s, yc == YC_MISSIONPROPS && xc == XC_DENSITY, 0);

	y += CDogsTextHeight();

	DrawStyleArea(
		Vec2iNew(20, y),
		"Wall",
		&gGraphicsDevice,
		PicManagerGetOldPic(&gPicManager, cWallPics[currentMission->wallStyle % WALL_STYLE_COUNT][WALL_SINGLE]),
		currentMission->wallStyle, WALL_STYLE_COUNT,
		yc == YC_MISSIONLOOKS && xc == XC_WALL);
	DrawStyleArea(
		Vec2iNew(50, y),
		"Floor",
		&gGraphicsDevice,
		PicManagerGetOldPic(&gPicManager, cFloorPics[currentMission->floorStyle % FLOOR_STYLE_COUNT][FLOOR_NORMAL]),
		currentMission->floorStyle, FLOOR_STYLE_COUNT,
		yc == YC_MISSIONLOOKS && xc == XC_FLOOR);
	DrawStyleArea(
		Vec2iNew(80, y),
		"Rooms",
		&gGraphicsDevice,
		PicManagerGetOldPic(&gPicManager, cRoomPics[currentMission->roomStyle % ROOMFLOOR_COUNT][ROOMFLOOR_NORMAL]),
		currentMission->roomStyle, ROOMFLOOR_COUNT,
		yc == YC_MISSIONLOOKS && xc == XC_ROOM);
	GetEditorInfo(&ei);
	DrawStyleArea(
		Vec2iNew(110, y),
		"Doors",
		&gGraphicsDevice,
		PicManagerGetOldPic(&gPicManager, cGeneralPics[gMission.doorPics[0].horzPic].picIndex),
		currentMission->doorStyle, ei.doorCount,
		yc == YC_MISSIONLOOKS && xc == XC_DOORS);
	DrawStyleArea(
		Vec2iNew(140, y),
		"Keys",
		&gGraphicsDevice,
		PicManagerGetOldPic(&gPicManager, cGeneralPics[gMission.keyPics[0]].picIndex),
		currentMission->keyStyle, ei.keyCount,
		yc == YC_MISSIONLOOKS && xc == XC_KEYS);
	DrawStyleArea(
		Vec2iNew(170, y),
		"Exit",
		&gGraphicsDevice,
		PicManagerGetOldPic(&gPicManager, gMission.exitPic),
		currentMission->exitStyle, ei.exitCount,
		yc == YC_MISSIONLOOKS && xc == XC_EXIT);

	sprintf(s, "Walls: %s", RangeName(currentMission->wallRange));
	DisplayCDogsText(
		200, y, s, yc == YC_MISSIONLOOKS && xc == XC_COLOR1, 0);
	sprintf(s, "Floor: %s", RangeName(currentMission->floorRange));
	DisplayCDogsText(
		200, y + TH, s, yc == YC_MISSIONLOOKS && xc == XC_COLOR2, 0);
	sprintf(s, "Rooms: %s", RangeName(currentMission->roomRange));
	DisplayCDogsText(
		200, y + 2 * TH, s, yc == YC_MISSIONLOOKS && xc == XC_COLOR3, 0);
	sprintf(s, "Extra: %s", RangeName(currentMission->altRange));
	DisplayCDogsText(
		200, y + 3 * TH, s, yc == YC_MISSIONLOOKS && xc == XC_COLOR4, 0);

	y += TH + 25;

	DisplayCDogsText(20, y, "Mission description", yc == YC_MISSIONDESC, 0);
	y += CDogsTextHeight();

	sprintf(s, "Characters (%d/%d)", currentMission->baddieCount, BADDIE_MAX);
	DisplayCDogsText(20, y, s, yc == YC_CHARACTERS, 0);
	y += CDogsTextHeight();

	sprintf(
		s,
		"Mission objective characters (%d/%d)",
		currentMission->specialCount,
		SPECIAL_MAX);
	DisplayCDogsText(20, y, s, yc == YC_SPECIALS, 0);
	y += CDogsTextHeight();

	sprintf(s, "Available weapons (%d/%d)", gMission.weaponCount, WEAPON_MAX);
	DisplayCDogsText(20, y, s, yc == YC_WEAPONS, 0);
	y += CDogsTextHeight();

	sprintf(s, "Map items (%d/%d)", gMission.objectCount, ITEMS_MAX);
	DisplayCDogsText(20, y, s, yc == YC_ITEMS, 0);
	y += CDogsTextHeight() + 2;

	if (currentMission->objectiveCount)
	{
		int i;
		for (i = 0; i < currentMission->objectiveCount; i++)
		{
			DrawEditableTextWithEmptyHint(
				Vec2iNew(20, y),
				currentMission->objectives[i].description,
				"(Objective description)",
				yc - YC_OBJECTIVES == i);
			y += CDogsTextHeight();
		}
	}
	else
	{
		DisplayCDogsText(
			20, y, "-- mission objectives --", yc == YC_OBJECTIVES, 0);
	}
}

static void MakeBackground(
	GraphicsDevice *g, GraphicsConfig *config, int mission)
{
	int i;
	// Clear background first
	for (i = 0; i < GraphicsGetScreenSize(&g->cachedConfig); i++)
	{
		g->buf[i] = PixelFromColor(g, colorBlack);
	}
	GrafxMakeBackground(g, config, tintDarker, mission);
}

static void Display(int mission, int xc, int yc, int willDisplayAutomap)
{
	char s[128];
	int y = 5;
	int i;
	int w = gGraphicsDevice.cachedConfig.ResolutionWidth;
	int h = gGraphicsDevice.cachedConfig.ResolutionHeight;

	sObjs2 = NULL;

	if (currentMission)
	{
		Vec2i mouseTile = GetMouseTile(&gGraphicsDevice, &gEventHandlers);
		int isMouseTileValid =
			mouseTile.x >= 0 && mouseTile.x < currentMission->mapWidth &&
			mouseTile.y >= 0 && mouseTile.y < currentMission->mapHeight;
		// Re-make the background if the resolution has changed
		if (gEventHandlers.HasResolutionChanged)
		{
			MakeBackground(&gGraphicsDevice, &gConfig.Graphics, mission);
		}
		if (isMouseTileValid)
		{
			SwapCursorTile(mouseTile);
			GrafxDrawBackground(
				&gGraphicsDevice, &gConfig.Graphics, tintDarker);
		}
		GraphicsBlitBkg(&gGraphicsDevice);
		sprintf(s, "Mission %d/%d", mission + 1, gCampaign.Setting.missionCount);
		DrawTextStringMasked(
			s, &gGraphicsDevice, Vec2iNew(270, y),
			yc == YC_MISSIONINDEX ? colorRed : colorWhite);
		DisplayMission(xc, yc, y);
		if (isMouseTileValid)
		{
			sprintf(s, "(%d, %d)", mouseTile.x, mouseTile.y);
			DrawTextString(s, &gGraphicsDevice, Vec2iNew(w - 40, h - 16));
		}
	}
	else
	{
		for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
		{
			gGraphicsDevice.buf[i] = LookupPalette(58);
		}
		if (gCampaign.Setting.missionCount)
		{
			sprintf(s, "End/%d", gCampaign.Setting.missionCount);
			DrawTextStringMasked(
				s, &gGraphicsDevice, Vec2iNew(270, y),
				yc == YC_MISSIONINDEX ? colorRed : colorWhite);
		}
	}

	if (fileChanged)
	{
		DrawTPic(10, y, PicManagerGetOldPic(&gPicManager, 221));
	}

	DrawTextString(
		"Press F1 for help",
		&gGraphicsDevice,
		Vec2iNew(20, h - 20 - CDogsTextHeight()));

	y = 170;

	switch (yc)
	{
	case YC_CAMPAIGNTITLE:
		sObjs2 = &sCampaignObjs;
		break;

	case YC_MISSIONTITLE:
		DrawEditableTextWithEmptyHint(
			Vec2iNew(20, 150),
			currentMission->song, "(Mission song)",
			yc == YC_MISSIONTITLE && xc == XC_MUSICFILE);
		sObjs2 = &sMissionObjs;
		break;

	case YC_MISSIONDESC:
		break;

	case YC_CHARACTERS:
		CDogsTextStringAt(5, 190, "Use Insert, Delete and PageUp/PageDown");
		if (!currentMission)
		{
			break;
		}
		for (i = 0; i < currentMission->baddieCount; i++)
		{
			DisplayCharacter(
				10 + 20 * i, y,
				gCampaign.Setting.characters.baddies[i], xc == i, 1);
		}
		sObjs2 = &sCharacterObjs;
		break;

	case YC_SPECIALS:
		CDogsTextStringAt(5, 190, "Use Insert, Delete and PageUp/PageDown");
		if (!currentMission)
		{
			break;
		}
		for (i = 0; i < currentMission->specialCount; i++)
		{
			DisplayCharacter(
				20 + 20 * i, y,
				gCampaign.Setting.characters.specials[i],
				xc == i,
				1);
		}
		sObjs2 = &sCharacterObjs;
		break;

	case YC_ITEMS:
		CDogsTextStringAt(
			5, 190, "Use Insert, Delete and PageUp/PageDown");
		if (!currentMission)
		{
			break;
		}
		sObjs2 = &sMapItemObjs;
		for (i = 0; i < currentMission->itemCount; i++)
		{
			DisplayMapItem(
				10 + 10 + 20 * i, y,
				gMission.mapObjects[i],
				currentMission->itemDensity[i],
				xc == i);
		}
		break;

	case YC_WEAPONS:
		if (!currentMission)
		{
			break;
		}
		sObjs2 = &sWeaponObjs;
		ListWeapons(150, xc);
		break;

	default:
		if (currentMission && yc >= YC_OBJECTIVES &&
			yc - YC_OBJECTIVES < currentMission->objectiveCount)
		{
			CDogsTextStringAt(
				5, 190, "Use Insert, Delete and PageUp/PageDown");
			sObjs2 = &sObjectiveObjs;
			DrawObjectiveInfo(yc - YC_OBJECTIVES, y, xc);
		}
		break;
	}

	UICollectionDraw(sObjs1, &gGraphicsDevice);
	UICollectionDraw(sObjs2, &gGraphicsDevice);

	if (willDisplayAutomap)
	{
		AutomapDraw(AUTOMAP_FLAGS_SHOWALL);
	}
	else
	{
		UIObject *o;
		if (TryGetClickObj(gEventHandlers.mouse.currentPos, &o) && o->Tooltip)
		{
			Vec2i tooltipPos = Vec2iAdd(
				gEventHandlers.mouse.currentPos, Vec2iNew(10, 10));
			DrawTooltip(&gGraphicsDevice, tooltipPos, o->Tooltip);
		}
		MouseDraw(&gEventHandlers.mouse);
	}
	BlitFlip(&gGraphicsDevice, &gConfig.Graphics);
}

static int Change(int yc, int xc, int d, int *mission)
{
	struct EditorInfo edInfo;
	int limit;
	int isChanged = 0;

	if (yc == YC_MISSIONINDEX) {
		*mission += d;
		*mission = CLAMP(*mission, 0, gCampaign.Setting.missionCount);
		return 0;
	}

	if (!currentMission)
		return 0;

	GetEditorInfo(&edInfo);

	switch (yc) {
	case YC_MISSIONPROPS:
		switch (xc) {
		case XC_WIDTH:
			currentMission->mapWidth = CLAMP(currentMission->mapWidth + d, 16, XMAX);
			break;
		case XC_HEIGHT:
			currentMission->mapHeight = CLAMP(currentMission->mapHeight + d, 16, YMAX);
			break;
		case XC_WALLCOUNT:
			currentMission->wallCount = CLAMP(currentMission->wallCount + d, 0, 200);
			break;
		case XC_WALLLENGTH:
			currentMission->wallLength = CLAMP(currentMission->wallLength + d, 1, 100);
			break;
		case XC_ROOMCOUNT:
			currentMission->roomCount = CLAMP(currentMission->roomCount + d, 0, 100);
			break;
		case XC_SQRCOUNT:
			currentMission->squareCount = CLAMP(currentMission->squareCount + d, 0, 100);
			break;
		case XC_DENSITY:
			currentMission->baddieDensity = CLAMP(currentMission->baddieDensity + d, 0, 100);
			break;
		}
		isChanged = 1;
		break;

	case YC_MISSIONLOOKS:
		switch (xc) {
		case XC_WALL:
			currentMission->wallStyle = CLAMP_OPPOSITE(
				(int)(currentMission->wallStyle + d), 0, WALL_STYLE_COUNT - 1);
			break;
		case XC_FLOOR:
			currentMission->floorStyle = CLAMP_OPPOSITE(
				(int)(currentMission->floorStyle + d),
				0,
				FLOOR_STYLE_COUNT - 1);
			break;
		case XC_ROOM:
			currentMission->roomStyle = CLAMP_OPPOSITE(
				(int)(currentMission->roomStyle + d), 0, ROOMFLOOR_COUNT - 1);
			break;
		case XC_DOORS:
			currentMission->doorStyle =
				CLAMP_OPPOSITE(currentMission->doorStyle + d, 0, edInfo.doorCount - 1);
			break;
		case XC_KEYS:
			currentMission->keyStyle = CLAMP_OPPOSITE(
				currentMission->keyStyle + d, 0, edInfo.keyCount - 1);
			break;
		case XC_EXIT:
			currentMission->exitStyle = CLAMP_OPPOSITE(
				currentMission->exitStyle + d, 0, edInfo.exitCount - 1);
			break;
		case XC_COLOR1:
			currentMission->wallRange = CLAMP_OPPOSITE(
				currentMission->wallRange + d, 0, edInfo.rangeCount - 1);
			break;
		case XC_COLOR2:
			currentMission->floorRange = CLAMP_OPPOSITE(
				currentMission->floorRange + d, 0, edInfo.rangeCount - 1);
			break;
		case XC_COLOR3:
			currentMission->roomRange = CLAMP_OPPOSITE(
				currentMission->roomRange + d, 0, edInfo.rangeCount - 1);
			break;
		case XC_COLOR4:
			currentMission->altRange = CLAMP_OPPOSITE(
				currentMission->altRange + d, 0, edInfo.rangeCount - 1);
			break;
		}
		isChanged = 1;
		break;

	case YC_CHARACTERS:
		currentMission->baddies[xc] = CLAMP_OPPOSITE(
			currentMission->baddies[xc] + d,
			0,
			gCampaign.Setting.characters.otherCount - 1);
		gCampaign.Setting.characters.baddies[xc] =
			&gCampaign.Setting.characters.others[currentMission->baddies[xc]];
		isChanged = 1;
		break;

	case YC_SPECIALS:
		currentMission->specials[xc] = CLAMP_OPPOSITE(
			currentMission->specials[xc] + d,
			0,
			gCampaign.Setting.characters.otherCount - 1);
		gCampaign.Setting.characters.specials[xc] =
			&gCampaign.Setting.characters.others[currentMission->specials[xc]];
		isChanged = 1;
		break;

	case YC_WEAPONS:
		currentMission->weaponSelection ^= (1 << xc);
		isChanged = 1;
		break;

	case YC_ITEMS:
		if (gEventHandlers.keyboard.modState & KMOD_SHIFT)
		{
			currentMission->itemDensity[xc] =
				CLAMP(currentMission->itemDensity[xc] +  5 * d, 0, 512);
		}
		else
		{
			currentMission->items[xc] = CLAMP_OPPOSITE(
				currentMission->items[xc] + d, 0, edInfo.itemCount - 1);
		}
		isChanged = 1;
		break;

	default:
		if (yc >= YC_OBJECTIVES)
		{
			struct MissionObjective *objective =
				&currentMission->objectives[yc - YC_OBJECTIVES];
			switch (xc)
			{
			case XC_TYPE:
				objective->type = CLAMP_OPPOSITE(
						objective->type + d, 0, OBJECTIVE_INVESTIGATE);
				d = 0;
				// fallthrough

			case XC_INDEX:
				switch (objective->type)
				{
				case OBJECTIVE_COLLECT:
					limit = edInfo.pickupCount - 1;
					break;
				case OBJECTIVE_DESTROY:
					limit = edInfo.itemCount - 1;
					break;
				case OBJECTIVE_KILL:
				case OBJECTIVE_INVESTIGATE:
					limit = 0;
					break;
				case OBJECTIVE_RESCUE:
					limit = gCampaign.Setting.characters.otherCount - 1;
					break;
				default:
					// should never get here
					return 0;
				}
				objective->index =
					CLAMP_OPPOSITE(objective->index + d, 0, limit);
				isChanged = 1;
				break;

			case XC_REQUIRED:
				objective->required = CLAMP_OPPOSITE(
					objective->required + d, 0, MIN(100, objective->count));
				isChanged = 1;
				break;

			case XC_TOTAL:
				objective->count = CLAMP_OPPOSITE(
					objective->count + d, objective->required, 100);
				isChanged = 1;
				break;

			case XC_FLAGS:
				objective->flags = CLAMP_OPPOSITE(objective->flags + d, 0, 15);
				isChanged = 1;
				break;
			}
		}
		break;
	}
	return isChanged;
}

void InsertMission(int idx, struct Mission *mission)
{
	int i;
	CREALLOC(
		gCampaign.Setting.missions,
		sizeof *gCampaign.Setting.missions * (gCampaign.Setting.missionCount + 1));
	for (i = gCampaign.Setting.missionCount; i > idx; i--)
	{
		gCampaign.Setting.missions[i] = gCampaign.Setting.missions[i - 1];
	}
	if (mission)
	{
		gCampaign.Setting.missions[idx] = *mission;
	}
	else
	{
		memset(&gCampaign.Setting.missions[idx], 0, sizeof(struct Mission));
		strcpy(gCampaign.Setting.missions[idx].title, "");
		strcpy(gCampaign.Setting.missions[idx].description, "");
		gCampaign.Setting.missions[idx].mapWidth = 48;
		gCampaign.Setting.missions[idx].mapHeight = 48;
	}
	gCampaign.Setting.missionCount++;
}

void DeleteMission(int *idx)
{
	int i;

	if (*idx >= gCampaign.Setting.missionCount)
	{
		return;
	}

	for (i = *idx; i < gCampaign.Setting.missionCount - 1; i++)
	{
		gCampaign.Setting.missions[i] = gCampaign.Setting.missions[i + 1];
	}
	gCampaign.Setting.missionCount--;
	if (gCampaign.Setting.missionCount > 0 && *idx >= gCampaign.Setting.missionCount)
	{
		*idx = gCampaign.Setting.missionCount - 1;
	}
}

void AddObjective(void)
{
	currentMission->objectiveCount =
		CLAMP(currentMission->objectiveCount + 1, 0, OBJECTIVE_MAX);
}

void DeleteObjective(int idx)
{
	int i;

	currentMission->objectiveCount =
		CLAMP(currentMission->objectiveCount - 1, 0, OBJECTIVE_MAX);
	for (i = idx; i < currentMission->objectiveCount; i++)
	{
		currentMission->objectives[i] = currentMission->objectives[i + 1];
	}
}

void DeleteCharacter(int idx)
{
	int i;

	currentMission->baddieCount =
		CLAMP(currentMission->baddieCount - 1, 0, BADDIE_MAX);
	for (i = idx; i < currentMission->baddieCount; i++)
	{
		currentMission->baddies[i] = currentMission->baddies[i + 1];
	}
	CharacterStoreDeleteBaddie(&gCampaign.Setting.characters, idx);
}

void DeleteSpecial(int idx)
{
	int i;

	currentMission->specialCount =
		CLAMP(currentMission->specialCount - 1, 0, SPECIAL_MAX);
	for (i = idx; i < currentMission->specialCount; i++)
	{
		currentMission->specials[i] = currentMission->specials[i + 1];
	}
	CharacterStoreDeleteSpecial(&gCampaign.Setting.characters, idx);
}

void DeleteItem(int idx)
{
	int i;

	currentMission->itemCount =
		CLAMP(currentMission->itemCount - 1, 0, ITEMS_MAX);
	for (i = idx; i < currentMission->itemCount; i++)
	{
		currentMission->items[i] = currentMission->items[i + 1];
	}
}

static void Append(char *s, int maxlen, char c)
{
	size_t l = strlen(s);

	if ((int)l < maxlen)
	{
		s[l + 1] = 0;
		s[l] = c;
	}
}

static void Backspace(char *s)
{
	if (s[0])
		s[strlen(s) - 1] = 0;
}

static void AddChar(int xc, int yc, char c)
{
	if (yc == YC_CAMPAIGNTITLE) {
		switch (xc) {
		case XC_CAMPAIGNTITLE:
			Append(
				gCampaign.Setting.title,
				sizeof(gCampaign.Setting.title) - 1,
				c);
			break;
		case XC_AUTHOR:
			Append(
				gCampaign.Setting.author,
				sizeof(gCampaign.Setting.author) - 1,
				c);
			break;
		case XC_CAMPAIGNDESC:
			Append(
				gCampaign.Setting.description,
				sizeof(gCampaign.Setting.description) - 1,
				c);
			break;
		}
	}

	if (!currentMission)
		return;

	switch (yc) {
	case YC_MISSIONTITLE:
		if (xc == XC_MUSICFILE)
			Append(currentMission->song,
			       sizeof(currentMission->song) - 1, c);
		else
			Append(currentMission->title,
			       sizeof(currentMission->title) - 1, c);
		break;

	case YC_MISSIONDESC:
		Append(currentMission->description,
		       sizeof(currentMission->description) - 1, c);
		break;

	default:
		if (yc - YC_OBJECTIVES < currentMission->objectiveCount)
			Append(currentMission->
			       objectives[yc - YC_OBJECTIVES].description,
			       sizeof(currentMission->objectives[0].
				      description) - 1, c);
		break;
	}
}

static void DelChar(int xc, int yc)
{
	if (yc == YC_CAMPAIGNTITLE) {
		switch (xc) {
		case XC_CAMPAIGNTITLE:
			Backspace(gCampaign.Setting.title);
			break;
		case XC_AUTHOR:
			Backspace(gCampaign.Setting.author);
			break;
		case XC_CAMPAIGNDESC:
			Backspace(gCampaign.Setting.description);
			break;
		}
	}

	if (!currentMission)
		return;

	switch (yc) {
	case YC_MISSIONTITLE:
		if (xc == XC_MUSICFILE)
			Backspace(currentMission->song);
		else
			Backspace(currentMission->title);
		break;

	case YC_MISSIONDESC:
		Backspace(currentMission->description);
		break;

	default:
		if (yc - YC_OBJECTIVES < currentMission->objectiveCount)
			Backspace(currentMission->
				  objectives[yc -
					     YC_OBJECTIVES].description);
		break;
	}
}

static void AdjustYC(int *yc)
{
	if (currentMission != NULL)
	{
		if (currentMission->objectiveCount)
		{
			*yc = CLAMP_OPPOSITE(
				*yc, 0, YC_OBJECTIVES + currentMission->objectiveCount - 1);
		}
		else
		{
			*yc = CLAMP_OPPOSITE(*yc, 0, YC_OBJECTIVES);
		}
	}
	else
	{
		*yc = CLAMP_OPPOSITE(*yc, 0, YC_MISSIONINDEX);
	}
}

static void AdjustXC(int yc, int *xc)
{
	switch (yc)
	{
	case YC_CAMPAIGNTITLE:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_CAMPAIGNDESC);
		break;

	case YC_MISSIONTITLE:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_MUSICFILE);
		break;

	case YC_MISSIONPROPS:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_DENSITY);
		break;

	case YC_MISSIONLOOKS:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_COLOR4);
		break;

	case YC_CHARACTERS:
		if (currentMission && currentMission->baddieCount > 0)
		{
			*xc = CLAMP_OPPOSITE(*xc, 0, currentMission->baddieCount - 1);
		}
		break;

	case YC_SPECIALS:
		if (currentMission && currentMission->specialCount > 0)
		{
			*xc = CLAMP_OPPOSITE(*xc, 0, currentMission->specialCount - 1);
		}
		break;

	case YC_ITEMS:
		if (currentMission && currentMission->itemCount > 0)
		{
			*xc = CLAMP_OPPOSITE(*xc, 0, currentMission->itemCount - 1);
		}
		break;

	case YC_WEAPONS:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_MAXWEAPONS);
		break;

	default:
		if (yc >= YC_OBJECTIVES)
		{
			*xc = CLAMP_OPPOSITE(*xc, 0, XC_FLAGS);
		}
		break;
	}
}

static void MoveSelection(int isForward, int *y, int *x)
{
	if (isForward)
	{
		(*y)++;
	}
	else
	{
		(*y)--;
	}
	AdjustYC(y);
	AdjustXC(*y, x);
}

static void Setup(int idx, int buildTables)
{
	if (idx >= gCampaign.Setting.missionCount)
	{
		currentMission = NULL;
		return;
	}
	currentMission = &gCampaign.Setting.missions[idx];
	SetupMission(idx, buildTables, &gCampaign);
	MakeBackground(&gGraphicsDevice, &gConfig.Graphics, idx);
}

static void Open(void)
{
	char filename[CDOGS_FILENAME_MAX];
	int c;
	
	strcpy(filename, lastFile);
	for (;;)
	{
		int i;
		for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
		{
			gGraphicsDevice.buf[i] = LookupPalette(58);
		}
		CDogsTextStringAt(125, 50, "Open file:");
		CDogsTextGoto(125, 50 + CDogsTextHeight());
		CDogsTextChar('\020');
		CDogsTextString(filename);
		CDogsTextChar('\021');
		BlitFlip(&gGraphicsDevice, &gConfig.Graphics);
		
		c = GetKey(&gEventHandlers);
		switch (c)
		{
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				if (!filename[0])
					break;
				if (LoadCampaign(filename, &gCampaign.Setting) != CAMPAIGN_OK)
				{
					printf("Error: cannot load %s\n", lastFile);
					continue;
				}
				fileChanged = 0;
				strcpy(lastFile, filename);
				return;
				
			case SDLK_ESCAPE:
				return;
				
			case SDLK_BACKSPACE:
				if (filename[0])
					filename[strlen(filename) - 1] = 0;
				break;
				
			default:
				if (strlen(filename) == sizeof(filename) - 1)
				{
					break;
				}
				c = KeyGetTyped(&gEventHandlers.keyboard);
				if (c && c != '*' &&
					(strlen(filename) > 1 || c != '-') &&
					c != ':' && c != '<' && c != '>' && c != '?' &&
					c != '|')
				{
					size_t si = strlen(filename);
					filename[si + 1] = 0;
					filename[si] = (char)c;
				}
		}
		SDL_Delay(10);
	}
}

static void Save(int asCode)
{
	char filename[CDOGS_FILENAME_MAX];
	char name[32];
	int c;

	strcpy(filename, lastFile);
	for (;;)
	{
		int i;
		for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
		{
			gGraphicsDevice.buf[i] = LookupPalette(58);
		}
		CDogsTextStringAt(125, 50, "Save as:");
		CDogsTextGoto(125, 50 + CDogsTextHeight());
		CDogsTextChar('\020');
		CDogsTextString(filename);
		CDogsTextChar('\021');
		BlitFlip(&gGraphicsDevice, &gConfig.Graphics);

		c = GetKey(&gEventHandlers);
		switch (c)
		{
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			if (!filename[0])
				break;
			if (asCode)
			{
				SaveCampaignAsC(filename, name, &gCampaign.Setting);
			}
			else
			{
				SaveCampaign(filename, &gCampaign.Setting);
			}
			fileChanged = 0;
			strcpy(lastFile, filename);
			return;

		case SDLK_ESCAPE:
			return;

		case SDLK_BACKSPACE:
			if (filename[0])
				filename[strlen(filename) - 1] = 0;
			break;

		default:
			if (strlen(filename) == sizeof(filename) - 1)
			{
				break;
			}
			c = KeyGetTyped(&gEventHandlers.keyboard);
			if (c && c != '*' &&
				(strlen(filename) > 1 || c != '-') &&
				c != ':' && c != '<' && c != '>' && c != '?' &&
				c != '|')
			{
				size_t si = strlen(filename);
				filename[si + 1] = 0;
				filename[si] = (char)c;
			}
		}
		SDL_Delay(10);
	}
}

static int ConfirmQuit(void)
{
	int c;
	int i;
	int w = gGraphicsDevice.cachedConfig.ResolutionWidth;
	int h = gGraphicsDevice.cachedConfig.ResolutionHeight;
	const char *s1 = "Campaign has been modified, but not saved";
	const char *s2 = "Quit anyway? (Y/N)";
	for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
	{
		gGraphicsDevice.buf[i] = LookupPalette(58);
	}
	DrawTextString(
		s1,
		&gGraphicsDevice,
		Vec2iNew((w - TextGetStringWidth(s1)) / 2, (h - CDogsTextHeight()) / 2));
	DrawTextString(
		s2,
		&gGraphicsDevice,
		Vec2iNew((w - TextGetStringWidth(s2)) / 2, (h + CDogsTextHeight()) / 2));
	BlitFlip(&gGraphicsDevice, &gConfig.Graphics);

	c = GetKey(&gEventHandlers);
	return (c == 'Y' || c == 'y');
}

static void HelpScreen(void)
{
	Vec2i pos = Vec2iNew(20, 20);
	const char *helpText =
		"Help\n"
		"====\n"
		"Use mouse to select controls; keyboard to type text\n"
		"Open files by dragging them over the editor shortcut\n"
		"\n"
		"Common commands\n"
		"===============\n"
		"left/right click, page up/down: Increase/decrease value\n"
		"shift + left/right click:       Increase/decrease number of items\n"
		"insert:                         Add new item\n"
		"delete:                         Delete selected item\n"
		"\n"
		"Other commands\n"
		"==============\n"
		"Escape:                         Back or quit\n"
		"Ctrl+E:                         Go to character editor\n"
		"Ctrl+N:                         New mission or character\n"
		"Ctrl+O:                         Open file\n"
		"Ctrl+S:                         Save file\n"
		"Ctrl+X, C, V:                   Cut/copy/paste\n"
		"Ctrl+M:                         Preview automap\n"
		"F1:                             This screen\n";
	int i;
	for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
	{
		gGraphicsDevice.buf[i] = LookupPalette(58);
	}
	DrawTextString(helpText, &gGraphicsDevice, pos);
	BlitFlip(&gGraphicsDevice, &gConfig.Graphics);
	GetKey(&gEventHandlers);
}

static void Delete(int xc, int yc, int *mission)
{
	switch (yc)
	{
	case YC_CHARACTERS:
		DeleteCharacter(xc);
		break;

	case YC_SPECIALS:
		DeleteSpecial(xc);
		break;

	case YC_ITEMS:
		DeleteItem(xc);
		break;

	default:
		if (yc >= YC_OBJECTIVES)
		{
			DeleteObjective(yc - YC_OBJECTIVES);
		}
		else
		{
			DeleteMission(mission);
		}
		AdjustYC(&yc);
		break;
	}
	fileChanged = 1;
	Setup(*mission, 0);
}

static void HandleInput(
	int c, int m,
	int *xc, int *yc, int *xcOld, int *ycOld,
	int *mission, struct Mission *scrap,
	int *willDisplayAutomap, int *done)
{
	if (m)
	{
		UIObject *o;
		if (TryGetClickObj(gEventHandlers.mouse.currentPos, &o))
		{
			if (sObjs1) sObjs1->Highlighted = o;
			if (sObjs2) sObjs2->Highlighted = o;
			*xcOld = *xc;
			*ycOld = *yc;
			// Only change selection on left/right click
			if (m == SDL_BUTTON_LEFT || m == SDL_BUTTON_RIGHT)
			{
				if (!(o->Flags & UI_LEAVE_YC))
				{
					*yc = o->Id;
					AdjustYC(yc);
				}
				if (!(o->Flags & UI_LEAVE_XC))
				{
					*xc = o->Id2;
					AdjustXC(*yc, xc);
				}
			}
			if (!(o->Flags & UI_SELECT_ONLY) &&
				(!(o->Flags & UI_SELECT_ONLY_FIRST) || (*xc == *xcOld && *yc == *ycOld)))
			{
				if (m == SDL_BUTTON_LEFT || m == SDL_BUTTON_WHEELUP)
				{
					c = SDLK_PAGEUP;
				}
				else if (m == SDL_BUTTON_RIGHT || m == SDL_BUTTON_WHEELDOWN)
				{
					c = SDLK_PAGEDOWN;
				}
			}
		}
		else
		{
			if (sObjs1) sObjs1->Highlighted = NULL;
			if (sObjs2) sObjs2->Highlighted = NULL;
		}
	}
	if (gEventHandlers.keyboard.modState & (KMOD_ALT | KMOD_CTRL))
	{
		switch (c)
		{
		case 'x':
			*scrap = gCampaign.Setting.missions[*mission];
			Delete(*xc, *yc, mission);
			break;

		case 'c':
			*scrap = gCampaign.Setting.missions[*mission];
			break;

		case 'v':
			InsertMission(*mission, scrap);
			fileChanged = 1;
			Setup(*mission, 0);
			break;

		case 'q':
			if (!fileChanged || ConfirmQuit())
			{
				*done = 1;
			}
			break;

		case 'n':
			InsertMission(gCampaign.Setting.missionCount, NULL);
			*mission = gCampaign.Setting.missionCount - 1;
			fileChanged = 1;
			Setup(*mission, 0);
			break;
				
		case 'o':
			Open();
			break;

		case 's':
			Save(0);
			break;

		case 'h':
			Save(1);
			break;

		case 'm':
			*willDisplayAutomap = 1;
			break;

		case 'e':
			EditCharacters(&gCampaign.Setting);
			Setup(*mission, 0);
			sObjs1 = &sMainObjs;
			sObjs2 = NULL;
			break;
		}
	}
	else
	{
		switch (c)
		{
		case SDLK_F1:
			HelpScreen();
			break;

		case SDLK_HOME:
			if (*mission > 0)
			{
				(*mission)--;
			}
			Setup(*mission, 0);
			break;

		case SDLK_END:
			if (*mission < gCampaign.Setting.missionCount)
			{
				(*mission)++;
			}
			Setup(*mission, 0);
			break;

		case SDLK_INSERT:
			switch (*yc)
			{
			case YC_CHARACTERS:
				if (gCampaign.Setting.characters.otherCount > 0)
				{
					currentMission->baddieCount =
						CLAMP(currentMission->baddieCount + 1, 0, BADDIE_MAX);
					CharacterStoreAddBaddie(&gCampaign.Setting.characters, 0);
					*xc = currentMission->baddieCount - 1;
				}
				break;

			case YC_SPECIALS:
				if (gCampaign.Setting.characters.otherCount > 0)
				{
					currentMission->specialCount =
						CLAMP(currentMission->specialCount + 1, 0, SPECIAL_MAX);
					CharacterStoreAddSpecial(&gCampaign.Setting.characters, 0);
					*xc = currentMission->specialCount - 1;
				}
				break;

			case YC_ITEMS:
				currentMission->itemCount =
					CLAMP(currentMission->itemCount + 1, 0, ITEMS_MAX);
				*xc = currentMission->itemCount - 1;
				break;

			default:
				if (*yc >= YC_OBJECTIVES)
				{
					AddObjective();
				}
				else
				{
					InsertMission(*mission, NULL);
				}
				break;
			}
			fileChanged = 1;
			Setup(*mission, 0);
			break;

		case SDLK_DELETE:
			Delete(*xc, *yc, mission);
			break;

		case SDLK_UP:
			MoveSelection(0, yc, xc);
			break;

		case SDLK_DOWN:
			MoveSelection(1, yc, xc);
			break;

		case SDLK_TAB:
			MoveSelection(
				!(gEventHandlers.keyboard.modState & KMOD_SHIFT), yc, xc);
			break;

		case SDLK_LEFT:
			(*xc)--;
			AdjustXC(*yc, xc);
			break;

		case SDLK_RIGHT:
			(*xc)++;
			AdjustXC(*yc, xc);
			break;

		case SDLK_PAGEUP:
			if (Change(*yc, *xc, 1, mission))
			{
				fileChanged = 1;
			}
			Setup(*mission, 0);
			break;

		case SDLK_PAGEDOWN:
			if (Change(*yc, *xc, -1, mission))
			{
				fileChanged = 1;
			}
			Setup(*mission, 0);
			break;

		case SDLK_ESCAPE:
			if (!fileChanged || ConfirmQuit())
			{
				*done = 1;
			}
			break;

		case SDLK_BACKSPACE:
			DelChar(*xc, *yc);
			fileChanged = 1;
			break;

		default:
			c = KeyGetTyped(&gEventHandlers.keyboard);
			if (c)
			{
				fileChanged = 1;
				AddChar(*xc, *yc, (char)c);
			}
			break;
		}
	}
}

static void EditCampaign(void)
{
	int done = 0;
	int mission = 0;
	int xc = 0, yc = 0;
	int xcOld, ycOld;
	struct Mission scrap;
	struct EditorInfo edInfo;

	GetEditorInfo(&edInfo);

	memset(&scrap, 0, sizeof(scrap));
	sObjs1 = &sMainObjs;
	sObjs2 = NULL;

	gCampaign.seed = 0;
	Setup(mission, 1);

	SDL_EnableKeyRepeat(0, 0);
	while (!done)
	{
		int willDisplayAutomap = 0;
		int c, m;
		EventPoll(&gEventHandlers, SDL_GetTicks());
		c = KeyGetPressed(&gEventHandlers.keyboard);
		m = MouseGetPressed(&gEventHandlers.mouse);

		HandleInput(
			c, m,
			&xc, &yc, &xcOld, &ycOld,
			&mission, &scrap,
			&willDisplayAutomap, &done);
		Display(mission, xc, yc, willDisplayAutomap);
		if (willDisplayAutomap)
		{
			GetKey(&gEventHandlers);
		}
		SDL_Delay(10);
	}
}


int main(int argc, char *argv[])
{
	int i;
	int loaded = 0;

	printf("C-Dogs SDL Editor\n");

	debug(D_NORMAL, "Initialising SDL...\n");
	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
	{
		printf("Failed to start SDL!\n");
		return -1;
	}
	SDL_EnableUNICODE(SDL_ENABLE);

	printf("Data directory:\t\t%s\n",	GetDataFilePath(""));
	printf("Config directory:\t%s\n\n",	GetConfigFilePath(""));

	if (!PicManagerTryInit(
		&gPicManager, "graphics/cdogs.px", "graphics/cdogs2.px"))
	{
		exit(0);
	}
	memcpy(origPalette, gPicManager.palette, sizeof origPalette);
	BuildTranslationTables(gPicManager.palette);
	CDogsTextInit(GetDataFilePath("graphics/font.px"), -2);

	// initialise UI collections
	// Note: must do this after text init since positions depend on text height
	sMainObjs = CreateMainObjs(&currentMission);
	sCampaignObjs = CreateCampaignObjs();
	sMissionObjs = CreateMissionObjs();
	sWeaponObjs = CreateWeaponObjs();
	sMapItemObjs = CreateMapItemObjs();
	sObjectiveObjs = CreateObjectiveObjs();
	sCharacterObjs = CreateCharacterObjs();

	CampaignInit(&gCampaign);

	ConfigLoadDefault(&gConfig);
	ConfigLoad(&gConfig, GetConfigFilePath(CONFIG_FILE));
	gConfig.Graphics.IsEditor = 1;
	BulletInitialize();
	WeaponInitialize();
	PlayerDataInitialize();
	GraphicsInit(&gGraphicsDevice);
	GraphicsInitialize(
		&gGraphicsDevice, &gConfig.Graphics, gPicManager.palette, 0);
	if (!gGraphicsDevice.IsInitialized)
	{
		printf("Video didn't init!\n");
		exit(EXIT_FAILURE);
	}

	// Reset campaign (graphics init may have created dummy campaigns)
	CharacterStoreTerminate(&gCampaign.Setting.characters);
	memset(&gCampaign.Setting, 0, sizeof gCampaign.Setting);
	CharacterStoreInit(&gCampaign.Setting.characters);
	gCampaign.Setting.missions = NULL;

	EventInit(&gEventHandlers, PicManagerGetOldPic(&gPicManager, 145));

	for (i = 1; i < argc; i++)
	{
		if (!loaded)
		{
			memset(lastFile, 0, sizeof(lastFile));
			strncpy(lastFile, argv[i], sizeof(lastFile) - 1);
			if (strchr(lastFile, '.') == NULL &&
				sizeof lastFile - strlen(lastFile) > 3)
			{
				strcat(lastFile, ".CPN");
			}
			if (LoadCampaign(lastFile, &gCampaign.Setting) == CAMPAIGN_OK)
			{
				loaded = 1;
			}
		}
	}

	currentMission = NULL;

	EditCampaign();

	CampaignTerminate(&gCampaign);

	GraphicsTerminate(&gGraphicsDevice);
	PicManagerTerminate(&gPicManager);

	UICollectionTerminate(&sMainObjs);
	UICollectionTerminate(&sCampaignObjs);
	UICollectionTerminate(&sMissionObjs);
	UICollectionTerminate(&sWeaponObjs);
	UICollectionTerminate(&sMapItemObjs);
	UICollectionTerminate(&sObjectiveObjs);
	UICollectionTerminate(&sCharacterObjs);

	exit(EXIT_SUCCESS);
}
