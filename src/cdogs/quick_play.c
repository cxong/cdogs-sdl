/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
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
#include "quick_play.h"

#include <assert.h>

#include "actors.h"
#include "door.h"
#include "files.h"


// Generate a random partition of an integer `total` into a pair of ints x, y
// With the restrictions that neither x, y are less than min, and
// neither x, y are greater than max
static Vec2i GenerateRandomPairPartitionWithRestrictions(
	int total, int min, int max)
{
	Vec2i v;
	int xLow, xHigh;

	// Check for invalid input
	// Can't proceed if exactly half of total is greater than max,
	// or if total less than min
	if ((total + 1) / 2 > max || total < min)
	{
		assert(0 && "invalid random pair partition input");
		return Vec2iNew(total / 2, total - (total / 2));
	}

	// Find range of x first
	// Must be at least min, or total - max
	// Must be at most max, or total - min
	xLow = MAX(min, total - max);
	xHigh = MIN(max, total - min);
	v.x = xLow + (rand() % (xHigh - xLow + 1));
	v.y = total - v.x;
	assert(v.x >= min);
	assert(v.y >= min);
	assert(v.x <= max);
	assert(v.y <= max);
	return v;
}

static Vec2i GenerateQuickPlayMapSize(QuickPlayQuantity size)
{
	const int minMapDim = 16;
	const int maxMapDim = 64;
	// Map sizes based on total dimensions (width + height)
	// Small: 32 - 64
	// Medium: 64 - 96
	// Large: 96 - 128
	// Restrictions: at least 16, at most 64 per side
	switch (size)
	{
	case QUICKPLAY_QUANTITY_ANY:
		return GenerateRandomPairPartitionWithRestrictions(
			32 + (rand() % (128 - 32 + 1)),
			minMapDim, maxMapDim);
	case QUICKPLAY_QUANTITY_SMALL:
		return GenerateRandomPairPartitionWithRestrictions(
			32 + (rand() % (64 - 32 + 1)),
			minMapDim, maxMapDim);
	case QUICKPLAY_QUANTITY_MEDIUM:
		return GenerateRandomPairPartitionWithRestrictions(
			64 + (rand() % (96 - 64 + 1)),
			minMapDim, maxMapDim);
	case QUICKPLAY_QUANTITY_LARGE:
		return GenerateRandomPairPartitionWithRestrictions(
			96 + (rand() % (128 - 96 + 1)),
			minMapDim, maxMapDim);
	default:
		assert(0 && "invalid quick play map size config");
		return Vec2iZero();
	}
}

// Generate a quick play parameter based on the quantity setting, and various
// thresholds
// e.g. if qty is "low", generate random number between low and medium
static int GenerateQuickPlayParam(
	QuickPlayQuantity qty, int low, int medium, int high, int max)
{
	switch (qty)
	{
	case QUICKPLAY_QUANTITY_ANY:
		return low + (rand() % (max - low + 1));
	case QUICKPLAY_QUANTITY_SMALL:
		return low + (rand() % (medium - low + 1));
	case QUICKPLAY_QUANTITY_MEDIUM:
		return medium + (rand() % (high - medium + 1));
	case QUICKPLAY_QUANTITY_LARGE:
		return high + (rand() % (max - high + 1));
	default:
		assert(0);
		return 0;
	}
}

static void SetupQuickPlayEnemy(Character *enemy, const GunDescription *gun)
{
	enemy->looks.Face = rand() % FACE_COUNT;
	enemy->Gun = gun;
	enemy->speed =GenerateQuickPlayParam(
		ConfigGetEnum(&gConfig, "QuickPlay.EnemySpeed"), 64, 112, 160, 256);
	if (IsShortRange(enemy->Gun))
	{
		enemy->speed = enemy->speed * 4 / 3;
	}
	if (IsShortRange(enemy->Gun))
	{
		enemy->bot->probabilityToMove = 35 + (rand() % 35);
	}
	else
	{
		enemy->bot->probabilityToMove = 30 + (rand() % 30);
	}
	enemy->bot->probabilityToTrack = 10 + (rand() % 60);
	if (!enemy->Gun->CanShoot)
	{
		enemy->bot->probabilityToShoot = 0;
	}
	else if (IsHighDPS(enemy->Gun))
	{
		enemy->bot->probabilityToShoot = 1 + (rand() % 3);
	}
	else
	{
		enemy->bot->probabilityToShoot = 1 + (rand() % 6);
	}
	enemy->bot->actionDelay = rand() % (50 + 1);
	enemy->looks.Skin = rand() % SHADE_COUNT;
	enemy->looks.Arm = rand() % SHADE_COUNT;
	enemy->looks.Body = rand() % SHADE_COUNT;
	enemy->looks.Leg = rand() % SHADE_COUNT;
	enemy->looks.Hair = rand() % SHADE_COUNT;
	enemy->maxHealth = GenerateQuickPlayParam(
		ConfigGetEnum(&gConfig, "QuickPlay.EnemyHealth"), 10, 20, 40, 60);
	enemy->flags = 0;
}

static void SetupQuickPlayEnemies(
	Mission *mission, const int numEnemies, CharacterStore *store)
{
	int i;
	for (i = 0; i < numEnemies; i++)
	{
		const GunDescription *gun;
		CArrayPushBack(&mission->Enemies, &i);

		for (;;)
		{
			gun = CArrayGet(
				&gGunDescriptions.Guns,
				rand() % (int)gGunDescriptions.Guns.size);
			if (!gun->IsRealGun)
			{
				continue;
			}
			// make at least one of each type of enemy:
			// - Short range weapon
			// - Long range weapon
			// - High explosive weapon
			if (i == 0 && !IsShortRange(gun))
			{
				continue;
			}
			if (i == 1 && !IsLongRange(gun))
			{
				continue;
			}
			if (i == 2 &&
				ConfigGetBool(&gConfig, "QuickPlay.EnemiesWithExplosives") &&
				!IsHighDPS(gun))
			{
				continue;
			}

			if (!ConfigGetBool(&gConfig, "QuickPlay.EnemiesWithExplosives") &&
				IsHighDPS(gun))
			{
				continue;
			}
			break;
		}
		Character *ch = CharacterStoreAddOther(store);
		SetupQuickPlayEnemy(ch, gun);
		CharacterSetColors(ch);
	}
}

static color_t RandomBGColor(void);
void SetupQuickPlayCampaign(CampaignSetting *setting)
{
	Mission *m;
	CMALLOC(m, sizeof *m);
	MissionInit(m);
	m->WallStyle = rand() % WALL_STYLE_COUNT;
	m->FloorStyle = rand() % FLOOR_STYLE_COUNT;
	m->RoomStyle = rand() % FLOOR_STYLE_COUNT;
	m->ExitStyle = rand() % GetExitCount();
	m->KeyStyle = rand() % KEYSTYLE_COUNT;
	strcpy(
		m->DoorStyle, DoorStyleStr(rand() % gPicManager.doorStyleNames.size));
	m->Size = GenerateQuickPlayMapSize(
		ConfigGetEnum(&gConfig, "QuickPlay.MapSize"));
	m->Type = MAPTYPE_CLASSIC;	// TODO: generate different map types
	switch (m->Type)
	{
	case MAPTYPE_CLASSIC:
		m->u.Classic.Walls = GenerateQuickPlayParam(
			ConfigGetEnum(&gConfig, "QuickPlay.WallCount"), 0, 5, 15, 30);
		m->u.Classic.WallLength = GenerateQuickPlayParam(
			ConfigGetEnum(&gConfig, "QuickPlay.WallLength"), 1, 3, 6, 12);
		m->u.Classic.CorridorWidth = rand() % 3 + 1;
		m->u.Classic.Rooms.Count = GenerateQuickPlayParam(
			ConfigGetEnum(&gConfig, "QuickPlay.RoomCount"), 0, 2, 5, 12);
		m->u.Classic.Rooms.Min = rand() % 10 + 5;
		m->u.Classic.Rooms.Max = rand() % 10 + m->u.Classic.Rooms.Min;
		m->u.Classic.Rooms.Edge = 1;
		m->u.Classic.Rooms.Overlap = 1;
		m->u.Classic.Rooms.Walls = rand() % 5;
		m->u.Classic.Rooms.WallLength = rand() % 6 + 1;
		m->u.Classic.Rooms.WallPad = rand() % 4 + 1;
		m->u.Classic.Squares = GenerateQuickPlayParam(
			ConfigGetEnum(&gConfig, "QuickPlay.SquareCount"), 0, 1, 3, 6);
		m->u.Classic.Doors.Enabled = rand() % 2;
		m->u.Classic.Doors.Min = 1;
		m->u.Classic.Doors.Max = 6;
		m->u.Classic.Pillars.Count = rand() % 5;
		m->u.Classic.Pillars.Min = rand() % 3 + 1;
		m->u.Classic.Pillars.Max = rand() % 3 + m->u.Classic.Pillars.Min;
		break;
	default:
		assert(0 && "unknown map type");
		break;
	}
	CharacterStoreInit(&setting->characters);
	int c = GenerateQuickPlayParam(
		ConfigGetEnum(&gConfig, "QuickPlay.EnemyCount"), 3, 5, 8, 12);
	SetupQuickPlayEnemies(m, c, &setting->characters);

	c = GenerateQuickPlayParam(
		ConfigGetEnum(&gConfig, "QuickPlay.ItemCount"), 0, 2, 5, 10);
	for (int i = 0; i < c; i++)
	{
		MapObjectDensity mop;
		mop.M = IndexMapObject(rand() % MapObjectsCount(&gMapObjects));
		mop.Density = GenerateQuickPlayParam(
			ConfigGetEnum(&gConfig, "QuickPlay.ItemCount"), 0, 5, 10, 20);
		CArrayPushBack(&m->MapObjectDensities, &mop);
	}
	m->EnemyDensity = (40 + (rand() % 20)) / m->Enemies.size;
	for (int i = 0; i < (int)gGunDescriptions.Guns.size; i++)
	{
		const GunDescription *g = CArrayGet(&gGunDescriptions.Guns, i);
		if (g->IsRealGun)
		{
			CArrayPushBack(&m->Weapons, &g);
		}
	}
	m->WallMask = RandomBGColor();
	m->FloorMask = RandomBGColor();
	m->RoomMask = RandomBGColor();
	m->AltMask = RandomBGColor();

	CFREE(setting->Title);
	CSTRDUP(setting->Title, "Quick play");
	CFREE(setting->Author);
	CSTRDUP(setting->Author, "");
	CFREE(setting->Description);
	CSTRDUP(setting->Description, "");
	CArrayPushBack(&setting->Missions, m);
	CFREE(m);
}
static color_t RandomBGColor(void)
{
	color_t c;
	c.r = rand() % 128;
	c.g = rand() % 128; 
	c.b = rand() % 128;
	c.a = 255;
	return c;
}
