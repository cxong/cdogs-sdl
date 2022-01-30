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

	Copyright (c) 2013-2021 Cong Xu
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include <cdogs/XGetopt.h>
#include <cdogs/actors.h>
#include <cdogs/automap.h>
#include <cdogs/collision/collision.h>
#include <cdogs/config_io.h>
#include <cdogs/draw/draw.h>
#include <cdogs/draw/drawtools.h>
#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/font_utils.h>
#include <cdogs/log.h>
#include <cdogs/map_wolf.h>
#include <cdogs/player_template.h>

#include <tinydir/tinydir.h>

#include <cdogsed/char_editor.h>
#include <cdogsed/editor_ui.h>
#include <cdogsed/editor_ui_common.h>
#include <cdogsed/osdialog/osdialog.h>

// Mouse click areas:
static UIObject *sObjs;
static CArray sDrawObjs; // of UIObjectDrawContext, used to cache BFS order
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

typedef struct
{
	GraphicsDevice *g;
	struct vec2 camera;
} EditorContext;
EditorContext ec;
static char lastFile[CDOGS_PATH_MAX];
static EditorBrush brush;
#define CAMERA_PAN_SPEED 3
Mission currentMission;
Mission lastMission;
#define AUTOSAVE_INTERVAL_SECONDS 60
Uint32 ticksAutosave;
Uint32 sTicksElapsed;
bool fileChanged = false;

#ifdef __APPLE__
#define KMOD_CMD KMOD_GUI
#define KMOD_CMD_NAME "Cmd"
#else
#define KMOD_CMD KMOD_CTRL
#define KMOD_CMD_NAME "Ctrl"
#endif

static struct vec2i GetMouseTile(EventHandlers *e)
{
	Mission *m = CampaignGetCurrentMission(&gCampaign);
	if (!m)
	{
		return svec2i(-1, -1);
	}
	else
	{
		return svec2i(
			(e->mouse.currentPos.x - sDrawBuffer.dx) / TILE_WIDTH +
				sDrawBuffer.xStart,
			(e->mouse.currentPos.y - sDrawBuffer.dy) / TILE_HEIGHT +
				sDrawBuffer.yStart);
	}
}
static struct vec2i GetScreenPos(struct vec2i mapTile)
{
	return svec2i(
		(mapTile.x - sDrawBuffer.xStart) * TILE_WIDTH + sDrawBuffer.dx,
		(mapTile.y - sDrawBuffer.yStart) * TILE_HEIGHT + sDrawBuffer.dy);
}

static void MakeBackground(const bool changedMission)
{
	if (changedMission)
	{
		// Automatically pan camera to middle of screen
		Mission *m = gMission.missionData;
		struct vec2i focusTile = svec2i_scale_divide(m->Size, 2);
		// Better yet, if the map has a known start position, focus on that
		if (m->Type == MAPTYPE_STATIC && !svec2i_is_zero(m->u.Static.Start))
		{
			focusTile = m->u.Static.Start;
		}
		ec.camera = Vec2CenterOfTile(focusTile);
	}

	DrawBufferArgs args;
	memset(&args, 0, sizeof args);
	args.Editor = true;
	args.GuideImage = &brush.GuideImagePic;
	args.GuideImageAlpha = brush.GuideImageAlpha;

	DrawBufferTerminate(&sDrawBuffer);
	ClearScreen(ec.g);
	DrawBufferInit(&sDrawBuffer, svec2i(X_TILES, Y_TILES), ec.g);
	GrafxMakeBackground(
		ec.g, &sDrawBuffer, &gCampaign, &gMission, &gMap, tintNone, true,
		ec.camera, &args);
}

// Returns whether a redraw is required
typedef struct
{
	bool Redraw;
	bool WillDisplayAutomap;
	bool Done;
} HandleInputResult;

static void Display(HandleInputResult result)
{
	char s[128];
	int y = 5;
	const int w = ec.g->cachedConfig.Res.x;
	const int h = ec.g->cachedConfig.Res.y;

	WindowContextPreRender(&ec.g->gameWindow);
	ClearScreen(ec.g);

	const Mission *mission = CampaignGetCurrentMission(&gCampaign);
	if (mission)
	{
		// Re-make the background if the resolution has changed
		if (gEventHandlers.HasResolutionChanged)
		{
			MakeBackground(false);
		}

		DrawBufferArgs args;
		memset(&args, 0, sizeof args);
		args.Editor = true;
		args.GuideImage = &brush.GuideImagePic;
		args.GuideImageAlpha = brush.GuideImageAlpha;
		GrafxDrawBackground(ec.g, &sDrawBuffer, tintNone, ec.camera, &args);
		BlitClearBuf(ec.g);

		// Draw brush highlight tiles
		if (brush.IsActive &&
			Rect2iIsInside(Rect2iNew(svec2i_zero(), mission->Size), brush.Pos))
		{
			EditorBrushSetHighlightedTiles(&brush);
		}
		CA_FOREACH(struct vec2i, pos, brush.HighlightedTiles)
		struct vec2i screenPos = GetScreenPos(*pos);
		if (screenPos.x >= 0 && screenPos.x < w && screenPos.y >= 0 &&
			screenPos.y < h)
		{
			DrawRectangle(ec.g, screenPos, TILE_SIZE, colorWhite, false);
		}
		CA_FOREACH_END()

		if (brush.LastPos.x)
		{
			sprintf(s, "(%d, %d)", brush.Pos.x, brush.Pos.y);
			FontStr(s, svec2i(w - 40, h - 16));
		}
	}

	if (fileChanged)
	{
		// Display a disk icon to show the game needs saving
		const Pic *pic = PicManagerGetPic(&gPicManager, "disk1");
		PicRender(
			pic, gGraphicsDevice.gameWindow.renderer, svec2i(10, y),
			colorWhite, 0, svec2_one(), SDL_FLIP_NONE, Rect2iZero());
	}

	FontStr(
		"Press " KMOD_CMD_NAME "+E to edit characters", svec2i(20, h - 20 - FontH() * 2));
	FontStr("Press F1 for help", svec2i(20, h - 20 - FontH()));

	UIObjectDraw(
		sObjs, ec.g, svec2i_zero(), gEventHandlers.mouse.currentPos,
		&sDrawObjs);

	if (result.WillDisplayAutomap && mission)
	{
		AutomapDraw(
			gGraphicsDevice.gameWindow.renderer, AUTOMAP_FLAGS_SHOWALL, true);
	}
	else
	{
		if (sTooltipObj && sTooltipObj->Tooltip)
		{
			UITooltipDraw(
				ec.g, gEventHandlers.mouse.currentPos, sTooltipObj->Tooltip);
		}
	}
	BlitUpdateFromBuf(ec.g, ec.g->screen);
	WindowContextPostRender(&ec.g->gameWindow);
}

static void Setup(const bool changedMission);

static void Change(UIObject *o, const int d, const bool shift)
{
	if (o == NULL)
	{
		return;
	}
	const EditorResult r = UIObjectChange(o, d, shift);
	if (r & EDITOR_RESULT_CHANGED)
	{
		fileChanged = true;
	}
	if (r & EDITOR_RESULT_CHANGED_AND_RELOAD)
	{
		Setup(false);
	}
}

static void AddObjective(Mission *m)
{
	// TODO: support more objectives
	if (m->Objectives.size < OBJECTIVE_MAX_OLD)
	{
		Objective o;
		memset(&o, 0, sizeof o);
		CArrayPushBack(&m->Objectives, &o);
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
}

static void AdjustXC(int yc, int *xc)
{
	Mission *mission = CampaignGetCurrentMission(&gCampaign);
	switch (yc)
	{
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
			buf, "%s~%d%s", dirname, sAutosaveIndex,
			PathGetBasename(lastFile));
		MapArchiveSave(buf, &gCampaign.Setting);
		sAutosaveIndex++;
	}
}

static void Setup(const bool changedMission)
{
	Mission *m = CampaignGetCurrentMission(&gCampaign);
	if (!m)
	{
		return;
	}
	MissionCopy(&lastMission, &currentMission);
	MissionCopy(&currentMission, m);
	MissionOptionsTerminate(&gMission);
	CampaignAndMissionSetup(&gCampaign, &gMission);
	MakeBackground(changedMission);

	Autosave();

	sJustLoaded = true;
	sHasUnbakedChanges = false;
}

// Reload UI so that we can load new elements based on custom data etc.
static void ReloadUI(void)
{
	UIObjectDestroy(sObjs);
	sObjs = CreateMainObjs(&gCampaign, &brush, svec2i(320, 240));
	CArrayTerminate(&sDrawObjs);
	sLastHighlightedObj = NULL;
	sTooltipObj = NULL;
}

static bool TryOpen(const char *filename);
static void ShowFailedToOpenMsg(const char *filename);
static void Open(void)
{
	char dirname[CDOGS_PATH_MAX];
	PathGetDirname(dirname, lastFile);
	char buf[CDOGS_PATH_MAX];
	RealPath(dirname, buf);
	FixPathSeparator(dirname, buf);
	osdialog_filters *filters =
		osdialog_filters_parse("C-Dogs SDL Campaign:cdogscpn");
	char *filename = osdialog_file(
		OSDIALOG_OPEN_DIR, dirname, PathGetBasename(lastFile), filters);
	// Try original filename
	if (filename)
	{
		WindowContextPreRender(&gGraphicsDevice.gameWindow);
		ClearScreen(&gGraphicsDevice);

		FontStrCenter("Loading...");

		BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.screen);
		WindowContextPostRender(&gGraphicsDevice.gameWindow);
		if (!TryOpen(filename))
		{
			ShowFailedToOpenMsg(filename);
		}
	}
	free(filename);
	osdialog_filters_free(filters);
}
static bool TryOpen(const char *filename)
{
	// Try opening a campaign
	CampaignSettingTerminateAll(&gCampaign.Setting);
	CampaignSettingInit(&gCampaign.Setting);
	char buf[CDOGS_PATH_MAX];
	RealPath(filename, buf);
	if (!MapNewLoad(buf, &gCampaign.Setting))
	{
		fileChanged = false;
		gCampaign.MissionIndex = 0;
		Setup(true);
		strcpy(lastFile, filename);
		sAutosaveIndex = 0;
		ReloadUI();
		return true;
	}
	return false;
}
static void ShowFailedToOpenMsg(const char *filename)
{
	char msgBuf[CDOGS_PATH_MAX];
	sprintf(msgBuf, "Failed to open file %s", filename);
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, "Error", msgBuf,
		gGraphicsDevice.gameWindow.window);
}

static void Save(void)
{
	char dirname[CDOGS_PATH_MAX];
	PathGetDirname(dirname, lastFile);
	osdialog_filters *filters =
		osdialog_filters_parse("C-Dogs SDL Campaign:cdogscpn");
	char *filename = osdialog_file(
		OSDIALOG_SAVE, dirname, PathGetBasename(lastFile), filters);
	if (filename)
	{
		WindowContextPreRender(&gGraphicsDevice.gameWindow);
		ClearScreen(&gGraphicsDevice);

		FontStrCenter("Saving...");

		BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.screen);
		WindowContextPostRender(&gGraphicsDevice.gameWindow);
		MapArchiveSave(filename, &gCampaign.Setting);
		fileChanged = false;
		strcpy(lastFile, filename);
		sAutosaveIndex = 0;
		char msgBuf[CDOGS_PATH_MAX];
		sprintf(msgBuf, "Saved to %s", filename);
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_INFORMATION, "Campaign Saved", msgBuf,
			gGraphicsDevice.gameWindow.window);
	}
	free(filename);
	osdialog_filters_free(filters);
}

static void HelpScreen(void)
{
	struct vec2i pos = svec2i(20, 20);
	const char *helpText =
		"Help\n"
		"====\n"
		"Use mouse to select controls; keyboard to type text\n"
		"Open files by dragging them over the editor shortcut\n"
		"\n"
		"Common commands\n"
		"===============\n"
		"left/right click, page up/down: Increase/decrease value\n"
		"shift + left/right click:          Increase/decrease number of items\n"
		"insert:                         Add new item\n"
		"delete:                         Delete selected item\n"
		"arrow keys:                     Move camera\n"
		"\n"
		"Other commands\n"
		"==============\n"
		"Escape:                         Back or quit\n"
		KMOD_CMD_NAME "+E:                         Go to character editor\n"
		KMOD_CMD_NAME"+N:                         New mission\n"
		KMOD_CMD_NAME"+O:                         Open file\n"
		KMOD_CMD_NAME"+S:                         Save file\n"
		KMOD_CMD_NAME"+X, C, V:                   Cut/copy/paste\n"
		"tab:                              Preview automap\n"
		"F1:                               This screen\n";
	WindowContextPreRender(&gGraphicsDevice.gameWindow);
	ClearScreen(&gGraphicsDevice);
	FontStr(helpText, pos);
	BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.screen);
	WindowContextPostRender(&gGraphicsDevice.gameWindow);
	GetKey(&gEventHandlers);
}

static void Delete(int xc, int yc)
{
	Mission *mission = CampaignGetCurrentMission(&gCampaign);
	bool changedMission = false;
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
			CampaignDeleteMission(&gCampaign, gCampaign.MissionIndex);
			changedMission = true;
		}
		AdjustYC(&yc);
		break;
	}
	fileChanged = true;
	Setup(changedMission);
}

static UIObject *OnUIInput(HandleInputResult *result, const EventHandlers *event, const int m, int *xc, int *yc, int *xcOld, int *ycOld, SDL_Scancode *sc, const Mission *mission);
static void InputInsert(int *xc, const int yc, Mission *mission);
static void InputDelete(const int xc, const int yc);
static HandleInputResult HandleInput(
	SDL_Scancode sc, const int m, int *xc, int *yc, int *xcOld, int *ycOld,
	Mission *scrap)
{
	HandleInputResult result = {false, false, false};
	Mission *mission = CampaignGetCurrentMission(&gCampaign);
	UIObject *o = NULL;
	const struct vec2i brushLastDrawPos = brush.Pos;
	brush.Pos = GetMouseTile(&gEventHandlers);
	const bool shift = gEventHandlers.keyboard.modState & KMOD_SHIFT;

	// Find whether the mouse has hovered over a tooltip
	const bool hadTooltip = sTooltipObj != NULL;
	bool isContextMenu = false;
	sTooltipObj = NULL;
	const struct vec2i mousePos = gEventHandlers.mouse.currentPos;
	UITryGetObject(sLastHighlightedObj, mousePos, &sTooltipObj);
	if (sTooltipObj == NULL)
	{
		UITryGetObject(sObjs, mousePos, &sTooltipObj);
	}
	if (sTooltipObj != NULL)
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
	if (brush.IsActive && !svec2i_is_equal(brushLastDrawPos, brush.Pos))
	{
		result.Redraw = true;
	}

	if (m && (m == SDL_BUTTON_LEFT || m == SDL_BUTTON_RIGHT ||
			  MouseWheel(&gEventHandlers.mouse).y != 0))
	{
		o = OnUIInput(&result, &gEventHandlers, m, xc, yc, xcOld, ycOld, &sc, mission);
	}
	if (!brush.IsActive)
	{
		MouseSetCursor(&gEventHandlers.mouse, SDL_SYSTEM_CURSOR_ARROW);
	}
	if (!o &&
		(MouseIsDown(&gEventHandlers.mouse, SDL_BUTTON_LEFT) ||
		 MouseIsDown(&gEventHandlers.mouse, SDL_BUTTON_RIGHT)) &&
		!sIgnoreMouse)
	{
		result.Redraw = true;
		if (brush.IsActive && mission && mission->Type == MAPTYPE_STATIC)
		{
			// Draw a tile
			if (Rect2iIsInside(
					Rect2iNew(svec2i_zero(), mission->Size), brush.Pos))
			{
				const bool isMain =
					MouseIsDown(&gEventHandlers.mouse, SDL_BUTTON_LEFT);
				const EditorResult r =
					EditorBrushStartPainting(&brush, mission, isMain);
				if (r & EDITOR_RESULT_CHANGED)
				{
					fileChanged = true;
					Autosave();
					sHasUnbakedChanges = true;
				}
				if (r & EDITOR_RESULT_RELOAD)
				{
					Setup(false);
				}
			}
		}
	}
	else
	{
		if (mission)
		{
			// Clamp brush position
			brush.Pos = svec2i_clamp(
				brush.Pos, svec2i_zero(),
				svec2i_subtract(mission->Size, svec2i_one()));
			const EditorResult r = EditorBrushStopPainting(&brush, mission);
			if (r & EDITOR_RESULT_CHANGED)
			{
				fileChanged = true;
				Autosave();
				result.Redraw = true;
				sHasUnbakedChanges = true;
			}
			if (r & EDITOR_RESULT_RELOAD)
			{
				Setup(false);
			}
		}
	}
	// Pan the camera based on keyboard cursor keys
	if (mission)
	{
		if (KeyIsDown(&gEventHandlers.keyboard, SDL_SCANCODE_LEFT))
		{
			ec.camera.x -= CAMERA_PAN_SPEED;
			result.Redraw = true;
		}
		else if (KeyIsDown(&gEventHandlers.keyboard, SDL_SCANCODE_RIGHT))
		{
			ec.camera.x += CAMERA_PAN_SPEED;
			result.Redraw = true;
		}
		if (KeyIsDown(&gEventHandlers.keyboard, SDL_SCANCODE_UP))
		{
			ec.camera.y -= CAMERA_PAN_SPEED;
			result.Redraw = true;
		}
		else if (KeyIsDown(&gEventHandlers.keyboard, SDL_SCANCODE_DOWN))
		{
			ec.camera.y += CAMERA_PAN_SPEED;
			result.Redraw = true;
		}
		// Also pan the camera based on middle mouse drag
		if (MouseIsDown(&gEventHandlers.mouse, SDL_BUTTON_MIDDLE))
		{
			ec.camera = svec2_add(
				ec.camera, svec2_assign_vec2i(svec2i_subtract(
							   gEventHandlers.mouse.previousPos,
							   gEventHandlers.mouse.currentPos)));
			result.Redraw = true;
		}

		// Change cursor to hand while dragging
		if (MouseIsPressed(&gEventHandlers.mouse, SDL_BUTTON_MIDDLE))
		{
			MouseSetCursor(&gEventHandlers.mouse, SDL_SYSTEM_CURSOR_SIZEALL);
		}
		if (MouseIsReleased(&gEventHandlers.mouse, SDL_BUTTON_MIDDLE))
		{
			MouseSetCursor(&gEventHandlers.mouse, SDL_SYSTEM_CURSOR_ARROW);
		}
		ec.camera.x = CLAMP(ec.camera.x, 0, Vec2CenterOfTile(mission->Size).x);
		ec.camera.y = CLAMP(ec.camera.y, 0, Vec2CenterOfTile(mission->Size).y);
	}
	bool hasQuit = false;
	if (sc != SDL_SCANCODE_UNKNOWN)
	{
		result.Redraw = true;
	}
	if (gEventHandlers.keyboard.modState & KMOD_CMD)
	{
		const SDL_Keycode kc = SDL_GetKeyFromScancode(sc);
		switch (kc)
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
				MissionCopy(&lastMission, mission); // B,A,Z -> B,A,B
			}
			else if (mission != NULL)
			{
				MissionCopy(mission, &lastMission);			// B,B,A -> A,B,A
				MissionCopy(&lastMission, &currentMission); // A,B,A -> A,B,B
			}
			fileChanged = true;
			Setup(false); // A,B,B -> A,A,B
			break;

		case 'x':
			MissionTerminate(scrap);
			MissionCopy(scrap, mission);
			Delete(*xc, *yc);
			// Unhighlight everything (in case this is the last mission)
			UIObjectUnhighlight(sObjs, true);
			CArrayTerminate(&sDrawObjs);
			sLastHighlightedObj = NULL;
			break;

		case 'c':
			MissionTerminate(scrap);
			MissionCopy(scrap, mission);
			break;

		case 'v':
			// Use map size as a proxy to whether there's a valid scrap mission
			if (!svec2i_is_zero(scrap->Size))
			{
				InsertMission(&gCampaign, scrap, gCampaign.MissionIndex);
				fileChanged = true;
				Setup(false);
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
			InsertMission(
				&gCampaign, NULL, (int)gCampaign.Setting.Missions.size);
			gCampaign.MissionIndex =
				(int)(gCampaign.Setting.Missions.size - 1);
			fileChanged = true;
			Setup(true);
			break;

		case 'o':
			if (!fileChanged || ConfirmScreen(
									"File has been modified, but not saved",
									"Open anyway? (Y/N)"))
			{
				Open();
			}
			break;

		case 's':
			Save();
			break;

		case 'e':
			CharEditor(
				ec.g, &gCampaign.Setting, &gEventHandlers, &fileChanged);
			Setup(false);
			UIObjectUnhighlight(sObjs, true);
			CArrayTerminate(&sDrawObjs);
			break;

		default:
			// do nothing
			break;
		}
	}
	else
	{
		switch (sc)
		{
		case SDL_SCANCODE_F1:
			HelpScreen();
			break;

		case SDL_SCANCODE_HOME:
			if (gCampaign.MissionIndex > 0)
			{
				gCampaign.MissionIndex--;
			}
			Setup(true);
			break;

		case SDL_SCANCODE_END:
			if (gCampaign.MissionIndex < (int)gCampaign.Setting.Missions.size)
			{
				gCampaign.MissionIndex++;
			}
			Setup(true);
			break;

		case SDL_SCANCODE_INSERT:
			InputInsert(xc, *yc, mission);
			break;

		case SDL_SCANCODE_DELETE:
			InputDelete(*xc, *yc);
			break;

		case SDL_SCANCODE_PAGEUP:
			Change(o, 1, shift);
			break;

		case SDL_SCANCODE_PAGEDOWN:
			Change(o, -1, shift);
			break;

		case SDL_SCANCODE_ESCAPE:
			hasQuit = true;
			break;

		case SDL_SCANCODE_GRAVE:
			ToggleCollapse(sObjs->Data, 0);
			OnUIInput(&result, &gEventHandlers, m, xc, yc, xcOld, ycOld, &sc, mission);
			break;

		case SDL_SCANCODE_BACKSPACE:
			fileChanged = UIObjectDelChar(sObjs) || fileChanged;
			break;

		case SDL_SCANCODE_KP_0:
			if (!(SDL_GetModState() & KMOD_NUM))
			{
				InputInsert(xc, *yc, mission);
			}
			break;

		case SDL_SCANCODE_KP_PERIOD:
			if (!(SDL_GetModState() & KMOD_NUM))
			{
				InputDelete(*xc, *yc);
			}
			break;
		
		case SDL_SCANCODE_TAB:
			result.WillDisplayAutomap = true;
			break;

		default:
			// Do nothing
			break;
		}
		// Get text input, ASCII only
		char *c = gEventHandlers.keyboard.Typed;
		while (c && *c >= ' ' && *c <= '~')
		{
			fileChanged = UIObjectAddChar(sObjs, *c) || fileChanged;
			c++;
		}
	}
	if (gEventHandlers.DropFile != NULL)
	{
		// Copy to buf because ConfirmScreen will cause the DropFile to be lost
		char buf[CDOGS_PATH_MAX];
		strcpy(buf, gEventHandlers.DropFile);
		if (IsCampaignOK(buf, NULL, NULL))
		{
			if (!fileChanged || ConfirmScreen(
									"File has been modified, but not saved",
									"Open anyway? (Y/N)"))
			{
				if (!TryOpen(buf))
				{
					ShowFailedToOpenMsg(buf);
				}
			}
		}
		else if (mission && mission->Type == MAPTYPE_STATIC)
		{
			// Try to load guide image
			if (EditorBrushTryLoadGuideImage(&brush, buf))
			{
				strcpy(brush.GuideImage, buf);
				result.Redraw = true;
			}
		}
	}
	if (gEventHandlers.HasQuit)
	{
		hasQuit = true;
		// Don't let the program quit yet; wait for confirmation screen
		gEventHandlers.HasQuit = false;
	}
	if (hasQuit)
	{
		// Make sure we redraw so if the user has cancelled the quit confirm
		// the editor reappears
		result.Redraw = true;
		result.Done =
			!fileChanged ||
			ConfirmScreen(
				"File has been modified, but not saved", "Quit anyway? (Y/N)");
	}
	if (!MouseIsDown(&gEventHandlers.mouse, SDL_BUTTON_LEFT) &&
		!MouseIsDown(&gEventHandlers.mouse, SDL_BUTTON_RIGHT))
	{
		sIgnoreMouse = false;
	}
	return result;
}
UIObject *OnUIInput(HandleInputResult *result, const EventHandlers *event, const int m, int *xc, int *yc, int *xcOld, int *ycOld, SDL_Scancode *sc, const Mission *mission)
{
	result->Redraw = true;
	UIObject *o = NULL;
	const struct vec2i mousePos = event->mouse.currentPos;
	const bool shift = event->keyboard.modState & KMOD_SHIFT;
	if (sLastHighlightedObj && !sLastHighlightedObj->IsBackground)
	{
		UITryGetObject(sLastHighlightedObj, mousePos, &o);
	}
	if (o == NULL)
	{
		UITryGetObject(sObjs, mousePos, &o);
	}
	if (o != NULL)
	{
		if (!o->DoNotHighlight)
		{
			if (sLastHighlightedObj)
			{
				if (UIObjectUnhighlight(sLastHighlightedObj, true))
				{
					Setup(false);
				}
			}
			sLastHighlightedObj = o;
			UIObjectHighlight(o, shift);
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
			(!(o->Flags & UI_SELECT_ONLY_FIRST) ||
			 (*xc == *xcOld && *yc == *ycOld)))
		{
			if (m == SDL_BUTTON_LEFT ||
				MouseWheel(&gEventHandlers.mouse).y > 0)
			{
				*sc = SDL_SCANCODE_PAGEUP;
			}
			else if (
				m == SDL_BUTTON_RIGHT ||
				MouseWheel(&gEventHandlers.mouse).y < 0)
			{
				*sc = SDL_SCANCODE_PAGEDOWN;
			}
		}
	}
	else
	{
		if (!(brush.IsActive && mission))
		{
			UIObjectUnhighlight(sObjs, true);
			CArrayTerminate(&sDrawObjs);
			sLastHighlightedObj = NULL;
		}
	}
	return o;
}
static void InputInsert(int *xc, const int yc, Mission *mission)
{
	bool changedMission = false;
	switch (yc)
	{
	case YC_CHARACTERS:
		if (gCampaign.Setting.characters.OtherChars.size > 0)
		{
			int ch = 0;
			CArrayPushBack(&mission->Enemies, &ch);
			CharacterStoreAddBaddie(&gCampaign.Setting.characters, ch);
			*xc = (int)(mission->Enemies.size - 1);
		}
		break;

	case YC_SPECIALS:
		if (gCampaign.Setting.characters.OtherChars.size > 0)
		{
			int ch = 0;
			CArrayPushBack(&mission->SpecialChars, &ch);
			CharacterStoreAddSpecial(&gCampaign.Setting.characters, ch);
			*xc = (int)(mission->SpecialChars.size - 1);
		}
		break;

	case YC_ITEMS: {
		MapObjectDensity mod;
		mod.M = IndexMapObject(0);
		mod.Density = 0;
		CArrayPushBack(&mission->MapObjectDensities, &mod);
		*xc = (int)(mission->MapObjectDensities.size - 1);
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
			changedMission = true;
		}
		break;
	}
	fileChanged = true;
	Setup(changedMission);
}
static void InputDelete(const int xc, const int yc)
{
	Delete(xc, yc);
	// Unhighlight everything (in case this is the last mission)
	UIObjectUnhighlight(sObjs, true);
	CArrayTerminate(&sDrawObjs);
	sLastHighlightedObj = NULL;
}

static void EditCampaign(void)
{
	int xc = 0, yc = 0;
	int xcOld, ycOld;
	Mission scrap;
	memset(&scrap, 0, sizeof scrap);

	Setup(true);

	Uint32 ticksNow = SDL_GetTicks();
	sTicksElapsed = 0;
	Uint32 ticksElapsed = 0;
	ticksAutosave = AUTOSAVE_INTERVAL_SECONDS * 1000;
	SDL_StartTextInput();
	for (;;)
	{
		Uint32 ticksThen = ticksNow;
		ticksNow = SDL_GetTicks();
		ticksElapsed += ticksNow - ticksThen;
		if (ticksElapsed < 1000 / FPS_FRAMELIMIT * 2)
		{
			SDL_Delay(1);
			continue;
		}

		EventPoll(&gEventHandlers, ticksElapsed, NULL);
		const SDL_Scancode sc = KeyGetPressed(&gEventHandlers.keyboard);
		const int m = MouseGetPressed(&gEventHandlers.mouse);

		HandleInputResult result =
			HandleInput(sc, m, &xc, &yc, &xcOld, &ycOld, &scrap);
		if (result.Done)
		{
			break;
		}
		if (result.Redraw || sJustLoaded)
		{
			sJustLoaded = false;
			LOG(LM_EDIT, LL_TRACE, "Drawing UI");
			Display(result);
			if (result.WillDisplayAutomap)
			{
				GetKey(&gEventHandlers);
			}
		}
		sTicksElapsed += ticksElapsed;
		ticksElapsed = 0;
	}
	SDL_StopTextInput();
}

static void ResetLastFile(char *s);
int main(int argc, char *argv[])
{
#if defined(_MSC_VER) && !defined(NDEBUG)
	FreeConsole();
#endif
	int loaded = 0;

	// Print command line
	char buf[CDOGS_PATH_MAX];
	struct option longopts[] = {{"log", required_argument, NULL, 1000},
								{"logfile", required_argument, NULL, 1001},
								{0, 0, NULL, 0}};
	int opt = 0;
	int idx = 0;
	while ((opt = getopt_long(argc, argv, "\0:\0", longopts, &idx)) != -1)
	{
		switch (opt)
		{
		case 1000: {
			char *comma = strchr(optarg, ',');
			if (comma)
			{
				// Set logging level for a single module
				// The module and level are comma separated
				*comma = '\0';
				const LogLevel ll = StrLogLevel(comma + 1);
				LogModuleSetLevel(StrLogModule(optarg), ll);
				printf("Logging %s at %s\n", optarg, LogLevelName(ll));
			}
			else
			{
				// Set logging level for all modules
				const LogLevel ll = StrLogLevel(optarg);
				for (int i = 0; i < (int)LM_COUNT; i++)
				{
					LogModuleSetLevel((LogModule)i, ll);
				}
				printf("Logging everything at %s\n", LogLevelName(ll));
			}
		}
		break;
		case 1001:
			LogOpenFile(optarg);
			break;
		default:
			// Ignore unknown arguments
			break;
		}
	}

	LogInit();
	printf("C-Dogs SDL Editor\n");

	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) != 0)
	{
		printf("Failed to start SDL!\n");
		return -1;
	}

	GetDataFilePath(buf, "");
	printf("Data directory:\t\t%s\n", buf);
	printf("Config directory:\t%s\n\n", GetConfigFilePath(""));

	ec.g = &gGraphicsDevice;
	ec.camera = svec2_zero();

	EditorBrushInit(&brush);
	ResetLastFile(lastFile);

	gConfig = ConfigLoad(GetConfigFilePath(CONFIG_FILE));
	PicManagerInit(&gPicManager);
	// Hardcode config settings
	ConfigGet(&gConfig, "Graphics.ScaleFactor")->u.Int.Value = 2;
	ConfigGet(&gConfig, "Graphics.ScaleMode")->u.Enum.Value = SCALE_MODE_NN;
	ConfigGet(&gConfig, "Graphics.WindowWidth")->u.Int.Value = 800;
	ConfigGet(&gConfig, "Graphics.WindowHeight")->u.Int.Value = 600;
	ConfigGet(&gConfig, "Graphics.SecondWindow")->u.Bool.Value = false;
	ConfigSetChanged(&gConfig);
	GraphicsInit(ec.g, &gConfig);
	ec.g->cachedConfig.IsEditor = true;
	GraphicsInitialize(ec.g);
	if (!ec.g->IsInitialized)
	{
		printf("Video didn't init!\n");
		exit(EXIT_FAILURE);
	}
	FontLoadFromJSON(&gFont, "graphics/font.png", "graphics/font.json");
	PicManagerLoad(&gPicManager);
	CharSpriteClassesInit(&gCharSpriteClasses);

	ParticleClassesInit(&gParticleClasses, "data/particles.json");
	AmmoInitialize(&gAmmo, "data/ammo.json");
	BulletAndWeaponInitialize(
		&gBulletClasses, &gWeaponClasses, "data/bullets.json",
		"data/guns.json");
	CharacterClassesInitialize(
		&gCharacterClasses, "data/character_classes.json");
	PlayerTemplatesLoad(&gPlayerTemplates, &gCharacterClasses);
	PickupClassesInit(
		&gPickupClasses, "data/pickups.json", &gAmmo, &gWeaponClasses);
	MapObjectsInit(
		&gMapObjects, "data/map_objects.json", &gAmmo, &gWeaponClasses);
	CollisionSystemInit(&gCollisionSystem);
	MapWolfInit();
	CampaignInit(&gCampaign);
	MissionInit(&lastMission);
	MissionInit(&currentMission);

	// initialise UI collections
	// Note: must do this after text init since positions depend on text height
	sObjs = CreateMainObjs(&gCampaign, &brush, svec2i(320, 240));
	memset(&sDrawObjs, 0, sizeof sDrawObjs);
	DrawBufferInit(&sDrawBuffer, svec2i(X_TILES, Y_TILES), &gGraphicsDevice);

	// Reset campaign (graphics init may have created dummy campaigns)
	CampaignSettingTerminateAll(&gCampaign.Setting);
	CampaignSettingInit(&gCampaign.Setting);

	EventInit(&gEventHandlers);

	for (int i = 1; i < argc; i++)
	{
		if (!loaded)
		{
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
				LOG(LM_EDIT, LL_INFO, "Loaded map %s", lastFile);
			}
			else
			{
				ResetLastFile(lastFile);
			}
		}
	}

	EditCampaign();

	MapTerminate(&gMap);
	MapObjectsTerminate(&gMapObjects);
	PickupClassesTerminate(&gPickupClasses);
	ParticleClassesTerminate(&gParticleClasses);
	AmmoTerminate(&gAmmo);
	WeaponClassesTerminate(&gWeaponClasses);
	BulletTerminate(&gBulletClasses);
	CharacterClassesTerminate(&gCharacterClasses);
	MapWolfTerminate();
	CampaignTerminate(&gCampaign);
	MissionTerminate(&lastMission);
	MissionTerminate(&currentMission);
	CollisionSystemTerminate(&gCollisionSystem);

	DrawBufferTerminate(&sDrawBuffer);
	GraphicsTerminate(ec.g);
	CharSpriteClassesTerminate(&gCharSpriteClasses);
	PicManagerTerminate(&gPicManager);
	FontTerminate(&gFont);
	PlayerTemplatesTerminate(&gPlayerTemplates);

	UIObjectDestroy(sObjs);
	CArrayTerminate(&sDrawObjs);
	EditorBrushTerminate(&brush);

	ConfigDestroy(&gConfig);
	LogTerminate();

	SDL_Quit();

	exit(EXIT_SUCCESS);
}
static void ResetLastFile(char *s)
{
	char buf[CDOGS_PATH_MAX];
	// initialise to missions dir
	GetDataFilePath(buf, CDOGS_CAMPAIGN_DIR);
	RelPath(s, buf, ".");
	strcat(s, "/");
}
