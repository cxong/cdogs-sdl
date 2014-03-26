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
#include <assert.h>
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
#include <cdogs/mission_convert.h>
#include <cdogs/objs.h>
#include <cdogs/palette.h>
#include <cdogs/pic_manager.h>
#include <cdogs/text.h>
#include <cdogs/triggers.h>
#include <cdogs/utils.h>

#include <cdogsed/charsed.h>
#include <cdogsed/editor_ui.h>
#include <cdogsed/ui_object.h>


#define TH  8


// Mouse click areas:
static UIObject *sObjs;
static CArray sDrawObjs;	// of UIObjectDrawContext, used to cache BFS order
static UIObject *sLastHighlightedObj = NULL;
Vec2i sUIOverlaySize = { 320, 240 };
static DrawBuffer sDrawBuffer;


// Globals

static char lastFile[CDOGS_PATH_MAX];
static EditorBrush brush;
static Tile sCursorTile;
Vec2i camera = { 0, 0 };
#define CAMERA_PAN_SPEED 8
int hasCameraMoved = 0;
Mission currentMission;
Mission lastMission;
#define AUTOSAVE_INTERVAL 10
int numChanges = 0;


static Vec2i GetMouseTile(GraphicsDevice *g, EventHandlers *e)
{
	int w = g->cachedConfig.ResolutionWidth;
	int h = g->cachedConfig.ResolutionHeight;
	Mission *m = CampaignGetCurrentMission(&gCampaign);
	if (!m)
	{
		return Vec2iNew(-1, -1);
	}
	else
	{
		Vec2i mapPos = Vec2iNew(w / 2 - camera.x, h / 2 - camera.y);
		return Vec2iNew(
			(e->mouse.currentPos.x - mapPos.x - 8) / TILE_WIDTH,
			(e->mouse.currentPos.y - mapPos.y - 12) / TILE_HEIGHT);
	}
}

static int IsBrushPosValid(Vec2i pos, Mission *m)
{
	return pos.x >= 0 && pos.x < m->Size.x &&
		pos.y >= 0 && pos.y < m->Size.y;
}

static void MakeBackground(GraphicsDevice *g, int buildTables)
{
	int i;
	// Clear background first
	for (i = 0; i < GraphicsGetScreenSize(&g->cachedConfig); i++)
	{
		g->buf[i] = PixelFromColor(g, colorBlack);
	}
	GrafxDrawExtra extra;
	extra.highlightedTiles = &brush.HighlightedTiles;
	extra.guideImage = brush.GuideImageSurface;
	extra.guideImageAlpha = brush.GuideImageAlpha;

	DrawBufferTerminate(&sDrawBuffer);
	DrawBufferInit(&sDrawBuffer, Vec2iNew(X_TILES, Y_TILES), &gGraphicsDevice);
	GrafxMakeBackground(
		g, &sDrawBuffer, &gCampaign, &gMission, &gMap,
		tintNone, 1, buildTables, camera, &extra);
}

static void Display(GraphicsDevice *g, int yc, int willDisplayAutomap)
{
	char s[128];
	int y = 5;
	int i;
	int w = g->cachedConfig.ResolutionWidth;
	int h = g->cachedConfig.ResolutionHeight;
	Mission *mission = CampaignGetCurrentMission(&gCampaign);

	if (mission)
	{
		Vec2i v;
		// Re-make the background if the resolution has changed
		if (gEventHandlers.HasResolutionChanged)
		{
			MakeBackground(g, 0);
		}
		if ((brush.IsActive && IsBrushPosValid(brush.Pos, mission)) ||
			hasCameraMoved ||
			brush.IsGuideImageNew)
		{
			if (brush.IsActive && IsBrushPosValid(brush.Pos, mission))
			{
				EditorBrushSetHighlightedTiles(&brush);
			}
			// Clear background first
			for (i = 0; i < GraphicsGetScreenSize(&g->cachedConfig); i++)
			{
				g->buf[i] = PixelFromColor(g, colorBlack);
			}
			brush.IsGuideImageNew = false;
			GrafxDrawExtra extra;
			extra.highlightedTiles = &brush.HighlightedTiles;
			extra.guideImage = brush.GuideImageSurface;
			extra.guideImageAlpha = brush.GuideImageAlpha;
			GrafxDrawBackground(g, &sDrawBuffer, tintNone, camera, &extra);
		}
		GraphicsBlitBkg(g);
		// Draw overlay
		for (v.y = 0; v.y < sUIOverlaySize.y; v.y++)
		{
			for (v.x = 0; v.x < sUIOverlaySize.x; v.x++)
			{
				if (v.x < w && v.y < h)
				{
					DrawPointTint(g, v, tintDarker);
				}
			}
		}
		sprintf(
			s, "Mission %d/%d",
			gCampaign.MissionIndex + 1, (int)gCampaign.Setting.Missions.size);
		TextStringMasked(&gTextManager, 
			s, g, Vec2iNew(270, y),
			yc == YC_MISSIONINDEX ? colorRed : colorWhite);
		if (brush.LastPos.x)
		{
			sprintf(s, "(%d, %d)", brush.Pos.x, brush.Pos.y);
			TextString(&gTextManager, s, g, Vec2iNew(w - 40, h - 16));
		}
	}
	else
	{
		for (i = 0; i < GraphicsGetScreenSize(&g->cachedConfig); i++)
		{
			g->buf[i] = LookupPalette(58);
		}
		if (gCampaign.Setting.Missions.size)
		{
			sprintf(s, "End/%d", (int)gCampaign.Setting.Missions.size);
			TextStringMasked(&gTextManager, 
				s, g, Vec2iNew(270, y),
				yc == YC_MISSIONINDEX ? colorRed : colorWhite);
		}
	}

	if (fileChanged)
	{
		DrawTPic(10, y, PicManagerGetOldPic(&gPicManager, 221));
	}

	TextString(&gTextManager, 
		"Press F1 for help", g, Vec2iNew(20, h - 20 - CDogsTextHeight()));

	y = 150;

	UIObjectDraw(
		sObjs, g, Vec2iZero(), gEventHandlers.mouse.currentPos, &sDrawObjs);

	if (willDisplayAutomap && mission)
	{
		AutomapDraw(AUTOMAP_FLAGS_SHOWALL, true);
	}
	else
	{
		UIObject *o;
		if (UITryGetObject(sObjs, gEventHandlers.mouse.currentPos, &o) &&
			o->Tooltip)
		{
			UITooltipDraw(g, gEventHandlers.mouse.currentPos, o->Tooltip);
		}
		MouseDraw(&gEventHandlers.mouse);
	}
	BlitFlip(g, &gConfig.Graphics);
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

	if (!CampaignGetCurrentMission(&gCampaign))
	{
		return 0;
	}

	if (o)
	{
		isChanged = UIObjectChange(o, d);
	}
	return isChanged;
}

static void InsertMission(Mission *mission)
{
	if (mission)
	{
		CArrayInsert(
			&gCampaign.Setting.Missions, gCampaign.MissionIndex, mission);
	}
	else
	{
		Mission defaultMission;
		MissionInit(&defaultMission);
		defaultMission.Size = Vec2iNew(48, 48);
		CArrayInsert(
			&gCampaign.Setting.Missions,
			gCampaign.MissionIndex,
			&defaultMission);
	}
}

static void DeleteMission(void)
{
	if (gCampaign.MissionIndex >= (int)gCampaign.Setting.Missions.size)
	{
		return;
	}
	MissionTerminate(CampaignGetCurrentMission(&gCampaign));
	CArrayDelete(&gCampaign.Setting.Missions, gCampaign.MissionIndex);
	if (gCampaign.Setting.Missions.size > 0 &&
		gCampaign.MissionIndex >= (int)gCampaign.Setting.Missions.size)
	{
		gCampaign.MissionIndex = gCampaign.Setting.Missions.size - 1;
	}
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
	CArrayDelete(&m->Objectives, idx);
}

static void DeleteCharacter(Mission *m, int idx)
{
	CArrayDelete(&m->Enemies, idx);
	CharacterStoreDeleteBaddie(&gCampaign.Setting.characters, idx);
}

static void DeleteSpecial(Mission *m, int idx)
{
	CArrayDelete(&m->SpecialChars, idx);
	CharacterStoreDeleteSpecial(&gCampaign.Setting.characters, idx);
}

static void DeleteItem(Mission *m, int idx)
{
	CArrayDelete(&m->Items, idx);
	CArrayDelete(&m->ItemDensities, idx);
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
		if (mission && mission->Items.size > 0)
		{
			*xc = CLAMP_OPPOSITE(*xc, 0, (int)mission->Items.size - 1);
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

static void Autosave(int idx)
{
	char buf[CDOGS_PATH_MAX];
	sprintf(buf, "%s~%d", lastFile, idx);
	MapNewSave(buf, &gCampaign.Setting);
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

	numChanges++;
	if ((numChanges % AUTOSAVE_INTERVAL) == 0)
	{
		int autosaveIndex = numChanges / AUTOSAVE_INTERVAL;
		Autosave(autosaveIndex);
	}
}

static void Open(void)
{
	char filename[CDOGS_PATH_MAX];
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
				CampaignSettingTerminate(&gCampaign.Setting);
				CampaignSettingInit(&gCampaign.Setting);
				if (MapNewLoad(filename, &gCampaign.Setting))
				{
					printf("Error: cannot load %s\n", lastFile);
					continue;
				}
				Setup(1);
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

static void Save(void)
{
	char filename[CDOGS_PATH_MAX];
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
			{
				break;
			}
			MapNewSave(filename, &gCampaign.Setting);
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

static int ConfirmClose(char *msg)
{
	int c;
	int i;
	int w = gGraphicsDevice.cachedConfig.ResolutionWidth;
	int h = gGraphicsDevice.cachedConfig.ResolutionHeight;
	const char *s1 = "Campaign has been modified, but not saved";
	for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
	{
		gGraphicsDevice.buf[i] = LookupPalette(58);
	}
	TextString(&gTextManager, 
		s1,
		&gGraphicsDevice,
		Vec2iNew((w - TextGetStringWidth(s1)) / 2, (h - CDogsTextHeight()) / 2));
	TextString(&gTextManager, 
		msg,
		&gGraphicsDevice,
		Vec2iNew((w - TextGetStringWidth(msg)) / 2, (h + CDogsTextHeight()) / 2));
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
	int i;
	for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
	{
		gGraphicsDevice.buf[i] = LookupPalette(58);
	}
	TextString(&gTextManager, helpText, &gGraphicsDevice, pos);
	BlitFlip(&gGraphicsDevice, &gConfig.Graphics);
	GetKey(&gEventHandlers);
}

static void Delete(int xc, int yc)
{
	Mission *mission = CampaignGetCurrentMission(&gCampaign);
	switch (yc)
	{
	case YC_CHARACTERS:
		DeleteCharacter(mission, xc);
		break;

	case YC_SPECIALS:
		DeleteSpecial(mission, xc);
		break;

	case YC_ITEMS:
		DeleteItem(mission, xc);
		break;

	default:
		if (yc >= YC_OBJECTIVES)
		{
			DeleteObjective(mission, yc - YC_OBJECTIVES);
		}
		else
		{
			DeleteMission();
		}
		AdjustYC(&yc);
		break;
	}
	fileChanged = 1;
	Setup(0);
}

static void HandleInput(
	int c, int m,
	int *xc, int *yc, int *xcOld, int *ycOld,
	Mission *scrap, int *willDisplayAutomap, int *done)
{
	Mission *mission = CampaignGetCurrentMission(&gCampaign);
	UIObject *o = NULL;
	brush.Pos = GetMouseTile(&gGraphicsDevice, &gEventHandlers);
	if (m)
	{
		if (UITryGetObject(sObjs, gEventHandlers.mouse.currentPos, &o))
		{
			if (sLastHighlightedObj)
			{
				UIObjectUnhighlight(sLastHighlightedObj);
			}
			sLastHighlightedObj = o;
			UIObjectHighlight(o);
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
		MouseIsDown(&gEventHandlers.mouse, SDL_BUTTON_RIGHT)))
	{
		if (brush.IsActive &&
			(gEventHandlers.mouse.currentPos.x >= sUIOverlaySize.x ||
			gEventHandlers.mouse.currentPos.y >= sUIOverlaySize.y) &&
			mission->Type == MAPTYPE_STATIC)
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
			}
			if (r == EDITOR_RESULT_CHANGED_AND_RELOAD)
			{
				Setup(0);
			}
		}
	}
	// Pan the camera based on keyboard cursor keys
	hasCameraMoved = 0;
	if (mission)
	{
		if (KeyIsDown(&gEventHandlers.keyboard, SDLK_LEFT))
		{
			camera.x -= CAMERA_PAN_SPEED;
			hasCameraMoved = 1;
		}
		else if (KeyIsDown(&gEventHandlers.keyboard, SDLK_RIGHT))
		{
			camera.x += CAMERA_PAN_SPEED;
			hasCameraMoved = 1;
		}
		if (KeyIsDown(&gEventHandlers.keyboard, SDLK_UP))
		{
			camera.y -= CAMERA_PAN_SPEED;
			hasCameraMoved = 1;
		}
		else if (KeyIsDown(&gEventHandlers.keyboard, SDLK_DOWN))
		{
			camera.y += CAMERA_PAN_SPEED;
			hasCameraMoved = 1;
		}
		camera.x = CLAMP(camera.x, 0, Vec2iCenterOfTile(mission->Size).x);
		camera.y = CLAMP(camera.y, 0, Vec2iCenterOfTile(mission->Size).y);
	}
	bool hasQuit = false;
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
			MissionCopy(mission, &lastMission);	// B,B,A -> A,B,A
			MissionCopy(&lastMission, &currentMission);	// A,B,A -> A,B,B
			fileChanged = 1;
			Setup(0);	// A,B,B -> A,A,B
			break;

		case 'x':
			MissionTerminate(scrap);
			MissionCopy(scrap, mission);
			Delete(*xc, *yc);
			break;

		case 'c':
			MissionTerminate(scrap);
			MissionCopy(scrap, mission);
			break;

		case 'v':
			// Use map size as a proxy to whether there's a valid scrap mission
			if (!Vec2iEqual(scrap->Size, Vec2iZero()))
			{
				InsertMission(scrap);
				fileChanged = 1;
				Setup(0);
			}
			break;

		case 'q':
			hasQuit = true;
			break;

		case 'n':
			gCampaign.MissionIndex = gCampaign.Setting.Missions.size;
			InsertMission(NULL);
			gCampaign.MissionIndex = gCampaign.Setting.Missions.size - 1;
			fileChanged = 1;
			Setup(0);
			break;
				
		case 'o':
			if (!fileChanged || ConfirmClose("Open anyway? (Y/N)"))
			{
				Open();
			}
			break;

		case 's':
			Save();
			break;

		case 'm':
			*willDisplayAutomap = 1;
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
			switch (*yc)
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
					int item = 0;
					CArrayPushBack(&mission->Items, &item);
					CArrayPushBack(&mission->ItemDensities, &item);
					*xc = mission->Items.size - 1;
				}
				break;

			default:
				if (*yc >= YC_OBJECTIVES)
				{
					AddObjective(mission);
				}
				else
				{
					InsertMission(NULL);
				}
				break;
			}
			fileChanged = 1;
			Setup(0);
			break;

		case SDLK_DELETE:
			Delete(*xc, *yc);
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
			fileChanged = UIObjectDelChar(sObjs);
			break;

		default:
			c = KeyGetTyped(&gEventHandlers.keyboard);
			if (c)
			{
				fileChanged = UIObjectAddChar(sObjs, (char)c);
			}
			break;
		}
	}
	if (gEventHandlers.HasQuit)
	{
		hasQuit = true;
	}
	if (hasQuit && (!fileChanged || ConfirmClose("Quit anyway? (Y/N)")))
	{
		*done = 1;
	}
}

static void EditCampaign(void)
{
	int done = 0;
	int xc = 0, yc = 0;
	int xcOld, ycOld;
	Mission scrap;
	memset(&scrap, 0, sizeof scrap);

	gCampaign.seed = 0;
	Setup(1);

	SDL_EnableKeyRepeat(0, 0);
	while (!done)
	{
		int willDisplayAutomap = 0;
		int c, m;
		debug(D_MAX, "Polling for input\n");
		EventPoll(&gEventHandlers, SDL_GetTicks());
		c = KeyGetPressed(&gEventHandlers.keyboard);
		m = MouseGetPressed(&gEventHandlers.mouse);

		debug(D_MAX, "Handling input\n");
		HandleInput(
			c, m,
			&xc, &yc, &xcOld, &ycOld,
			&scrap, &willDisplayAutomap, &done);
		debug(D_MAX, "Drawing UI\n");
		Display(&gGraphicsDevice, yc, willDisplayAutomap);
		if (willDisplayAutomap)
		{
			GetKey(&gEventHandlers);
		}
		debug(D_MAX, "End loop\n");
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

	EditorBrushInit(&brush);

	CampaignInit(&gCampaign);
	MissionInit(&lastMission);
	MissionInit(&currentMission);

	ConfigLoadDefault(&gConfig);
	ConfigLoad(&gConfig, GetConfigFilePath(CONFIG_FILE));
	gConfig.Graphics.IsEditor = 1;
	BulletInitialize();
	WeaponInitialize();
	PlayerDataInitialize();
	MapInit(&gMap);
	if (!PicManagerTryInit(
		&gPicManager, "graphics/cdogs.px", "graphics/cdogs2.px"))
	{
		exit(0);
	}
	memcpy(origPalette, gPicManager.palette, sizeof origPalette);
	BuildTranslationTables(gPicManager.palette);
	TextManagerInit(&gTextManager, GetDataFilePath("graphics/font.px"));
	GraphicsInit(&gGraphicsDevice);
	// Hardcode config settings
	gConfig.Graphics.ScaleMode = SCALE_MODE_NN;
	gConfig.Graphics.ScaleFactor = 2;
	GraphicsInitialize(
		&gGraphicsDevice, &gConfig.Graphics, gPicManager.palette, 0);
	if (!gGraphicsDevice.IsInitialized)
	{
		printf("Video didn't init!\n");
		exit(EXIT_FAILURE);
	}
	TextManagerGenerateOldPics(&gTextManager, &gGraphicsDevice);
	PicManagerLoadDir(&gPicManager, GetDataFilePath("graphics"));
	// initialise UI collections
	// Note: must do this after text init since positions depend on text height
	sObjs = CreateMainObjs(&gCampaign, &brush);
	memset(&sDrawObjs, 0, sizeof sDrawObjs);
	DrawBufferInit(&sDrawBuffer, Vec2iNew(X_TILES, Y_TILES), &gGraphicsDevice);

	// Reset campaign (graphics init may have created dummy campaigns)
	CampaignSettingTerminate(&gCampaign.Setting);
	CampaignSettingInit(&gCampaign.Setting);

	EventInit(&gEventHandlers, PicManagerGetPic(&gPicManager, "editor/arrow"));

	for (i = 1; i < argc; i++)
	{
		if (!loaded)
		{
			debug(D_NORMAL, "Loading map %s\n", argv[i]);
			memset(lastFile, 0, sizeof(lastFile));
			strncpy(lastFile, argv[i], sizeof(lastFile) - 1);
			if (strchr(lastFile, '.') == NULL &&
				sizeof lastFile - strlen(lastFile) > 3)
			{
				strcat(lastFile, ".CPN");
			}
			if (MapNewLoad(lastFile, &gCampaign.Setting) == 0)
			{
				loaded = 1;
			}
			debug(D_NORMAL, "Loaded map %s\n", argv[i]);
		}
	}

	debug(D_NORMAL, "Starting editor\n");
	EditCampaign();

	MapTerminate(&gMap);
	CampaignTerminate(&gCampaign);
	MissionTerminate(&lastMission);
	MissionTerminate(&currentMission);

	DrawBufferTerminate(&sDrawBuffer);
	GraphicsTerminate(&gGraphicsDevice);
	PicManagerTerminate(&gPicManager);
	TextManagerTerminate(&gTextManager);

	UIObjectDestroy(sObjs);
	CArrayTerminate(&sDrawObjs);
	EditorBrushTerminate(&brush);

	SDL_Quit();

	exit(EXIT_SUCCESS);
}
