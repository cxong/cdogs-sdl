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

    Copyright (c) 2013-2015, Cong Xu
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <SDL.h>

#include <cdogs/actors.h>
#include <cdogs/ammo.h>
#include <cdogs/automap.h>
#include <cdogs/collision.h>
#include <cdogs/config_io.h>
#include <cdogs/draw.h>
#include <cdogs/drawtools.h>
#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/grafx.h>
#include <cdogs/keyboard.h>
#include <cdogs/map_archive.h>
#include <cdogs/mission.h>
#include <cdogs/mission_convert.h>
#include <cdogs/objs.h>
#include <cdogs/palette.h>
#include <cdogs/particle.h>
#include <cdogs/pic_manager.h>
#include <cdogs/pickup.h>
#include <cdogs/player_template.h>
#include <cdogs/triggers.h>
#include <cdogs/utils.h>

#include <tinydir/tinydir.h>

#include <cdogsed/charsed.h>
#include <cdogsed/editor_ui.h>
#include <cdogsed/editor_ui_common.h>
#include <cdogsed/ui_object.h>


// Mouse click areas:
static UIObject *sObjs;
static CArray sDrawObjs;	// of UIObjectDrawContext, used to cache BFS order
static UIObject *sLastHighlightedObj = NULL;
static UIObject *sTooltipObj = NULL;
static DrawBuffer sDrawBuffer;
static bool sJustLoaded = true;
static bool sHasUnbakedChanges = false;
static int sAutosaveIndex = 0;
// State for whether to ignore the current mouse click
// This is to prevent painting immediately after selecting a new tool,
// but before the user has clicked again.
static bool sIgnoreMouse = false;


// Globals

static char lastFile[CDOGS_PATH_MAX];
static EditorBrush brush;
static Tile sCursorTile;
Vec2i camera = { 0, 0 };
#define CAMERA_PAN_SPEED 8
Mission currentMission;
Mission lastMission;
#define AUTOSAVE_INTERVAL_SECONDS 60
Uint32 ticksAutosave;
Uint32 sTicksElapsed;


static Vec2i GetMouseTile(EventHandlers *e)
{
	Mission *m = CampaignGetCurrentMission(&gCampaign);
	if (!m)
	{
		return Vec2iNew(-1, -1);
	}
	else
	{
		return Vec2iNew(
			(e->mouse.currentPos.x - sDrawBuffer.dx) / TILE_WIDTH + sDrawBuffer.xStart,
			(e->mouse.currentPos.y - sDrawBuffer.dy) / TILE_HEIGHT + sDrawBuffer.yStart);
	}
}
static Vec2i GetScreenPos(Vec2i mapTile)
{
	return Vec2iNew(
		(mapTile.x - sDrawBuffer.xStart) * TILE_WIDTH + sDrawBuffer.dx,
		(mapTile.y - sDrawBuffer.yStart) * TILE_HEIGHT + sDrawBuffer.dy);
}

static int IsBrushPosValid(Vec2i pos, Mission *m)
{
	return pos.x >= 0 && pos.x < m->Size.x &&
		pos.y >= 0 && pos.y < m->Size.y;
}

static void MakeBackground(GraphicsDevice *g, int buildTables)
{
	if (buildTables)
	{
		// Automatically pan camera to middle of screen
		Mission *m = gMission.missionData;
		Vec2i focusTile = Vec2iScaleDiv(m->Size, 2);
		// Better yet, if the map has a known start position, focus on that
		if (m->Type == MAPTYPE_STATIC && !Vec2iIsZero(m->u.Static.Start))
		{
			focusTile = m->u.Static.Start;
		}
		camera = Vec2iCenterOfTile(focusTile);
	}

	// Clear background first
	for (int i = 0; i < GraphicsGetScreenSize(&g->cachedConfig); i++)
	{
		g->buf[i] = COLOR2PIXEL(colorBlack);
	}
	GrafxDrawExtra extra;
	extra.guideImage = brush.GuideImageSurface;
	extra.guideImageAlpha = brush.GuideImageAlpha;

	DrawBufferTerminate(&sDrawBuffer);
	DrawBufferInit(&sDrawBuffer, Vec2iNew(X_TILES, Y_TILES), &gGraphicsDevice);
	GrafxMakeBackground(
		g, &sDrawBuffer, &gCampaign, &gMission, &gMap,
		tintNone, 1, buildTables, camera, &extra);
}

// Returns whether a redraw is required
typedef struct
{
	bool Redraw;
	bool RemakeBg;
	bool WillDisplayAutomap;
	bool Done;
} HandleInputResult;
static HandleInputResult HandleInput(
	int c, int m, int *xc, int *yc, int *xcOld, int *ycOld, Mission *scrap);

static void Display(GraphicsDevice *g, int yc, HandleInputResult result)
{
	char s[128];
	int y = 5;
	int i;
	int w = g->cachedConfig.Res.x;
	int h = g->cachedConfig.Res.y;
	Mission *mission = CampaignGetCurrentMission(&gCampaign);

	if (mission)
	{
		// Re-make the background if the resolution has changed
		if (gEventHandlers.HasResolutionChanged)
		{
			MakeBackground(g, 0);
		}
		if (result.RemakeBg || brush.IsGuideImageNew)
		{
			// Clear background first
			for (i = 0; i < GraphicsGetScreenSize(&g->cachedConfig); i++)
			{
				g->buf[i] = COLOR2PIXEL(colorBlack);
			}
			brush.IsGuideImageNew = false;
			GrafxDrawExtra extra;
			extra.guideImage = brush.GuideImageSurface;
			extra.guideImageAlpha = brush.GuideImageAlpha;
			GrafxDrawBackground(g, &sDrawBuffer, tintNone, camera, &extra);
		}
		GraphicsBlitBkg(g);

		// Draw brush highlight tiles
		if (brush.IsActive && IsBrushPosValid(brush.Pos, mission))
		{
			EditorBrushSetHighlightedTiles(&brush);
		}
		for (i = 0; i < (int)brush.HighlightedTiles.size; i++)
		{
			Vec2i *pos = CArrayGet(&brush.HighlightedTiles, i);
			Vec2i screenPos = GetScreenPos(*pos);
			if (screenPos.x >= 0 && screenPos.x < w &&
				screenPos.y >= 0 && screenPos.y < h)
			{
				DrawRectangle(
					g,
					screenPos, Vec2iNew(TILE_WIDTH, TILE_HEIGHT),
					colorWhite, DRAW_FLAG_LINE);
			}
		}

		sprintf(
			s, "Mission %d/%d",
			gCampaign.MissionIndex + 1, (int)gCampaign.Setting.Missions.size);
		FontStrMask(
			s, Vec2iNew(270, y),
			yc == YC_MISSIONINDEX ? colorRed : colorWhite);
		if (brush.LastPos.x)
		{
			sprintf(s, "(%d, %d)", brush.Pos.x, brush.Pos.y);
			FontStr(s, Vec2iNew(w - 40, h - 16));
		}
	}
	else
	{
		ClearScreen(g);
		if (gCampaign.Setting.Missions.size)
		{
			sprintf(s, "End/%d", (int)gCampaign.Setting.Missions.size);
			FontStrMask(
				s, Vec2iNew(270, y),
				yc == YC_MISSIONINDEX ? colorRed : colorWhite);
		}
	}

	if (fileChanged)
	{
		DrawTPic(10, y, PicManagerGetOldPic(&gPicManager, 221));
	}

	FontStr("Press Ctrl+E to edit characters", Vec2iNew(20, h - 20 - FontH() * 2));
	FontStr("Press F1 for help", Vec2iNew(20, h - 20 - FontH()));

	y = 150;

	UIObjectDraw(
		sObjs, g, Vec2iZero(), gEventHandlers.mouse.currentPos, &sDrawObjs);

	if (result.WillDisplayAutomap && mission)
	{
		AutomapDraw(AUTOMAP_FLAGS_SHOWALL, true);
	}
	else
	{
		if (sTooltipObj && sTooltipObj->Tooltip)
		{
			UITooltipDraw(
				g, gEventHandlers.mouse.currentPos, sTooltipObj->Tooltip);
		}
		MouseDraw(&gEventHandlers.mouse);
	}
	BlitFlip(g);
}

static int Change(UIObject *o, int yc, int d)
{
	int isChanged = 0;

	if (yc == YC_MISSIONINDEX)
	{
		gCampaign.MissionIndex = CLAMP(
			gCampaign.MissionIndex + d,
			0,
			(int)gCampaign.Setting.Missions.size);
		return 0;
	}

	if (o)
	{
		isChanged = UIObjectChange(o, d);
	}
	return isChanged;
}

static void AddObjective(Mission *m)
{
	// TODO: support more objectives
	if (m->Objectives.size < OBJECTIVE_MAX_OLD)
	{
		MissionObjective mo;
		memset(&mo, 0, sizeof mo);
		CArrayPushBack(&m->Objectives, &mo);
	}
}

static void DeleteObjective(Mission *m, int idx)
{
	idx = CLAMP(idx, 0, (int)m->Objectives.size);
	CArrayDelete(&m->Objectives, idx);
}

static void DeleteCharacter(Mission *m, int idx)
{
	idx = CLAMP(idx, 0, (int)m->Enemies.size);
	CArrayDelete(&m->Enemies, idx);
	CharacterStoreDeleteBaddie(&gCampaign.Setting.characters, idx);
}

static void DeleteSpecial(Mission *m, int idx)
{
	idx = CLAMP(idx, 0, (int)m->SpecialChars.size);
	CArrayDelete(&m->SpecialChars, idx);
	CharacterStoreDeleteSpecial(&gCampaign.Setting.characters, idx);
}

static void DeleteItem(Mission *m, int idx)
{
	if (idx >= (int)m->MapObjectDensities.size)
	{
		// Nothing to delete; do nothing
		return;
	}
	idx = CLAMP(idx, 0, (int)m->MapObjectDensities.size);
	CArrayDelete(&m->MapObjectDensities, idx);
}

static void AdjustYC(int *yc)
{
	Mission *mission = CampaignGetCurrentMission(&gCampaign);
	if (mission)
	{
		if (mission->Objectives.size)
		{
			*yc = CLAMP_OPPOSITE(
				*yc, 0, YC_OBJECTIVES + (int)mission->Objectives.size - 1);
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
	Mission *mission = CampaignGetCurrentMission(&gCampaign);
	switch (yc)
	{
	case YC_CAMPAIGNTITLE:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_CAMPAIGNDESC);
		break;

	case YC_MISSIONTITLE:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_MUSICFILE);
		break;

	case YC_MISSIONLOOKS:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_COLOR4);
		break;

	case YC_CHARACTERS:
		if (mission && mission->Enemies.size > 0)
		{
			*xc = CLAMP_OPPOSITE(*xc, 0, (int)mission->Enemies.size - 1);
		}
		break;

	case YC_SPECIALS:
		if (mission && mission->SpecialChars.size > 0)
		{
			*xc = CLAMP_OPPOSITE(*xc, 0, (int)mission->SpecialChars.size - 1);
		}
		break;

	case YC_ITEMS:
		if (mission && mission->MapObjectDensities.size > 0)
		{
			*xc = CLAMP_OPPOSITE(
				*xc, 0, (int)mission->MapObjectDensities.size - 1);
		}
		break;

	default:
		if (yc >= YC_OBJECTIVES)
		{
			*xc = CLAMP_OPPOSITE(*xc, 0, XC_FLAGS);
		}
		break;
	}
}

static void Autosave(void)
{
	if (fileChanged && sTicksElapsed > ticksAutosave)
	{
		ticksAutosave = sTicksElapsed + AUTOSAVE_INTERVAL_SECONDS * 1000;
		char dirname[CDOGS_PATH_MAX];
		PathGetDirname(dirname, lastFile);
		char buf[CDOGS_PATH_MAX];
		sprintf(
			buf, "%s~%d%s", dirname, sAutosaveIndex, PathGetBasename(lastFile));
		fprintf(stderr, "Autosaving...");
		MapArchiveSave(buf, &gCampaign.Setting);
		fprintf(stderr, "done\n");
		sAutosaveIndex++;
	}
}

static void Setup(int buildTables)
{
	Mission *m = CampaignGetCurrentMission(&gCampaign);
	if (!m)
	{
		return;
	}
	MissionCopy(&lastMission, &currentMission);
	MissionCopy(&currentMission, m);
	MissionOptionsTerminate(&gMission);
	CampaignAndMissionSetup(buildTables, &gCampaign, &gMission);
	MakeBackground(&gGraphicsDevice, buildTables);
	sCursorTile = TileNone();

	Autosave();

	sJustLoaded = true;
	sHasUnbakedChanges = false;
}

// Reload UI so that we can load new elements based on custom data etc.
static void ReloadUI(void)
{
	UIObjectDestroy(sObjs);
	sObjs = CreateMainObjs(&gCampaign, &brush, Vec2iNew(320, 240));
	CArrayTerminate(&sDrawObjs);
	sLastHighlightedObj = NULL;
	sTooltipObj = NULL;
}

static bool TryOpen(const char *filename);
static void Open(void)
{
	char filename[CDOGS_PATH_MAX];
	strcpy(filename, lastFile);
	bool done = false;
	while (!done)
	{
		ClearScreen(&gGraphicsDevice);
		const int x = 125;
		Vec2i pos = Vec2iNew(x, 50);
		FontStr("Open file:", pos);
		pos.y += FontH();
		pos = FontCh('>', pos);
		pos = FontStr(filename, pos);
		pos = FontCh('<', pos);

		// Based on the filename, print a list of candidates
		tinydir_dir dir;
		char buf[CDOGS_PATH_MAX];
		PathGetDirname(buf, filename);
		if (!tinydir_open_sorted(&dir, buf))
		{
			const char *basename = PathGetBasename(filename);
			pos.x = x;
			pos.y += FontH() * 2;
			for (int i = 0; i < (int)dir.n_files; i++)
			{
				tinydir_file file;
				tinydir_readfile_n(&dir, &file, i);
				if (strncmp(file.name, basename, strlen(basename)) == 0)
				{
					// Ignore files that aren't campaigns
					if (file.name[0] == '.') continue;
					if (!file.is_dir &&
						strcmp(file.extension, "cdogscpn") != 0 &&
						strcmp(file.extension, "CDOGSCPN") != 0 &&
						strcmp(file.extension, "cpn") != 0 &&
						strcmp(file.extension, "CPN") != 0)
						continue;
					pos = FontStrMask(file.path, pos, colorGray);
					pos.x = x;
					pos.y += FontH();
				}
			}
			tinydir_close(&dir);
		}

		BlitFlip(&gGraphicsDevice);

		bool doOpen = false;
		int c = GetKey(&gEventHandlers);
		switch (c)
		{
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				if (!filename[0])
				{
					break;
				}
				doOpen = true;
				break;
				
			case SDLK_ESCAPE:
				done = true;
				break;
				
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
				break;
		}
		if (doOpen)
		{
			ClearScreen(&gGraphicsDevice);

			FontStrCenter("Loading...");

			BlitFlip(&gGraphicsDevice);
			// Try original filename
			if (TryOpen(filename))
			{
				goto loaded;
			}
			// Try adding .cdogscpn
			sprintf(buf, "%s.cdogscpn", filename);
			if (TryOpen(buf))
			{
				goto loaded;
			}
			// Try adding .cpn
			sprintf(buf, "%s.cpn", filename);
			if (TryOpen(filename))
			{
				goto loaded;
			}
			// All attempts failed
			printf("Error: cannot load %s\n", buf);
			continue;

		loaded:
			fileChanged = 0;
			Setup(1);
			strcpy(lastFile, buf);
			done = true;
			sAutosaveIndex = 0;
			ReloadUI();
		}
		SDL_Delay(10);
	}
}
static bool TryOpen(const char *filename)
{
	// Try opening a campaign
	CampaignSettingTerminate(&gCampaign.Setting);
	CampaignSettingInit(&gCampaign.Setting);
	char buf[CDOGS_PATH_MAX];
	RealPath(filename, buf);
	return !MapNewLoad(buf, &gCampaign.Setting);
}

static void Save(void)
{
	char filename[CDOGS_PATH_MAX];
	strcpy(filename, lastFile);
	bool doSave = false;
	bool done = false;
	while (!done)
	{
		ClearScreen(&gGraphicsDevice);
		Vec2i pos = Vec2iNew(125, 50);
		FontStr("Save as:", pos);
		pos.y += FontH();
		pos = FontCh('>', pos);
		pos = FontStr(filename, pos);
		pos = FontCh('<', pos);
		BlitFlip(&gGraphicsDevice);

		int c = GetKey(&gEventHandlers);
		switch (c)
		{
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			if (!filename[0])
			{
				break;
			}
			doSave = true;
			done = true;
			break;

		case SDLK_ESCAPE:
			break;

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
	if (doSave)
	{
		ClearScreen(&gGraphicsDevice);

		FontStrCenter("Saving...");
		
		BlitFlip(&gGraphicsDevice);
		MapArchiveSave(filename, &gCampaign.Setting);
		fileChanged = 0;
		strcpy(lastFile, filename);
		sAutosaveIndex = 0;
		printf("Saved to %s\n", filename);
	}
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
		"arrow keys:                     Move camera\n"
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
	ClearScreen(&gGraphicsDevice);
	FontStr(helpText, pos);
	BlitFlip(&gGraphicsDevice);
	GetKey(&gEventHandlers);
}

static void Delete(int xc, int yc)
{
	Mission *mission = CampaignGetCurrentMission(&gCampaign);
	switch (yc)
	{
	case YC_CHARACTERS:
		if (xc >= (int)mission->Enemies.size)
		{
			// Nothing to delete; do nothing
			return;
		}
		DeleteCharacter(mission, xc);
		break;

	case YC_SPECIALS:
		if (xc >= (int)mission->SpecialChars.size)
		{
			// Nothing to delete; do nothing
			return;
		}
		DeleteSpecial(mission, xc);
		break;

	case YC_ITEMS:
		DeleteItem(mission, xc);
		break;

	default:
		if (yc >= YC_OBJECTIVES)
		{
			if (mission->Objectives.size == 0)
			{
				// Nothing to delete; do nothing
				return;
			}
			DeleteObjective(mission, yc - YC_OBJECTIVES);
		}
		else
		{
			if (gCampaign.Setting.Missions.size == 0 ||
				gCampaign.MissionIndex >= (int)gCampaign.Setting.Missions.size)
			{
				// Nothing to delete; do nothing
				return;
			}
			DeleteMission(&gCampaign);
		}
		AdjustYC(&yc);
		break;
	}
	fileChanged = 1;
	Setup(0);
}

static void InputInsert(int *xc, const int yc, Mission *mission);
static void InputDelete(const int xc, const int yc);
static HandleInputResult HandleInput(
	int c, int m, int *xc, int *yc, int *xcOld, int *ycOld, Mission *scrap)
{
	HandleInputResult result = { false, false, false, false };
	Mission *mission = CampaignGetCurrentMission(&gCampaign);
	UIObject *o = NULL;
	brush.Pos = GetMouseTile(&gEventHandlers);

	// Find whether the mouse has hovered over a tooltip
	const bool hadTooltip = sTooltipObj != NULL;
	bool isContextMenu = false;
	sTooltipObj = NULL;
	if (UITryGetObject(sObjs, gEventHandlers.mouse.currentPos, &sTooltipObj)
		&& sTooltipObj)
	{
		if (sTooltipObj->Parent &&
			sTooltipObj->Parent->Type == UITYPE_CONTEXT_MENU)
		{
			isContextMenu = true;
		}
		if (!sTooltipObj->Tooltip)
		{
			sTooltipObj = NULL;
		}
	}
	// Need to redraw if we either had a tooltip (draw to remove) or there's a
	// tooltip to draw
	if (hadTooltip || sTooltipObj || isContextMenu)
	{
		result.Redraw = true;
	}

	// Make sure a redraw is done immediately if the resolution changes
	// Otherwise the resolution change is ignored and we try to redraw
	// later, when the draw buffer has not yet been recreated
	if (gEventHandlers.HasResolutionChanged)
	{
		result.Redraw = true;
	}

	// Also need to redraw if the brush is active to update the highlight
	if (brush.IsActive)
	{
		result.Redraw = true;
	}

	if (m &&
		(m == SDL_BUTTON_LEFT || m == SDL_BUTTON_RIGHT ||
		m == SDL_BUTTON_WHEELUP || m == SDL_BUTTON_WHEELDOWN))
	{
		result.Redraw = true;
		if (UITryGetObject(sObjs, gEventHandlers.mouse.currentPos, &o))
		{
			if (!o->DoNotHighlight)
			{
				if (sLastHighlightedObj)
				{
					UIObjectUnhighlight(sLastHighlightedObj);
				}
				sLastHighlightedObj = o;
				UIObjectHighlight(o);
				sIgnoreMouse = true;
			}
			CArrayTerminate(&sDrawObjs);
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
			if (!(brush.IsActive && mission))
			{
				UIObjectUnhighlight(sObjs);
				CArrayTerminate(&sDrawObjs);
				sLastHighlightedObj = NULL;
			}
		}
	}
	if (!o &&
		(MouseIsDown(&gEventHandlers.mouse, SDL_BUTTON_LEFT) ||
		MouseIsDown(&gEventHandlers.mouse, SDL_BUTTON_RIGHT)) &&
		!sIgnoreMouse)
	{
		result.Redraw = true;
		if (brush.IsActive && mission->Type == MAPTYPE_STATIC)
		{
			// Draw a tile
			if (IsBrushPosValid(brush.Pos, mission))
			{
				int isMain =
					MouseIsDown(&gEventHandlers.mouse, SDL_BUTTON_LEFT);
				EditorResult r =
					EditorBrushStartPainting(&brush, mission, isMain);
				if (r == EDITOR_RESULT_CHANGED ||
					r == EDITOR_RESULT_CHANGED_AND_RELOAD)
				{
					fileChanged = 1;
					Autosave();
					result.RemakeBg = true;
					sHasUnbakedChanges = true;
				}
				if (r == EDITOR_RESULT_CHANGED_AND_RELOAD)
				{
					Setup(0);
				}
			}
		}
	}
	else
	{
		if (mission)
		{
			// Clamp brush position
			brush.Pos = Vec2iClamp(
				brush.Pos,
				Vec2iZero(), Vec2iMinus(mission->Size, Vec2iUnit()));
			EditorResult r = EditorBrushStopPainting(&brush, mission);
			if (r == EDITOR_RESULT_CHANGED ||
				r == EDITOR_RESULT_CHANGED_AND_RELOAD)
			{
				fileChanged = 1;
				Autosave();
				result.Redraw = true;
				result.RemakeBg = true;
				sHasUnbakedChanges = true;
			}
			if (r == EDITOR_RESULT_CHANGED_AND_RELOAD)
			{
				Setup(0);
			}
		}
	}
	// Pan the camera based on keyboard cursor keys
	if (mission)
	{
		if (KeyIsDown(&gEventHandlers.keyboard, SDLK_LEFT))
		{
			camera.x -= CAMERA_PAN_SPEED;
			result.Redraw = result.RemakeBg = true;
		}
		else if (KeyIsDown(&gEventHandlers.keyboard, SDLK_RIGHT))
		{
			camera.x += CAMERA_PAN_SPEED;
			result.Redraw = result.RemakeBg = true;
		}
		if (KeyIsDown(&gEventHandlers.keyboard, SDLK_UP))
		{
			camera.y -= CAMERA_PAN_SPEED;
			result.Redraw = result.RemakeBg = true;
		}
		else if (KeyIsDown(&gEventHandlers.keyboard, SDLK_DOWN))
		{
			camera.y += CAMERA_PAN_SPEED;
			result.Redraw = result.RemakeBg = true;
		}
		// Also pan the camera based on middle mouse drag
		if (MouseIsDown(&gEventHandlers.mouse, SDL_BUTTON_MIDDLE))
		{
			camera = Vec2iAdd(camera, Vec2iMinus(
				gEventHandlers.mouse.previousPos,
				gEventHandlers.mouse.currentPos));
			result.Redraw = result.RemakeBg = true;
		}
		camera.x = CLAMP(camera.x, 0, Vec2iCenterOfTile(mission->Size).x);
		camera.y = CLAMP(camera.y, 0, Vec2iCenterOfTile(mission->Size).y);
	}
	bool hasQuit = false;
	if (c != 0)
	{
		result.Redraw = true;
	}
	if (gEventHandlers.keyboard.modState & (KMOD_ALT | KMOD_CTRL))
	{
		switch (c)
		{
		case 'z':
			// Undo
			// Do this by swapping the current mission with the last mission
			// This requires a bit of copy-acrobatics; because missions
			// are saved in Setup(), but by this stage the mission has already
			// changed, _two_ mission caches are used, copied in sequence.
			// That is, if the current mission is at state B, the first cache
			// is still at state B (copied after the mission has changed
			// already), and the second cache is at state A.
			// If we were to perform an undo and still maintain functionality,
			// we need to copy such that the states change from B,B,A to
			// A,A,B.

			// However! The above is true only if we have "baked" changes
			// The editor has been optimised to perform some changes
			// without reloading map files; that is, the files are actually
			// in states C,B,A.
			// In this case, another set of "acrobatics" is required
			if (sHasUnbakedChanges)
			{
				MissionCopy(&lastMission, mission);	// B,A,Z -> B,A,B
			}
			else
			{
				MissionCopy(mission, &lastMission);	// B,B,A -> A,B,A
				MissionCopy(&lastMission, &currentMission);	// A,B,A -> A,B,B
			}
			fileChanged = 1;
			Setup(0);	// A,B,B -> A,A,B
			break;

		case 'x':
			MissionTerminate(scrap);
			MissionCopy(scrap, mission);
			Delete(*xc, *yc);
			// Unhighlight everything (in case this is the last mission)
			UIObjectUnhighlight(sObjs);
			CArrayTerminate(&sDrawObjs);
			sLastHighlightedObj = NULL;
			break;

		case 'c':
			MissionTerminate(scrap);
			MissionCopy(scrap, mission);
			break;

		case 'v':
			// Use map size as a proxy to whether there's a valid scrap mission
			if (!Vec2iIsZero(scrap->Size))
			{
				InsertMission(&gCampaign, scrap, gCampaign.MissionIndex);
				fileChanged = 1;
				Setup(0);
			}
			break;

		case 'i':
			InputInsert(xc, *yc, mission);
			break;

		case 'd':
			InputDelete(*xc, *yc);
			break;

		case 'q':
			hasQuit = true;
			break;

		case 'n':
			InsertMission(&gCampaign, NULL, gCampaign.Setting.Missions.size);
			gCampaign.MissionIndex = gCampaign.Setting.Missions.size - 1;
			fileChanged = 1;
			Setup(0);
			break;
				
		case 'o':
			if (!fileChanged || ConfirmScreen(
				"File has been modified, but not saved", "Open anyway? (Y/N)"))
			{
				Open();
			}
			break;

		case 's':
			Save();
			break;

		case 'm':
			result.WillDisplayAutomap = true;
			break;

		case 'e':
			EditCharacters(&gCampaign.Setting);
			Setup(0);
			UIObjectUnhighlight(sObjs);
			CArrayTerminate(&sDrawObjs);
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
			if (gCampaign.MissionIndex > 0)
			{
				gCampaign.MissionIndex--;
			}
			Setup(0);
			break;

		case SDLK_END:
			if (gCampaign.MissionIndex < (int)gCampaign.Setting.Missions.size)
			{
				gCampaign.MissionIndex++;
			}
			Setup(0);
			break;

		case SDLK_INSERT:
			InputInsert(xc, *yc, mission);
			break;

		case SDLK_DELETE:
			InputDelete(*xc, *yc);
			break;

		case SDLK_PAGEUP:
			if (Change(o, *yc, 1))
			{
				fileChanged = 1;
			}
			Setup(0);
			break;

		case SDLK_PAGEDOWN:
			if (Change(o, *yc, -1))
			{
				fileChanged = 1;
			}
			Setup(0);
			break;

		case SDLK_ESCAPE:
			hasQuit = true;
			break;

		case SDLK_BACKSPACE:
			fileChanged |= UIObjectDelChar(sObjs);
			break;

		default:
			c = KeyGetTyped(&gEventHandlers.keyboard);
			if (c)
			{
				fileChanged |= UIObjectAddChar(sObjs, (char)c);
			}
			break;
		}
	}
	if (gEventHandlers.HasQuit)
	{
		hasQuit = true;
	}
	if (hasQuit && (!fileChanged || ConfirmScreen(
		"File has been modified, but not saved", "Quit anyway? (Y/N)")))
	{
		result.Done = true;
	}
	if (!MouseIsDown(&gEventHandlers.mouse, SDL_BUTTON_LEFT) &&
		!MouseIsDown(&gEventHandlers.mouse, SDL_BUTTON_RIGHT))
	{
		sIgnoreMouse = false;
	}
	return result;
}
static void InputInsert(int *xc, const int yc, Mission *mission)
{
	switch (yc)
	{
	case YC_CHARACTERS:
		if (gCampaign.Setting.characters.OtherChars.size > 0)
		{
			int ch = 0;
			CArrayPushBack(&mission->Enemies, &ch);
			CharacterStoreAddBaddie(&gCampaign.Setting.characters, ch);
			*xc = mission->Enemies.size - 1;
		}
		break;

	case YC_SPECIALS:
		if (gCampaign.Setting.characters.OtherChars.size > 0)
		{
			int ch = 0;
			CArrayPushBack(&mission->SpecialChars, &ch);
			CharacterStoreAddSpecial(&gCampaign.Setting.characters, ch);
			*xc = mission->SpecialChars.size - 1;
		}
		break;

	case YC_ITEMS:
		{
			MapObjectDensity mod;
			mod.M = IndexMapObject(0);
			mod.Density = 0;
			CArrayPushBack(&mission->MapObjectDensities, &mod);
			*xc = mission->MapObjectDensities.size - 1;
		}
		break;

	default:
		if (yc >= YC_OBJECTIVES)
		{
			AddObjective(mission);
		}
		else
		{
			InsertMission(&gCampaign, NULL, gCampaign.MissionIndex);
		}
		break;
	}
	fileChanged = 1;
	Setup(0);
}
static void InputDelete(const int xc, const int yc)
{
	Delete(xc, yc);
	// Unhighlight everything (in case this is the last mission)
	UIObjectUnhighlight(sObjs);
	CArrayTerminate(&sDrawObjs);
	sLastHighlightedObj = NULL;
}

static void EditCampaign(void)
{
	int xc = 0, yc = 0;
	int xcOld, ycOld;
	Mission scrap;
	memset(&scrap, 0, sizeof scrap);

	gCampaign.seed = 0;
	Setup(1);

	SDL_EnableKeyRepeat(0, 0);
	Uint32 ticksNow = SDL_GetTicks();
	sTicksElapsed = 0;
	ticksAutosave = AUTOSAVE_INTERVAL_SECONDS * 1000;
	for (;;)
	{
		Uint32 ticksThen = ticksNow;
		ticksNow = SDL_GetTicks();
		sTicksElapsed += ticksNow - ticksThen;
		if (sTicksElapsed < 1000 / FPS_FRAMELIMIT * 2)
		{
			SDL_Delay(1);
			debug(D_VERBOSE, "Delaying 1 ticksNow %u elapsed %u\n", ticksNow, sTicksElapsed);
			continue;
		}

		debug(D_MAX, "Polling for input\n");
		EventPoll(&gEventHandlers, SDL_GetTicks());
		int c = KeyGetPressed(&gEventHandlers.keyboard);
		int m = MouseGetPressed(&gEventHandlers.mouse);

		debug(D_MAX, "Handling input\n");
		HandleInputResult result = HandleInput(
			c, m, &xc, &yc, &xcOld, &ycOld, &scrap);
		if (result.Done)
		{
			break;
		}
		if (result.Redraw || result.RemakeBg || sJustLoaded)
		{
			sJustLoaded = false;
			debug(D_MAX, "Drawing UI\n");
			Display(&gGraphicsDevice, yc, result);
			if (result.WillDisplayAutomap)
			{
				GetKey(&gEventHandlers);
			}
		}
		debug(D_MAX, "End loop\n");
		sTicksElapsed -= 1000 / (FPS_FRAMELIMIT * 2);
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

	char buf[CDOGS_PATH_MAX];
	char buf2[CDOGS_PATH_MAX];
	GetDataFilePath(buf, "");
	printf("Data directory:\t\t%s\n", buf);
	printf("Config directory:\t%s\n\n", GetConfigFilePath(""));

	EditorBrushInit(&brush);
	strcpy(lastFile, "");

	gConfig = ConfigLoad(GetConfigFilePath(CONFIG_FILE));
	if (!PicManagerTryInit(
		&gPicManager, "graphics/cdogs.px", "graphics/cdogs2.px"))
	{
		exit(0);
	}
	memcpy(origPalette, gPicManager.palette, sizeof origPalette);
	BuildTranslationTables(gPicManager.palette);
	// Hardcode config settings
	ConfigGet(&gConfig, "Graphics.ScaleMode")->u.Enum.Value = SCALE_MODE_NN;
	ConfigGet(&gConfig, "Graphics.ScaleFactor")->u.Int.Value = 2;
	ConfigGet(&gConfig, "Graphics.ResolutionWidth")->u.Int.Value = 400;
	ConfigGet(&gConfig, "Graphics.ResolutionHeight")->u.Int.Value = 300;
	GraphicsInit(&gGraphicsDevice, &gConfig);
	gGraphicsDevice.cachedConfig.IsEditor = true;
	GraphicsInitialize(&gGraphicsDevice, false);
	if (!gGraphicsDevice.IsInitialized)
	{
		printf("Video didn't init!\n");
		exit(EXIT_FAILURE);
	}
	GetDataFilePath(buf, "graphics/font.png");
	GetDataFilePath(buf2, "graphics/font.json");
	FontLoad(&gFont, buf, buf2);
	GetDataFilePath(buf, "graphics");
	PicManagerLoadDir(&gPicManager, buf);

	GetDataFilePath(buf, "data/particles.json");
	ParticleClassesInit(&gParticleClasses, buf);
	GetDataFilePath(buf, "data/ammo.json");
	AmmoInitialize(&gAmmo, buf);
	GetDataFilePath(buf, "data/bullets.json");
	GetDataFilePath(buf2, "data/guns.json");
	BulletAndWeaponInitialize(&gBulletClasses, &gGunDescriptions, buf, buf2);
	GetDataFilePath(buf, "data/pickups.json");
	PickupClassesInit(&gPickupClasses, buf, &gAmmo, &gGunDescriptions);
	GetDataFilePath(buf, "data/map_objects.json");
	MapObjectsInit(&gMapObjects, buf);
	CollisionSystemInit(&gCollisionSystem);
	CampaignInit(&gCampaign);
	MissionInit(&lastMission);
	MissionInit(&currentMission);

	// initialise UI collections
	// Note: must do this after text init since positions depend on text height
	sObjs = CreateMainObjs(&gCampaign, &brush, Vec2iNew(320, 240));
	memset(&sDrawObjs, 0, sizeof sDrawObjs);
	DrawBufferInit(&sDrawBuffer, Vec2iNew(X_TILES, Y_TILES), &gGraphicsDevice);

	// Reset campaign (graphics init may have created dummy campaigns)
	CampaignSettingTerminate(&gCampaign.Setting);
	CampaignSettingInit(&gCampaign.Setting);

	// Not needed by editor; used as work-around for strange OSX linker error
	LoadPlayerTemplates(&gPlayerTemplates, PLAYER_TEMPLATE_FILE);

	EventInit(&gEventHandlers, NULL, NULL, false);

	for (i = 1; i < argc; i++)
	{
		if (!loaded)
		{
			debug(D_NORMAL, "Loading map %s\n", argv[i]);
			RealPath(argv[i], lastFile);
			if (strchr(lastFile, '.') == NULL &&
				sizeof lastFile - strlen(lastFile) > 3)
			{
				strcat(lastFile, ".cdogscpn");
			}
			if (MapNewLoad(lastFile, &gCampaign.Setting) == 0)
			{
				loaded = 1;
				ReloadUI();
			}
			debug(D_NORMAL, "Loaded map %s\n", argv[i]);
		}
	}

	debug(D_NORMAL, "Starting editor\n");
	EditCampaign();

	CArrayTerminate(&gPlayerTemplates);

	MapTerminate(&gMap);
	MapObjectsTerminate(&gMapObjects);
	PickupClassesTerminate(&gPickupClasses);
	ParticleClassesTerminate(&gParticleClasses);
	AmmoTerminate(&gAmmo);
	WeaponTerminate(&gGunDescriptions);
	BulletTerminate(&gBulletClasses);
	CampaignTerminate(&gCampaign);
	MissionTerminate(&lastMission);
	MissionTerminate(&currentMission);

	DrawBufferTerminate(&sDrawBuffer);
	GraphicsTerminate(&gGraphicsDevice);
	PicManagerTerminate(&gPicManager);

	UIObjectDestroy(sObjs);
	CArrayTerminate(&sDrawObjs);
	EditorBrushTerminate(&brush);

	SDL_Quit();

	exit(EXIT_SUCCESS);
}
