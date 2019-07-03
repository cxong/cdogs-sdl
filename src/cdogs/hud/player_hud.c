/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2017, 2019 Cong Xu
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
#include "player_hud.h"

#include "actors.h"
#include "automap.h"
#include "draw/draw_actor.h"
#include "hud_defs.h"

#define HEALTH_COUNTER_SHOW_MS 5000
#define AMMO_COUNTER_SHOW_MS 4000
#define GRENADE_COUNTER_SHOW_MS 4000
#define WEAPON_GAUGE_EXTRA_HEIGHT 2
#define AMMO_GAUGE_HEIGHT 4


void HUDPlayerInit(HUDPlayer *h)
{
	memset(h, 0, sizeof *h);
	h->healthCounter = HEALTH_COUNTER_SHOW_MS;
	HealthGaugeInit(&h->healthGauge);
	h->ammoCounter = AMMO_COUNTER_SHOW_MS;
	h->grenadeCounter = GRENADE_COUNTER_SHOW_MS;
}

void HUDPlayerUpdate(HUDPlayer *h, const PlayerData *p, const int ms)
{
	h->healthCounter = MAX(h->healthCounter - ms, 0);
	h->ammoCounter = MAX(h->ammoCounter - ms, 0);
	h->grenadeCounter = MAX(h->grenadeCounter - ms, 0);

	const TActor *a = ActorGetByUID(p->ActorUID);
	if (a == NULL) return;

	// Health
	const bool healthUpdating = h->healthGauge.waitMs > 0;
	HealthGaugeUpdate(&h->healthGauge, a, ms);
	if ((!healthUpdating || h->healthCounter == 0) && h->healthGauge.waitMs > 0)
	{
		h->healthCounter = HEALTH_COUNTER_SHOW_MS;
	}

	// Ammo / gauge
	const Weapon *w = ACTOR_GET_GUN(a);
	const int ammo = ActorWeaponGetAmmo(a, w->Gun);
	if (p->UID != h->lastPlayerUID || a->gunIndex != h->lastGunIndex ||
		ammo != h->lastAmmo)
	{
		h->ammoCounter = AMMO_COUNTER_SHOW_MS;
	}

	// Grenade name
	const Weapon *grenade = ACTOR_GET_GRENADE(a);
	if (p->UID != h->lastPlayerUID || a->grenadeIndex != h->lastGrenadeIndex ||
		grenade->Gun != h->lastGrenade)
	{
		h->grenadeCounter = GRENADE_COUNTER_SHOW_MS;
	}

	h->lastPlayerUID = p->UID;
	h->lastScore = p->Stats.Score;
	h->lastHealth = a->health;
	h->lastAmmo = ammo;
	h->lastGunIndex = a->gunIndex;
	h->lastGrenadeIndex = a->grenadeIndex;
	h->lastGrenade = grenade->Gun;
}

static void DrawPlayerStatus(
	HUD *hud, const PlayerData *data, TActor *p,
	const int flags, const HUDPlayer *h, const Rect2i r);
static void DrawPlayerObjectiveCompass(
	const HUD *hud, TActor *a, const int hudPlayerIndex, const int numViews);
void DrawPlayerHUD(
	HUD *hud, const PlayerData *p, const int drawFlags,
	const int hudPlayerIndex, const Rect2i r, const int numViews)
{
	TActor *a = IsPlayerAlive(p) ? ActorGetByUID(p->ActorUID) : NULL;
	DrawPlayerStatus(hud, p, a, drawFlags, &hud->hudPlayers[hudPlayerIndex], r);
	HUDNumPopupsDrawPlayer(&hud->numPopups, hudPlayerIndex, drawFlags, r);
	DrawPlayerObjectiveCompass(hud, a, hudPlayerIndex, numViews);
}

static void DrawPlayerIcon(
	TActor *a, GraphicsDevice *g, const PicManager *pm, const int flags,
	const SDL_RendererFlip flip);
static void DrawScore(
	GraphicsDevice *g, const PicManager *pm, const TActor *a, const int score,
	const int flags, const Rect2i r);
static void DrawLives(
	const GraphicsDevice *device, const PlayerData *player,
	const struct vec2i pos,
	const FontAlign hAlign, const FontAlign vAlign);
static void DrawWeaponStatus(
	HUD *hud, const HUDPlayer *h, const TActor *actor, struct vec2i pos,
	const FontAlign hAlign, const FontAlign vAlign);
static void DrawGrenadeStatus(
	HUD *hud, const HUDPlayer *h, const TActor *a, struct vec2i pos,
	const FontAlign hAlign, const FontAlign vAlign);
static void DrawRadar(
	GraphicsDevice *device, const TActor *p,
	const int flags, const bool showExit);
// Draw player's score, health etc.
static void DrawPlayerStatus(
	HUD *hud, const PlayerData *data, TActor *p,
	const int flags, const HUDPlayer *h, const Rect2i r)
{
	SDL_RendererFlip flip = SDL_FLIP_NONE;
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		flip |= SDL_FLIP_HORIZONTAL;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		flip |= SDL_FLIP_VERTICAL;
	}

	DrawPlayerIcon(p, hud->device, &gPicManager, flags, flip);

	struct vec2i pos;

	// Draw back bar, stretched across the screen
	const Pic *backBar = PicManagerGetPic(&gPicManager, "hud/back_bar");
	const int barWidth = r.Size.x - 22;
	struct vec2i barPos = svec2i(22 + barWidth / 2, 0);
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		barPos.x = r.Pos.x + barWidth / 2;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		barPos.y = hud->device->cachedConfig.Res.y - backBar->size.y;
	}
	PicRender(
		backBar, hud->device->gameWindow.renderer, barPos, colorWhite,
		0, svec2(barWidth, 1), flip);

	// Name
	pos = svec2i(23, 2);

	FontOpts opts = FontOptsNew();
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		opts.HAlign = ALIGN_END;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		opts.VAlign = ALIGN_END;
		pos.y += BOTTOM_PADDING;
	}
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	opts.Pad = pos;
	FontStrOpt(data->name, svec2i_zero(), opts);

	const int rowHeight = 1 + FontH();
	pos.x = 5;
	pos.y += rowHeight;

	DrawScore(hud->device, &gPicManager, p, data->Stats.Score, flags, r);

	opts.Pad = pos;
	pos.y += rowHeight;
	if (p)
	{
		// Health
		const bool isLowHealth = ActorIsLowHealth(p);
		if (h->healthCounter > 0 || isLowHealth)
		{
			if (!isLowHealth)
			{
				opts.Mask.a = (uint8_t)CLAMP(
					h->healthCounter * 255 * 2 / HEALTH_COUNTER_SHOW_MS, 0, 255);
			}
			HealthGaugeDraw(&h->healthGauge, hud->device, p, pos, opts);
			opts.Mask.a = 255;
		}
		pos.y += rowHeight;

		// Lives
		DrawLives(hud->device, data, pos, opts.HAlign, opts.VAlign);
		pos.y += rowHeight + LIVES_ROW_EXTRA_Y;

		// Weapon
		DrawWeaponStatus(hud, h, p, pos, opts.HAlign, opts.VAlign);
		pos.y += rowHeight + GRENADES_ROW_EXTRA_Y;

		// Grenades
		DrawGrenadeStatus(hud, h, p, pos, opts.HAlign, opts.VAlign);
	}

	if (ConfigGetBool(&gConfig, "Interface.ShowHUDMap") &&
		!(flags & HUDFLAGS_SHARE_SCREEN) &&
		IsAutoMapEnabled(gCampaign.Entry.Mode))
	{
		DrawRadar(hud->device, p, flags, hud->showExit);
	}
}
static void DrawPlayerIcon(
	TActor *a, GraphicsDevice *g, const PicManager *pm, const int flags,
	const SDL_RendererFlip flip)
{
	const Pic *framePic = PicManagerGetPic(pm, "hud/player_frame");
	const Pic *underlayPic = PicManagerGetPic(pm, "hud/player_frame");
	struct vec2i picPos = svec2i_zero();
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		picPos.x = g->cachedConfig.Res.x - framePic->size.x;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		picPos.y = g->cachedConfig.Res.y - framePic->size.y;
	}

	PicRender(
		underlayPic, g->gameWindow.renderer, picPos, colorWhite, 0,
		svec2_one(), flip);
	if (a)
	{
		ActorPics pics = GetCharacterPicsFromActor(a);
		struct vec2i pos =
			svec2i(framePic->size.x / 2 - 2, framePic->size.y / 2);
		if (flags & HUDFLAGS_PLACE_RIGHT)
		{
			pos.x = g->cachedConfig.Res.x - pos.x;
		}
		if (flags & HUDFLAGS_PLACE_BOTTOM)
		{
			pos.y =  g->cachedConfig.Res.y - pos.y;
		}
		pos.y += 10;
		DrawActorPics(&pics, pos, false);
	}
	PicRender(
		framePic, g->gameWindow.renderer, picPos, colorWhite, 0,
		svec2_one(), flip);
}
static void DrawScore(
	GraphicsDevice *g, const PicManager *pm, const TActor *a, const int score,
	const int flags, const Rect2i r)
{
	const Pic *backPic = PicManagerGetPic(pm, "hud/score_back");

	// Score aligned to the right
	struct vec2i backPos = svec2i(r.Size.x - backPic->size.x, 1);
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		backPos.x = r.Pos.x;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		backPos.y = g->cachedConfig.Res.y - backPic->size.y - 1;
	}

	PicRender(
		backPic, g->gameWindow.renderer, backPos, colorWhite, 0, svec2_one(),
		SDL_FLIP_NONE);

	if (a == NULL)
	{
		return;
	}

	char s[50];
	if (IsScoreNeeded(gCampaign.Entry.Mode))
	{
		if (ConfigGetBool(&gConfig, "Game.Ammo"))
		{
			// Display money instead of ammo
			sprintf(s, "$%d", score);
		}
		else
		{
			sprintf(s, "%d", score);
		}
	}
	else
	{
		s[0] = 0;
	}

	const FontOpts opts = PlayerHUDGetScorePos(flags, r);
	FontStrOpt(s, svec2i_zero(), opts);
}
static void DrawLives(
	const GraphicsDevice *device, const PlayerData *player,
	const struct vec2i pos,
	const FontAlign hAlign, const FontAlign vAlign)
{
	const int xStep = hAlign == ALIGN_START ? 10 : -10;
	const struct vec2i offset = svec2i(2, 5);
	struct vec2i drawPos = svec2i_add(pos, offset);
	if (hAlign == ALIGN_END)
	{
		const int w = device->cachedConfig.Res.x;
		drawPos.x = w - drawPos.x - offset.x;
	}
	if (vAlign == ALIGN_END)
	{
		const int h = device->cachedConfig.Res.y;
		drawPos.y = h - drawPos.y - 1;
	}
	for (int i = 0; i < player->Lives; i++)
	{
		DrawHead(
			device->gameWindow.renderer, &player->Char, DIRECTION_DOWN,
			drawPos);
		drawPos.x += xStep;
	}
}

static void DrawWeaponStatus(
	HUD *hud, const HUDPlayer *h, const TActor *actor, struct vec2i pos,
	const FontAlign hAlign, const FontAlign vAlign)
{
	const Weapon *weapon = ACTOR_GET_WEAPON(actor);
	const WeaponClass *wc = weapon->Gun;

	// Draw gauge if ammo or reloading
	const bool useAmmo =
		ConfigGetBool(&gConfig, "Game.Ammo") && wc->AmmoId >= 0;
	const bool showAmmo = useAmmo && wc->AmmoId >= 0 && h->ammoCounter > 0;
	const uint8_t ammoAlpha = (uint8_t)CLAMP(
		h->ammoCounter * 255 * 2 / AMMO_COUNTER_SHOW_MS, 0, 255);
	const Ammo *ammo = useAmmo ? AmmoGetById(&gAmmo, wc->AmmoId) : NULL;
	const int amount = useAmmo ? ActorWeaponGetAmmo(actor, wc) : 0;
	const struct vec2i gaugePos = svec2i_add(pos, svec2i(-1 + GUN_ICON_PAD, -1));
	const struct vec2i size = svec2i(
		GAUGE_WIDTH - GUN_ICON_PAD, FontH() + 2 + WEAPON_GAUGE_EXTRA_HEIGHT);
	if (showAmmo || weapon->lock > 0)
	{
		const int maxLock = weapon->Gun->Lock;
		const double reloadProgressColorMod = 0.5 +
			0.5 * (weapon->lock / (double) maxLock);
		const uint8_t gaugeAlpha = weapon->lock > 0 ? 255 : ammoAlpha;
		const HSV hsv = { 0.0, 1.0, reloadProgressColorMod };
		color_t barColor = ColorTint(colorWhite, hsv);
		barColor.a = gaugeAlpha;
		int innerWidth;
		if (maxLock == 0 || weapon->lock == 0)
		{
			innerWidth = 0;
		}
		else
		{
			innerWidth = MAX(1, size.x * (maxLock - weapon->lock) / maxLock);
		}
		const color_t backColor = { 128, 128, 128, gaugeAlpha };
		HUDDrawGauge(
			hud->device, gaugePos, size, innerWidth, barColor, backColor,
			hAlign, vAlign);
	}

	// Draw gun icon, and allocate padding to draw the gun icon
	const struct vec2i iconPos = Vec2iAligned(
		svec2i(pos.x - 2, pos.y - 2),
		wc->Icon->size, hAlign, vAlign, hud->device->cachedConfig.Res);
	Blit(hud->device, wc->Icon, iconPos);

	if (showAmmo)
	{
		FontOpts opts = FontOptsNew();
		opts.HAlign = hAlign;
		opts.VAlign = vAlign;
		opts.Area = hud->device->cachedConfig.Res;
		opts.Pad = svec2i(pos.x + GUN_ICON_PAD, pos.y);
		char buf[128];
		// Include ammo counter
		sprintf(buf, "%d", ActorWeaponGetAmmo(actor, wc));

		// If low / no ammo, draw text with different colours, flashing
		uint8_t drawAlpha = ammoAlpha;
		const int fps = ConfigGetInt(&gConfig, "Game.FPS");
		if (amount == 0)
		{
			// No ammo; fast flashing
			const int pulsePeriod = fps / 4;
			if ((gMission.time % pulsePeriod) < (pulsePeriod / 2))
			{
				opts.Mask = colorRed;
			}
			drawAlpha = 255;
		}
		else if (AmmoIsLow(ammo, amount))
		{
			// Low ammo; slow flashing
			const int pulsePeriod = fps / 2;
			if ((gMission.time % pulsePeriod) < (pulsePeriod / 2))
			{
				opts.Mask = colorOrange;
			}
			drawAlpha = 255;
		}
		opts.Mask.a = drawAlpha;
		FontStrOpt(buf, svec2i_zero(), opts);

		// Draw ammo level as inner mini-gauge, no background
		const int yOffset = size.y - AMMO_GAUGE_HEIGHT;
		const struct vec2i gaugeAmmoPos =
			svec2i(gaugePos.x, gaugePos.y + yOffset);
		const struct vec2i gaugeAmmoSize = svec2i(size.x, AMMO_GAUGE_HEIGHT);
		const int ammoGaugeWidth =
			MAX(1, gaugeAmmoSize.x * amount / ammo->Max);
		color_t gaugeColor = colorBlue;
		gaugeColor.a = drawAlpha;
		HUDDrawGauge(
			hud->device, gaugeAmmoPos, gaugeAmmoSize, ammoGaugeWidth,
			gaugeColor, colorTransparent, hAlign, vAlign);
	}
}

static void DrawGrenadeStatus(
	HUD *hud, const HUDPlayer *h, const TActor *a, struct vec2i pos,
	const FontAlign hAlign, const FontAlign vAlign)
{
	const Weapon *grenade = ACTOR_GET_GRENADE(a);
	const WeaponClass *wc = grenade->Gun;
	if (wc == NULL)
	{
		return;
	}

	// Draw number of grenade icons; if there are too many draw one with the
	// amount as text
	const bool useAmmo =
		ConfigGetBool(&gConfig, "Game.Ammo") && wc->AmmoId >= 0;
	const int amount = useAmmo ? ActorWeaponGetAmmo(a, wc) : -1;
	const Pic *icon = WeaponClassGetIcon(wc);
	if (useAmmo && amount > 0 && amount <= MAX_GRENADE_ICONS)
	{
		const struct vec2i iconPosEnd = svec2i_add(
			pos, svec2i(GRENADES_ROW_PAD_LEFT, GRENADES_ROW_PAD_TOP));
		const int width = GAUGE_WIDTH - GRENADES_ROW_PAD_LEFT * 2;
		for (int i = 0; i < amount; i++)
		{
			const int x =
				iconPosEnd.x + width - wc->Icon->size.x -
				width * i / MAX_GRENADE_ICONS;
			const struct vec2i posAligned = Vec2iAligned(
				svec2i(x, iconPosEnd.y),
				icon->size, hAlign, vAlign, hud->device->cachedConfig.Res);
			Blit(hud->device, icon, posAligned);
		}
	}
	else
	{
		char buf[256];
		if (amount >= 0)
		{
			sprintf(buf, " x %d", amount);
		}
		else
		{
			sprintf(buf, " x 99");
		}
		struct vec2i iconPos =
			svec2i_add(pos, svec2i(GAUGE_WIDTH, GRENADES_ROW_PAD_TOP));
		const struct vec2i ammoSize =
			svec2i(FontStrW(buf) + icon->size.x, FontH());
		iconPos.x -= ammoSize.x;
			svec2i(pos.x + GAUGE_WIDTH, pos.y + GRENADES_ROW_PAD_TOP);
		iconPos = Vec2iAligned(
			iconPos, ammoSize, hAlign, vAlign, hud->device->cachedConfig.Res);
		Blit(hud->device, icon, iconPos);
		iconPos.x += wc->Icon->size.x;
		FontStr(buf, iconPos);
	}

	// Grenade name
	if (h->grenadeCounter > 0)
	{
		FontOpts opts = FontOptsNew();
		opts.HAlign = hAlign;
		opts.VAlign = vAlign;
		opts.Area = hud->device->cachedConfig.Res;
		opts.Pad = svec2i(pos.x, pos.y);
		opts.Mask.a = (uint8_t)CLAMP(
			h->grenadeCounter * 255 * 2 / GRENADE_COUNTER_SHOW_MS, 0, 255);
		FontStrOpt(wc->name, svec2i_zero(), opts);
	}
}

static void DrawRadar(
	GraphicsDevice *device, const TActor *p,
	const int flags, const bool showExit)
{
	struct vec2i pos = svec2i_zero();
	int w = device->cachedConfig.Res.x;
	int h = device->cachedConfig.Res.y;

	if (!p)
	{
		return;
	}

	// Possible map positions:
	// top-right (player 1 only)
	// top-left (player 2 only)
	// bottom-right (player 3 only)
	// bottom-left (player 4 only)
	// top-left-of-middle (player 1 when two players)
	// top-right-of-middle (player 2 when two players)
	// bottom-left-of-middle (player 3)
	// bottom-right-of-middle (player 4)
	// top (shared screen)
	if (flags & HUDFLAGS_HALF_SCREEN)
	{
		// two players
		pos.y = AUTOMAP_PADDING;
		if (flags & HUDFLAGS_PLACE_RIGHT)
		{
			// player 2
			pos.x = w / 2 + AUTOMAP_PADDING;
		}
		else
		{
			// player 1
			pos.x = w / 2 - AUTOMAP_SIZE - AUTOMAP_PADDING;
		}
	}
	else if (flags & HUDFLAGS_QUARTER_SCREEN)
	{
		// four players
		if (flags & HUDFLAGS_PLACE_RIGHT)
		{
			// player 2 or 4
			pos.x = w / 2 + AUTOMAP_PADDING;
		}
		else
		{
			// player 1 or 3
			pos.x = w / 2 - AUTOMAP_SIZE - AUTOMAP_PADDING;
		}

		if (flags & HUDFLAGS_PLACE_BOTTOM)
		{
			// player 3 or 4
			pos.y = h - AUTOMAP_SIZE - AUTOMAP_PADDING;
		}
		else
		{
			// player 1 or 2
			pos.y = AUTOMAP_PADDING;
		}
	}
	else
	{
		// one player
		if (flags & HUDFLAGS_PLACE_RIGHT)
		{
			// player 2 or 4
			pos.x = AUTOMAP_PADDING;
		}
		else
		{
			// player 1 or 3
			pos.x = w - AUTOMAP_SIZE - AUTOMAP_PADDING;
		}

		if (flags & HUDFLAGS_PLACE_BOTTOM)
		{
			// player 3 or 4
			pos.y = h - AUTOMAP_SIZE - AUTOMAP_PADDING;
		}
		else
		{
			// player 1 or 2
			pos.y = AUTOMAP_PADDING;
		}
	}

	if (!svec2i_is_zero(pos))
	{
		const struct vec2i playerPos = Vec2ToTile(p->thing.Pos);
		AutomapDrawRegion(
			device->gameWindow.renderer,
			&gMap,
			pos,
			svec2i(AUTOMAP_SIZE, AUTOMAP_SIZE),
			playerPos,
			AUTOMAP_FLAGS_MASK,
			showExit);
	}
}

static void DrawObjectiveCompass(
	GraphicsDevice *g, const struct vec2 playerPos, const Rect2i r,
	const bool showExit);
static void DrawPlayerObjectiveCompass(
	const HUD *hud, TActor *a, const int hudPlayerIndex, const int numViews)
{
	// Draw objective compass
	if (a == NULL)
	{
		return;
	}
	Rect2i r = Rect2iNew(svec2i_zero(), hud->device->cachedConfig.Res);
	if (hudPlayerIndex & 1)
	{
		r.Pos.x = r.Size.x;
	}
	if (hudPlayerIndex >= 2)
	{
		r.Pos.y = r.Size.y;
	}
	if (numViews == 1)
	{
		// No change
	}
	else if (numViews == 2)
	{
		r.Size.x /= 2;
	}
	else if (numViews == 3 || numViews == 4)
	{
		r.Size.x /= 2;
		r.Size.y /= 2;
	}
	else
	{
		CASSERT(false, "not implemented");
	}
	DrawObjectiveCompass(hud->device, a->Pos, r, hud->showExit);
}

static void DrawCompassArrow(
	GraphicsDevice *g,const Rect2i r, const struct vec2 pos,
	const struct vec2 playerPos, const color_t mask, const char *label);
static void DrawObjectiveCompass(
	GraphicsDevice *g, const struct vec2 playerPos, const Rect2i r,
	const bool showExit)
{
	// Draw exit position
	if (showExit)
	{
		DrawCompassArrow(
			g, r, MapGetExitPos(&gMap), playerPos, colorGreen, "Exit");
	}

	// Draw objectives
	Map *map = &gMap;
	struct vec2i tilePos;
	for (tilePos.y = 0; tilePos.y < map->Size.y; tilePos.y++)
	{
		for (tilePos.x = 0; tilePos.x < map->Size.x; tilePos.x++)
		{
			Tile *tile = MapGetTile(map, tilePos);
			CA_FOREACH(ThingId, tid, tile->things)
				Thing *ti = ThingIdGetThing(tid);
				if (!(ti->flags & THING_OBJECTIVE))
				{
					continue;
				}
				const int objective = ObjectiveFromThing(ti->flags);
				const Objective *o =
					CArrayGet(&gMission.missionData->Objectives, objective);
				if (o->Flags & OBJECTIVE_HIDDEN)
				{
					continue;
				}
				if (!(o->Flags & OBJECTIVE_POSKNOWN) && !tile->isVisited)
				{
					continue;
				}
				DrawCompassArrow(g, r, ti->Pos, playerPos, o->color, NULL);
			CA_FOREACH_END()
		}
	}
}

#define COMP_SATURATE_DIST 350
static void DrawCompassArrow(
	GraphicsDevice *g,const Rect2i r, const struct vec2 pos,
	const struct vec2 playerPos, const color_t mask, const char *label)
{
	const struct vec2 compassV = svec2_subtract(pos, playerPos);
	// Don't draw if objective is on screen
	if (fabsf(pos.x - playerPos.x) < r.Size.x / 2 &&
		fabsf(pos.y - playerPos.y) < r.Size.y / 2)
	{
		return;
	}
	// Saturate according to dist from screen edge
	int xDist = (int)fabsf(pos.x - playerPos.x) - r.Size.x / 2;
	int yDist = (int)fabsf(pos.y - playerPos.y) - r.Size.y / 2;
	int lDist;
	xDist > yDist ? lDist = xDist: (lDist = yDist);
	HSV hsv = { -1.0, 1.0,
		2.0 - 1.5 * MIN(lDist, COMP_SATURATE_DIST) / COMP_SATURATE_DIST };
	const color_t tintedMask = ColorTint(mask, hsv);
	struct vec2i textPos = svec2i_zero();
	// Find which edge of screen is the best
	bool hasDrawn = false;
	if (compassV.x != 0)
	{
		double sx = r.Size.x / 2.0 / compassV.x;
		int yInt = (int)floor(fabs(sx) * compassV.y + 0.5);
		if (yInt >= -r.Size.y / 2 && yInt <= r.Size.y / 2)
		{
			// Intercepts either left or right side
			hasDrawn = true;
			if (compassV.x > 0)
			{
				// right edge
				textPos = svec2i(
					r.Pos.x + r.Size.x, r.Pos.y + r.Size.y / 2 + yInt);
				const Pic *p = PicManagerGetPic(&gPicManager, "arrow_right");
				struct vec2i drawPos = svec2i(
					textPos.x - p->size.x, textPos.y - p->size.y / 2);
				BlitMasked(g, p, drawPos, tintedMask, true);
			}
			else if (compassV.x < 0)
			{
				// left edge
				textPos = svec2i(r.Pos.x, r.Pos.y + r.Size.y / 2 + yInt);
				const Pic *p = PicManagerGetPic(&gPicManager, "arrow_left");
				struct vec2i drawPos = svec2i(textPos.x, textPos.y - p->size.y / 2);
				BlitMasked(g, p, drawPos, tintedMask, true);
			}
		}
	}
	if (!hasDrawn && compassV.y != 0)
	{
		double sy = r.Size.y / 2.0 / compassV.y;
		int xInt = (int)floor(fabs(sy) * compassV.x + 0.5);
		if (xInt >= -r.Size.x / 2 && xInt <= r.Size.x / 2)
		{
			// Intercepts either top or bottom side
			if (compassV.y > 0)
			{
				// bottom edge
				textPos = svec2i(
					r.Pos.x + r.Size.x / 2 + xInt, r.Pos.y + r.Size.y);
				const Pic *p = PicManagerGetPic(&gPicManager, "arrow_down");
				struct vec2i drawPos = svec2i(
					textPos.x - p->size.x / 2, textPos.y - p->size.y);
				BlitMasked(g, p, drawPos, tintedMask, true);
			}
			else if (compassV.y < 0)
			{
				// top edge
				textPos = svec2i(r.Pos.x + r.Size.x / 2 + xInt, r.Pos.y);
				const Pic *p = PicManagerGetPic(&gPicManager, "arrow_up");
				struct vec2i drawPos = svec2i(textPos.x - p->size.x / 2, textPos.y);
				BlitMasked(g, p, drawPos, tintedMask, true);
			}
		}
	}
	if (label && strlen(label) > 0)
	{
		struct vec2i textSize = FontStrSize(label);
		// Center the text around the target position
		textPos.x -= textSize.x / 2;
		textPos.y -= textSize.y / 2;
		// Make sure the text is inside the screen
		int padding = 8;
		textPos.x = MAX(textPos.x, r.Pos.x + padding);
		textPos.x = MIN(textPos.x, r.Pos.x + r.Size.x - textSize.x - padding);
		textPos.y = MAX(textPos.y, r.Pos.y + padding);
		textPos.y = MIN(textPos.y, r.Pos.y + r.Size.y - textSize.y - padding);
		FontStrMask(label, textPos, tintedMask);
	}
}

FontOpts PlayerHUDGetScorePos(const int flags, const Rect2i r)
{
	FontOpts opts = FontOptsNew();
	opts.Area = r.Size;
	opts.Pad = svec2i(2, 2);
	// Score aligned to the right
	opts.HAlign = ALIGN_END;
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		opts.Pad.x += r.Pos.x;
		opts.HAlign = ALIGN_START;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		opts.VAlign = ALIGN_END;
	}
	return opts;
}
