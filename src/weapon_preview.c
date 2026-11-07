/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2026 Cong Xu
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
#include <sys/stat.h>
#include <time.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <cdogs/SDL_JoystickButtonNames/SDL_joystickbuttonnames.h>
#include <cdogs/actor_placement.h>
#include <cdogs/actors.h>
#include <cdogs/ammo.h>
#include <cdogs/blit.h>
#include <cdogs/bullet_class.h>
#include <cdogs/camera.h>
#include <cdogs/campaigns.h>
#include <cdogs/character_class.h>
#include <cdogs/collision/collision.h>
#include <cdogs/config.h>
#include <cdogs/config_io.h>
#include <cdogs/defs.h>
#include <cdogs/draw/char_sprites.h>
#include <cdogs/draw/drawtools.h>
#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/font_utils.h>
#include <cdogs/game_events.h>
#include <cdogs/grafx.h>
#include <cdogs/handle_game_events.h>
#include <cdogs/log.h>
#include <cdogs/los.h>
#include <cdogs/map.h>
#include <cdogs/map_archive.h>
#include <cdogs/map_build.h>
#include <cdogs/map_wolf.h>
#include <cdogs/mission.h>
#include <cdogs/mouse.h>
#include <cdogs/objs.h>
#include <cdogs/particle.h>
#include <cdogs/pic.h>
#include <cdogs/pic_manager.h>
#include <cdogs/pickup.h>
#include <cdogs/player.h>
#include <cdogs/player_template.h>
#include <cdogs/sounds.h>
#include <cdogs/triggers.h>
#include <cdogs/utils.h>
#include <cdogs/vector.h>
#include <cdogs/weapon.h>
#include <cdogs/weapon_class.h>
#include <json/json.h>
#include <tinydir/tinydir.h>

#define PREVIEW_FPS 70
#define PREVIEW_MAP_W 20
#define PREVIEW_MAP_H 24

typedef struct
{
	char Name[64];
	bool IsCustom;
	const WeaponClass *Class;
} DropdownItem;

typedef struct
{
	bool IsOpen;
	int SelectedIndex;
	int ScrollOffset;
	CArray Items; // of DropdownItem
	struct vec2i Pos;
	struct vec2i Size;
	int MaxVisible;
	int ItemHeight;
} WeaponDropdown;

static char sMapFilename[CDOGS_PATH_MAX] = "";
static uint64_t sLastFolderHash = 0;
static WeaponDropdown sDropdown;
static Camera sCamera;
static int sPlayerUID = -1;
static direction_e sAimDir = DIRECTION_DOWN;
static char sStatusMessage[128] = "Drag & drop map folder or edit files";
static Uint32 sStatusMessageTimer = 0;
static bool sHasJSONError = false;

static void SetStatus(const char *msg, const bool isError)
{
	strncpy(sStatusMessage, msg, sizeof(sStatusMessage) - 1);
	sStatusMessage[sizeof(sStatusMessage) - 1] = '\0';
	sStatusMessageTimer = SDL_GetTicks() + 4000;
	sHasJSONError = isError;
}

static void ScanDirectoryHash(const char *dirPath, uint64_t *hash, int depth)
{
	if (depth > 4)
	{
		return;
	}
	tinydir_dir dir;
	if (tinydir_open(&dir, dirPath) == -1)
	{
		return;
	}
	for (; dir.has_next; tinydir_next(&dir))
	{
		tinydir_file file;
		if (tinydir_readfile(&dir, &file) == -1)
		{
			continue;
		}
		if (strcmp(file.name, ".") == 0 || strcmp(file.name, "..") == 0)
		{
			continue;
		}
		if (file.is_dir)
		{
			ScanDirectoryHash(file.path, hash, depth + 1);
		}
		else if (file.is_reg)
		{
			struct stat st;
			if (stat(file.path, &st) == 0)
			{
				for (const char *p = file.name; *p; p++)
				{
					*hash = (*hash ^ (uint8_t)*p) * 1099511628211ULL;
				}
				const uint64_t mtime = (uint64_t)st.st_mtime;
				const uint64_t size = (uint64_t)st.st_size;
				*hash = (*hash ^ mtime) * 1099511628211ULL;
				*hash = (*hash ^ size) * 1099511628211ULL;
			}
		}
	}
	tinydir_close(&dir);
}

static uint64_t GetDirectoryHash(const char *dirPath)
{
	if (dirPath == NULL || dirPath[0] == '\0')
	{
		return 0;
	}
	uint64_t hash = 14695981039346656037ULL;
	ScanDirectoryHash(dirPath, &hash, 0);
	return hash;
}

static void DropdownInit(
	WeaponDropdown *dd, const struct vec2i pos, const struct vec2i size)
{
	memset(dd, 0, sizeof(*dd));
	dd->Pos = pos;
	dd->Size = size;
	dd->ItemHeight = FontH();
	dd->MaxVisible = 24;
	dd->IsOpen = false;
	dd->SelectedIndex = 0;
	dd->ScrollOffset = 0;
	CArrayInit(&dd->Items, sizeof(DropdownItem));
}

static void DropdownTerminate(WeaponDropdown *dd)
{
	CArrayTerminate(&dd->Items);
}

static const WeaponClass *DropdownGetSelected(const WeaponDropdown *dd)
{
	if (dd->SelectedIndex >= 0 && dd->SelectedIndex < (int)dd->Items.size)
	{
		const DropdownItem *item = CArrayGet(&dd->Items, dd->SelectedIndex);
		return item->Class;
	}
	return NULL;
}

static void DropdownRebuild(WeaponDropdown *dd, const char *preferredName)
{
	CArrayClear(&dd->Items);

	// Add custom weapons first
	CA_FOREACH(const WeaponClass, wc, gWeaponClasses.CustomGuns)
	if (wc->IsRealGun && wc->name)
	{
		DropdownItem item;
		memset(&item, 0, sizeof(item));
		strncpy(item.Name, wc->name, sizeof(item.Name) - 1);
		item.IsCustom = true;
		item.Class = wc;
		CArrayPushBack(&dd->Items, &item);
	}
	CA_FOREACH_END()

	// Then add built-in weapons
	CA_FOREACH(const WeaponClass, wc, gWeaponClasses.Guns)
	if (wc->IsRealGun && wc->name)
	{
		DropdownItem item;
		memset(&item, 0, sizeof(item));
		strncpy(item.Name, wc->name, sizeof(item.Name) - 1);
		item.IsCustom = false;
		item.Class = wc;
		CArrayPushBack(&dd->Items, &item);
	}
	CA_FOREACH_END()

	// Restore previous selection if possible
	dd->SelectedIndex = 0;
	if (preferredName && preferredName[0] != '\0')
	{
		for (int i = 0; i < (int)dd->Items.size; i++)
		{
			const DropdownItem *item = CArrayGet(&dd->Items, i);
			if (strcmp(item->Name, preferredName) == 0)
			{
				dd->SelectedIndex = i;
				break;
			}
		}
	}
}

static bool DropdownHandleInput(
	WeaponDropdown *dd, const Mouse *mouse, bool *weaponChanged)
{
	*weaponChanged = false;
	const struct vec2i mpos = mouse->currentPos;

	if (MouseIsPressed(mouse, SDL_BUTTON_LEFT))
	{
		const Rect2i headerRect = Rect2iNew(dd->Pos, dd->Size);
		if (Rect2iIsInside(headerRect, mpos))
		{
			dd->IsOpen = !dd->IsOpen;
			if (dd->IsOpen)
			{
				if (dd->SelectedIndex >= dd->ScrollOffset + dd->MaxVisible)
				{
					dd->ScrollOffset = dd->SelectedIndex - dd->MaxVisible + 1;
				}
				else if (dd->SelectedIndex < dd->ScrollOffset)
				{
					dd->ScrollOffset = dd->SelectedIndex;
				}
			}
			return true;
		}

		if (dd->IsOpen)
		{
			const int visibleCount = MIN(dd->MaxVisible, (int)dd->Items.size);
			const Rect2i listRect = Rect2iNew(
				svec2i(dd->Pos.x, dd->Pos.y + dd->Size.y),
				svec2i(dd->Size.x, visibleCount * dd->ItemHeight));

			if (Rect2iIsInside(listRect, mpos))
			{
				const int relY = mpos.y - (dd->Pos.y + dd->Size.y);
				const int clicked = dd->ScrollOffset + relY / dd->ItemHeight;
				if (clicked >= 0 && clicked < (int)dd->Items.size)
				{
					if (dd->SelectedIndex != clicked)
					{
						dd->SelectedIndex = clicked;
						*weaponChanged = true;
					}
					dd->IsOpen = false;
				}
				return true;
			}
			else
			{
				dd->IsOpen = false;
				return false;
			}
		}
	}

	if (dd->IsOpen && mouse->wheel.y != 0)
	{
		dd->ScrollOffset -= mouse->wheel.y;
		const int maxScroll = MAX(0, (int)dd->Items.size - dd->MaxVisible);
		dd->ScrollOffset = CLAMP(dd->ScrollOffset, 0, maxScroll);
		return true;
	}

	return false;
}

static void DrawBox(
	GraphicsDevice *g, const struct vec2i pos, const struct vec2i size,
	const color_t fill, const color_t border)
{
	DrawRectangle(g, pos, size, fill, true);
	DrawRectangle(g, pos, size, border, false);
}

static void EnsurePlayerAmmo(TActor *a)
{
	for (int i = 0; i < AmmoGetNumClasses(&gAmmo); i++)
	{
		if (i < (int)a->ammo.size)
		{
			*(int *)CArrayGet(&a->ammo, i) = 9999;
		}
		else
		{
			int ammoCount = 9999;
			CArrayPushBack(&a->ammo, &ammoCount);
		}
	}
}

static void EquipActiveWeapon(void)
{
	const WeaponClass *wc = DropdownGetSelected(&sDropdown);
	if (wc == NULL)
	{
		return;
	}
	if (gPlayerDatas.size > 0)
	{
		PlayerData *p = CArrayGet(&gPlayerDatas, 0);
		p->guns[0] = wc;
		TActor *a = ActorGetByUID(p->ActorUID);
		if (a)
		{
			a->guns[0] = WeaponCreate(wc);
			a->gunIndex = 0;
			EnsurePlayerAmmo(a);
		}
	}
}

static void ReloadAllData(const char *filename)
{
	const WeaponClass *currentWC = DropdownGetSelected(&sDropdown);
	char currentWeaponName[64] = "";
	if (currentWC && currentWC->name)
	{
		strncpy(
			currentWeaponName, currentWC->name, sizeof(currentWeaponName) - 1);
	}

	GameEventsClear(&gGameEvents);

	// Terminate classes
	MissionOptionsTerminate(&gMission);
	MapObjectsTerminate(&gMapObjects);
	PickupClassesTerminate(&gPickupClasses);
	ParticleClassesTerminate(&gParticleClasses);
	AmmoTerminate(&gAmmo);
	WeaponClassesTerminate(&gWeaponClasses);
	BulletTerminate(&gBulletClasses);
	CharacterClassesTerminate(&gCharacterClasses);
	PicManagerClearCustom(&gPicManager);
	CharSpriteClassesClear(gCharSpriteClasses.customClasses);

	// Re-initialize base data
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

	// Load custom data if filename provided
	if (filename && strlen(filename))
	{
		int err = MapNewLoad(filename, &gCampaign.Setting);
		if (err)
		{
			printf("Failed to load map %s!\n", filename);
		}
		CampaignAndMissionSetup(&gCampaign, &gMission);
		GameEventsInit(&gGameEvents);
		Mission *m = CampaignGetCurrentMission(&gCampaign);
		MapBuild(&gMap, m, false, 0, GAME_MODE_NORMAL, NULL);
		LOSSetAllVisible(&gMap.LOS);
	}

	MapMarkAllAsVisited(&gMap);
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
	if (!p->IsLocal)
		continue;
	GameEvent e = GameEventNew(GAME_EVENT_PLAYER_DATA);
	e.u.PlayerData = PlayerDataMissionReset(p);
	GameEventsEnqueue(&gGameEvents, e);
	CA_FOREACH_END()
	HandleGameEvents(&gGameEvents, NULL, NULL, NULL, NULL);
	const struct vec2 centerPos =
		Vec2CenterOfTile(svec2i(gMap.Size.x / 2, gMap.Size.y / 2));
	gMap.start = svec2i(gMap.Size.x / 2, gMap.Size.y / 2);
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
	if (!p->Ready)
		continue;
	PlacePlayer(&gMap, p, centerPos, true);
	CA_FOREACH_END()
	CameraReset(&sCamera);
	sCamera.lastPosition = centerPos;
	sCamera.FollowNextPlayer = true;

	DropdownRebuild(&sDropdown, currentWeaponName);
	EquipActiveWeapon();
}

static void SetupEmptyGame(void)
{
	Mission m;
	MissionInit(&m);
	m.Size = svec2i(PREVIEW_MAP_W, PREVIEW_MAP_H);
	m.Type = MAPTYPE_CLASSIC;
	MissionTileClassesInitDefault(&m.u.Classic.TileClasses);
	m.u.Classic.Walls = 0;
	m.u.Classic.WallLength = 0;
	m.u.Classic.CorridorWidth = 1;
	m.u.Classic.Rooms.Count = 0;
	m.u.Classic.Squares = 0;
	m.u.Classic.Doors.Enabled = false;
	m.u.Classic.Pillars.Count = 0;
	m.u.Classic.ExitEnabled = false;
	CArrayPushBack(&gCampaign.Setting.Missions, &m);

	CampaignAndMissionSetup(&gCampaign, &gMission);
	MapBuild(&gMap, &m, false, 0, GAME_MODE_NORMAL, NULL);
	LOSSetAllVisible(&gMap.LOS);
	MapMarkAllAsVisited(&gMap);

	PlayerDataTerminate(&gPlayerDatas);
	PlayerDataInit(&gPlayerDatas);

	GameEvent e = GameEventNew(GAME_EVENT_PLAYER_DATA);
	e.u.PlayerData = PlayerDataDefault(0);
	e.u.PlayerData.UID = 0;
	const WeaponClass *wc = DropdownGetSelected(&sDropdown);
	if (wc)
	{
		// e.u.PlayerData.guns[0] = wc;
	}
	GameEventsEnqueue(&gGameEvents, e);
	HandleGameEvents(&gGameEvents, NULL, NULL, NULL, NULL);

	PlayerData *p = CArrayGet(&gPlayerDatas, 0);
	const struct vec2 centerPos =
		Vec2CenterOfTile(svec2i(gMap.Size.x / 2, gMap.Size.y / 2));
	gMap.start = svec2i(gMap.Size.x / 2, gMap.Size.y / 2);
	PlacePlayer(&gMap, p, centerPos, true);

	sPlayerUID = p->UID;
	TActor *a = ActorGetByUID(p->ActorUID);
	if (a)
	{
		a->flags |= FLAGS_INVULNERABLE | FLAGS_ASBESTOS | FLAGS_IMMUNITY;
		a->health = 10000;
		if (wc)
		{
			a->guns[0] = WeaponCreate(wc);
			a->gunIndex = 0;
			EnsurePlayerAmmo(a);
		}
	}

	CameraInit(&sCamera);
	sCamera.lastPosition = centerPos;
	sCamera.FollowNextPlayer = true;
}

static void DrawDropdown(WeaponDropdown *dd, GraphicsDevice *g)
{
	const color_t headerBg = {22, 28, 40, 235};
	const color_t headerBorder =
		dd->IsOpen ? colorCyan : (color_t){90, 130, 190, 255};

	// Main selection bar
	DrawBox(g, dd->Pos, dd->Size, headerBg, headerBorder);

	const WeaponClass *selectedWC = DropdownGetSelected(dd);
	if (selectedWC)
	{
		const bool isCustom =
			dd->SelectedIndex < (int)dd->Items.size &&
			((const DropdownItem *)CArrayGet(&dd->Items, dd->SelectedIndex))
				->IsCustom;

		int textX = dd->Pos.x + 4;
		const int textY = dd->Pos.y + 3;
		if (isCustom)
		{
			FontStrMask("[C] ", svec2i(textX, textY), colorCyan);
			textX += 22;
		}
		FontStrMask(selectedWC->name, svec2i(textX, textY), colorWhite);
	}
	else
	{
		FontStrMask(
			"(No weapon)", svec2i(dd->Pos.x + 4, dd->Pos.y + 3), colorGray);
	}

	FontStrMask(
		dd->IsOpen ? "^" : "v",
		svec2i(
			dd->Pos.x + dd->Size.y ? dd->Pos.x + dd->Size.x - 12 : dd->Pos.x,
			dd->Pos.y + 3),
		colorYellow);

	// Expanded list
	if (dd->IsOpen)
	{
		const int visibleCount = MIN(dd->MaxVisible, (int)dd->Items.size);
		const struct vec2i listPos = svec2i(dd->Pos.x, dd->Pos.y + dd->Size.y);
		const struct vec2i listSize =
			svec2i(dd->Size.x, visibleCount * dd->ItemHeight + 6);
		const color_t listBg = {16, 20, 30, 250};
		const color_t listBorder = {90, 130, 190, 255};

		DrawBox(g, listPos, listSize, listBg, listBorder);

		const struct vec2i mpos = gEventHandlers.mouse.currentPos;

		for (int i = 0; i < visibleCount; i++)
		{
			const int itemIdx = dd->ScrollOffset + i;
			if (itemIdx >= (int)dd->Items.size)
			{
				break;
			}
			const DropdownItem *item = CArrayGet(&dd->Items, itemIdx);
			const struct vec2i itemPos =
				svec2i(listPos.x + 1, listPos.y + 1 + i * dd->ItemHeight);
			const struct vec2i itemSize =
				svec2i(listSize.x - 2, dd->ItemHeight);

			const Rect2i itemRect = Rect2iNew(itemPos, itemSize);
			const bool isHovered = Rect2iIsInside(itemRect, mpos);
			const bool isSelected = (itemIdx == dd->SelectedIndex);

			if (isHovered)
			{
				DrawBox(
					g, itemPos, itemSize, (color_t){45, 70, 115, 255},
					colorTransparent);
			}

			int ix = itemPos.x + 3;
			const int iy = itemPos.y + 2;

			if (isSelected)
			{
				FontStrMask(">", svec2i(ix, iy), colorYellow);
			}
			ix += 8;

			if (item->IsCustom)
			{
				FontStrMask("[C] ", svec2i(ix, iy), colorCyan);
				ix += 22;
			}

			const color_t textColor =
				isSelected ? colorYellow
						   : (isHovered ? colorWhite : colorLightBlue);
			FontStrMask(item->Name, svec2i(ix, iy), textColor);
		}

		// Scrollbar indicator
		if ((int)dd->Items.size > dd->MaxVisible)
		{
			const int trackHeight = listSize.y - 4;
			const int thumbHeight =
				MAX(6, trackHeight * dd->MaxVisible / (int)dd->Items.size);
			const int maxScroll = (int)dd->Items.size - dd->MaxVisible;
			const int thumbY =
				listPos.y + 2 +
				(trackHeight - thumbHeight) * dd->ScrollOffset / maxScroll;
			DrawBox(
				g, svec2i(listPos.x + listSize.x - 4, thumbY),
				svec2i(3, thumbHeight), colorCyan, colorCyan);
		}
	}
}

static void DrawHUDOverlay(GraphicsDevice *g)
{
	const int resX = g->cachedConfig.Res.x;
	const int resY = g->cachedConfig.Res.y;

	// 1. Top status banner
	const struct vec2i bannerPos = svec2i(resX - 195, 8);
	const struct vec2i bannerSize = svec2i(187, 44);
	DrawBox(
		g, bannerPos, bannerSize, (color_t){18, 24, 36, 220},
		(color_t){60, 85, 120, 255});

	char folderText[64];
	if (sMapFilename[0] != '\0')
	{
		snprintf(
			folderText, sizeof(folderText), "Map: %s",
			PathGetBasename(sMapFilename));
	}
	else
	{
		strncpy(folderText, "Map: (Built-in data)", sizeof(folderText) - 1);
	}
	FontStrMask(
		folderText, svec2i(bannerPos.x + 4, bannerPos.y + 4), colorCyan);

	const Uint32 now = SDL_GetTicks();
	const color_t statusColor =
		sHasJSONError ? colorRed
					  : (now < sStatusMessageTimer ? colorGreen : colorGray);
	FontStrMask(
		sStatusMessage, svec2i(bannerPos.x + 4, bannerPos.y + 16),
		statusColor);

	char watchText[64];
	snprintf(
		watchText, sizeof(watchText), "Watching: %s",
		sMapFilename[0] != '\0' ? "Live edit enabled" : "Drag folder here");
	FontStrMask(
		watchText, svec2i(bannerPos.x + 4, bannerPos.y + 28), colorYellow);

	// 2. Weapon stats panel (top-right below banner)
	const WeaponClass *wc = DropdownGetSelected(&sDropdown);
	if (wc)
	{
		const struct vec2i statsPos = svec2i(resX - 195, 56);
		const struct vec2i statsSize = svec2i(187, 48);
		DrawBox(
			g, statsPos, statsSize, (color_t){18, 24, 36, 210},
			(color_t){60, 85, 120, 255});

		char line1[64];
		snprintf(
			line1, sizeof(line1), "Type: %s  Lock: %d", GunTypeStr(wc->Type),
			wc->Lock);
		FontStrMask(line1, svec2i(statsPos.x + 4, statsPos.y + 4), colorWhite);

		const BulletClass *bc = WeaponClassGetBullet(wc, 0);
		char line2[64];
		if (bc)
		{
			snprintf(
				line2, sizeof(line2),
				"Speed: %.1f-%.1f  Power: %d  Mass: %.1f", bc->SpeedLow,
				bc->SpeedHigh, bc->Power, bc->Mass);
		}
		else
		{
			snprintf(
				line2, sizeof(line2), "Barrels: %d",
				WeaponClassNumBarrels(wc));
		}
		FontStrMask(
			line2, svec2i(statsPos.x + 4, statsPos.y + 18), colorLightBlue);

		char line3[64];
		snprintf(
			line3, sizeof(line3), "Shake: %d  Overheat: %d",
			wc->u.Normal.Shake.Amount, wc->OverheatTicks);
		FontStrMask(line3, svec2i(statsPos.x + 4, statsPos.y + 32), colorGray);
	}

	// 3. Bottom instructions bar
	const struct vec2i helpPos = svec2i(8, resY - 18);
	const struct vec2i helpSize = svec2i(resX - 16, 14);
	DrawBox(
		g, helpPos, helpSize, (color_t){12, 16, 24, 210},
		(color_t){50, 65, 90, 255});
	FontStrMask(
		"Drag map folder | Aim: Mouse | [ / ]: Change gun | R: "
		"Reload | ESC: Quit",
		svec2i(helpPos.x + 4, helpPos.y + 2), colorWhite);

	// 4. Dropdown rendered last so it sits on top of all HUD panels
	DrawDropdown(&sDropdown, g);
}

int main(int argc, char *argv[])
{
#if defined(_MSC_VER) && !defined(NDEBUG)
	FreeConsole();
#endif

	srand((unsigned int)time(NULL));
	LogInit();
	printf("C-Dogs SDL Weapon Preview\n");

	sMapFilename[0] = '\0';

	SetupConfigDir();
	gConfig = ConfigLoad(GetConfigFilePath(CONFIG_FILE));

	ConfigGet(&gConfig, "Graphics.WindowWidth")->u.Int.Value = 800;
	ConfigGet(&gConfig, "Graphics.WindowHeight")->u.Int.Value = 600;
	ConfigGet(&gConfig, "Graphics.ScaleFactor")->u.Int.Value = 2;
	ConfigGet(&gConfig, "Graphics.ScaleMode")->u.Enum.Value = SCALE_MODE_NN;
	ConfigGet(&gConfig, "Graphics.Fullscreen")->u.Bool.Value = false;
	ConfigGet(&gConfig, "Graphics.SecondWindow")->u.Bool.Value = false;
	ConfigGet(&gConfig, "Graphics.ShowHUD")->u.Bool.Value = true;
	ConfigGet(&gConfig, "Game.FireMoveStyle")->u.Enum.Value = FIREMOVE_STOP;
	ConfigSetChanged(&gConfig);

	const int sdlFlags = SDL_INIT_TIMER | SDL_INIT_VIDEO;
	if (SDL_Init(sdlFlags) != 0)
	{
		fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}
	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

	PicManagerInit(&gPicManager);
	GraphicsInit(&gGraphicsDevice, &gConfig);
	GraphicsInitialize(&gGraphicsDevice);
	if (!gGraphicsDevice.IsInitialized)
	{
		fprintf(stderr, "Video initialization failed!\n");
		return EXIT_FAILURE;
	}
	FontLoadFromJSON(&gFont, "graphics/font.png", "graphics/font.json");
	PicManagerLoad(&gPicManager);

	EventInit(&gEventHandlers);
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
	gCampaign.Setting.Ammo = false; // Infinite ammo for preview
	PlayerDataInit(&gPlayerDatas);
	GameEventsInit(&gGameEvents);

	DropdownInit(&sDropdown, svec2i(8, 8), svec2i(190, 16));

	// Optional initial map from command line
	if (argc > 1)
	{
		RealPath(argv[1], sMapFilename);
	}

	ReloadAllData(sMapFilename);
	SetupEmptyGame();
	sLastFolderHash = GetDirectoryHash(sMapFilename);

	if (sMapFilename[0] != '\0')
	{
		char msg[128];
		snprintf(msg, sizeof(msg), "Loaded map: %s", sMapFilename);
		SetStatus(msg, false);
	}

	bool quit = false;
	Uint32 lastWatchTick = SDL_GetTicks();
	Uint32 lastFrameTick = SDL_GetTicks();
	const Uint32 frameDuration = 1000 / PREVIEW_FPS;

	while (!quit)
	{
		const Uint32 frameStart = SDL_GetTicks();
		const Uint32 ticksElapsed = frameStart - lastFrameTick;
		lastFrameTick = frameStart;

		EventPoll(&gEventHandlers, ticksElapsed, NULL);
		if (gEventHandlers.HasQuit)
		{
			quit = true;
			break;
		}

		// Handle drag and drop of folder
		if (gEventHandlers.DropFile != NULL)
		{
			const char *dropped = gEventHandlers.DropFile;
			strncpy(sMapFilename, dropped, sizeof(sMapFilename) - 1);
			sMapFilename[sizeof(sMapFilename) - 1] = '\0';
			ReloadAllData(sMapFilename);
			sLastFolderHash = GetDirectoryHash(sMapFilename);
			char msg[128];
			snprintf(msg, sizeof(msg), "Loaded: %s", sMapFilename);
			SetStatus(msg, false);
		}

		// Watch for live file edits in folder (~150ms interval)
		if (sMapFilename[0] != '\0' && frameStart - lastWatchTick >= 150)
		{
			lastWatchTick = frameStart;
			const uint64_t currentHash = GetDirectoryHash(sMapFilename);
			if (currentHash != sLastFolderHash)
			{
				sLastFolderHash = currentHash;
				ReloadAllData(sMapFilename);
				time_t now = time(NULL);
				struct tm *tm = localtime(&now);
				char msg[128];
				snprintf(
					msg, sizeof(msg), "Reloaded: %02d:%02d:%02d", tm->tm_hour,
					tm->tm_min, tm->tm_sec);
				SetStatus(msg, false);
			}
		}

		// Keyboard controls
		if (KeyIsPressed(&gEventHandlers.keyboard, SDL_SCANCODE_ESCAPE))
		{
			quit = true;
			break;
		}
		if (KeyIsPressed(&gEventHandlers.keyboard, SDL_SCANCODE_R))
		{
			ReloadAllData(sMapFilename);
			SetStatus("Force reloaded all data files", false);
		}
		if (KeyIsPressed(&gEventHandlers.keyboard, SDL_SCANCODE_LEFTBRACKET))
		{
			sDropdown.SelectedIndex--;
			if (sDropdown.SelectedIndex < 0)
			{
				sDropdown.SelectedIndex = (int)sDropdown.Items.size - 1;
			}
			EquipActiveWeapon();
		}
		if (KeyIsPressed(&gEventHandlers.keyboard, SDL_SCANCODE_RIGHTBRACKET))
		{
			sDropdown.SelectedIndex++;
			if (sDropdown.SelectedIndex >= (int)sDropdown.Items.size)
			{
				sDropdown.SelectedIndex = 0;
			}
			EquipActiveWeapon();
		}

		// Dropdown UI input
		bool weaponChanged = false;
		DropdownHandleInput(&sDropdown, &gEventHandlers.mouse, &weaponChanged);
		if (weaponChanged)
		{
			EquipActiveWeapon();
		}

		// Player aiming and continuous firing
		if (gPlayerDatas.size > 0)
		{
			PlayerData *p = CArrayGet(&gPlayerDatas, 0);
			TActor *a = ActorGetByUID(p->ActorUID);
			if (a)
			{
				const struct vec2 centerPos =
					Vec2CenterOfTile(svec2i(gMap.Size.x / 2, gMap.Size.y / 2));
				a->thing.Pos = centerPos;
				a->thing.Vel = svec2_zero();
				a->health = 10000;
				a->dead = false;

				// Aim towards mouse cursor unless hovering inside the open
				// dropdown
				bool mouseInDropdown = false;
				if (sDropdown.IsOpen)
				{
					const int vis =
						MIN(sDropdown.MaxVisible, (int)sDropdown.Items.size);
					const Rect2i ddRect = Rect2iNew(
						sDropdown.Pos,
						svec2i(
							sDropdown.Size.x,
							sDropdown.Size.y + vis * sDropdown.ItemHeight));
					mouseInDropdown = Rect2iIsInside(
						ddRect, gEventHandlers.mouse.currentPos);
				}

				if (!mouseInDropdown)
				{
					const struct vec2i mpos = gEventHandlers.mouse.currentPos;
					const struct vec2 screenCenter = svec2(
						gGraphicsDevice.cachedConfig.Res.x * 0.5f,
						gGraphicsDevice.cachedConfig.Res.y * 0.5f);
					const struct vec2 delta = svec2(
						mpos.x - screenCenter.x, mpos.y - screenCenter.y);
					if (svec2_length_squared(delta) > 16.0f)
					{
						sAimDir =
							RadiansToDirection(svec2_angle(delta) + MPI_2);
					}
				}

				EnsurePlayerAmmo(a);

				// Fire continuously in the aimed direction
				const int cmd = CMD_BUTTON1 | dir2cmd[sAimDir];
				CommandActor(a, cmd, 1);
			}
		}

		// Update game world
		UpdateAllActors(1);
		UpdateObjects(1);
		UpdateMobileObjects(1);
		PickupsUpdate(&gPickups, 1);
		ParticlesUpdate(&gParticles, 1);
		MapUpdate(&gMap);
		CameraUpdate(&sCamera, 1, 1000 / PREVIEW_FPS);
		HandleGameEvents(&gGameEvents, &sCamera, NULL, NULL, NULL);

		// Render frame
		WindowContextPreRender(&gGraphicsDevice.gameWindow);

		// Background game render
		BlitClearBuf(&gGraphicsDevice);
		CameraDraw(&sCamera, sCamera.HUD.DrawData);
		BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.screen);

		// Foreground HUD render
		DrawHUDOverlay(&gGraphicsDevice);

		WindowContextPostRender(&gGraphicsDevice.gameWindow);

		// Frame limiter
		const Uint32 frameElapsed = SDL_GetTicks() - frameStart;
		if (frameElapsed < frameDuration)
		{
			SDL_Delay(frameDuration - frameElapsed);
		}
	}

	// Clean shutdown
	DropdownTerminate(&sDropdown);
	MissionOptionsTerminate(&gMission);
	MapTerminate(&gMap);
	MapObjectsTerminate(&gMapObjects);
	PickupClassesTerminate(&gPickupClasses);
	ParticleClassesTerminate(&gParticleClasses);
	AmmoTerminate(&gAmmo);
	WeaponClassesTerminate(&gWeaponClasses);
	BulletTerminate(&gBulletClasses);
	CharacterClassesTerminate(&gCharacterClasses);
	PlayerDataTerminate(&gPlayerDatas);
	CampaignTerminate(&gCampaign);
	CollisionSystemTerminate(&gCollisionSystem);
	GameEventsTerminate(&gGameEvents);
	CameraTerminate(&sCamera);
	CharSpriteClassesTerminate(&gCharSpriteClasses);
	PicManagerTerminate(&gPicManager);
	FontTerminate(&gFont);
	GraphicsTerminate(&gGraphicsDevice);
	EventTerminate(&gEventHandlers);
	ConfigDestroy(&gConfig);
	LogTerminate();
	SDL_Quit();

	return EXIT_SUCCESS;
}
