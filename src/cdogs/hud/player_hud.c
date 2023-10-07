/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2017, 2019-2021 Cong Xu
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
#include "draw/nine_slice.h"
#include "hud/gauge.h"
#include "hud_defs.h"

#define SCORE_WIDTH 26
#define GRENADES_WIDTH 30
#define AMMO_WIDTH 27
#define GUN_ICON_WIDTH 14
#define PLAYER_ICON_WIDTH 20
#define BAR_HEIGHT 13

void HUDPlayerInit(HUDPlayer *h)
{
	memset(h, 0, sizeof *h);
	HealthGaugeInit(&h->healthGauge);
}

static const TActor *GetActor(const PlayerData *p)
{
	if (!IsPlayerAlive(p))
	{
		return NULL;
	}
	const TActor *a = ActorGetByUID(p->ActorUID);
	if (a != NULL)
	{
		if (a->vehicleUID != -1)
		{
			// Draw HUD for vehicle if piloting
			a = ActorGetByUID(a->vehicleUID);
		}
	}
	return a;
}

void HUDPlayerUpdate(HUDPlayer *h, const PlayerData *p, const int ms)
{
	const TActor *a = GetActor(p);
	if (a == NULL)
		return;

	// Health
	HealthGaugeUpdate(&h->healthGauge, a, ms);
}

static void DrawPlayerStatus(
	HUD *hud, const PlayerData *data, const TActor *p, const int flags,
	const HUDPlayer *h, const Rect2i r);
static void DrawPlayerObjectiveCompass(
	const HUD *hud, const TActor *a, const int hudPlayerIndex,
	const int numViews);
void DrawPlayerHUD(
	HUD *hud, const PlayerData *p, const int drawFlags,
	const int hudPlayerIndex, const Rect2i r, const int numViews)
{
	const TActor *a = GetActor(p);
	DrawPlayerStatus(
		hud, p, a, drawFlags, &hud->hudPlayers[hudPlayerIndex], r);
	HUDNumPopupsDrawPlayer(&hud->numPopups, hudPlayerIndex, drawFlags, r);
	DrawPlayerObjectiveCompass(hud, a, hudPlayerIndex, numViews);
}

static void DrawPlayerIcon(
	const TActor *a, GraphicsDevice *g, const PicManager *pm, const int flags,
	const SDL_RendererFlip flip, const color_t mask);
static void DrawScore(
	GraphicsDevice *g, const PicManager *pm, const TActor *a, const int score,
	const int flags, const Rect2i r, const color_t mask);
static void DrawLives(
	const GraphicsDevice *device, const PlayerData *player,
	const FontAlign hAlign, const FontAlign vAlign);
static void DrawWeaponStatus(
	GraphicsDevice *g, const PicManager *pm, const TActor *actor,
	const int flags, const Rect2i r, const color_t mask);
static void DrawGunIcons(
	GraphicsDevice *g, const TActor *actor, const int flags, const Rect2i r);
static void DrawGrenadeStatus(
	GraphicsDevice *g, const TActor *a, const int flags, const Rect2i r);
static void DrawRadar(
	GraphicsDevice *device, const TActor *p, const int flags,
	const bool showExit);
static void DrawHealth(
	GraphicsDevice *g, const TActor *a, const int flags, const HUDPlayer *h,
	const Rect2i r, const color_t mask);
// Draw player's score, health etc.
static void DrawPlayerStatus(
	HUD *hud, const PlayerData *data, const TActor *p, const int flags,
	const HUDPlayer *h, const Rect2i r)
{
	const color_t mask = data->Char.Colors.Body;
	SDL_RendererFlip flip = SDL_FLIP_NONE;
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		flip |= SDL_FLIP_HORIZONTAL;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		flip |= SDL_FLIP_VERTICAL;
	}

	DrawPlayerIcon(p, hud->device, &gPicManager, flags, flip, mask);

	// Draw back bar, stretched across the screen
	const Pic *backBar = PicManagerGetPic(&gPicManager, "hud/back_bar");
	const int barWidth = r.Size.x - 22;
	struct vec2i barPos = svec2i(22, 0);
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		barPos.x = r.Pos.x;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		barPos.y = hud->device->cachedConfig.Res.y - backBar->size.y;
	}
	Draw9Slice(
		hud->device, backBar, Rect2iNew(barPos, svec2i(barWidth, BAR_HEIGHT)),
		0, 0, 0, 0, true, mask, flip);

	FontOpts opts = FontOptsNew();
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		opts.HAlign = ALIGN_END;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		opts.VAlign = ALIGN_END;
	}
	opts.Area = gGraphicsDevice.cachedConfig.Res;

	DrawScore(hud->device, &gPicManager, p, data->Stats.Score, flags, r, mask);
	DrawGrenadeStatus(hud->device, p, flags, r);
	DrawWeaponStatus(hud->device, &gPicManager, p, flags, r, mask);
	DrawGunIcons(hud->device, p, flags, r);
	DrawLives(hud->device, data, opts.HAlign, opts.VAlign);
	DrawHealth(hud->device, p, flags, h, r, mask);

	// Name
	opts.Pad = svec2i(23, 2);
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		opts.Pad.y += 1;
	}
	char buf[256];
	if (p != NULL && p->pilotUID != p->uid)
	{
		// Actor is a vehicle
		sprintf(buf, "%s (%s)", data->name, ActorGetCharacter(p)->Class->Name);
	}
	else
	{
		strcpy(buf, data->name);
	}
	FontStrOpt(buf, svec2i_zero(), opts);

	if (ConfigGetBool(&gConfig, "Interface.ShowHUDMap") &&
		!(flags & HUDFLAGS_SHARE_SCREEN) &&
		IsAutoMapEnabled(gCampaign.Entry.Mode))
	{
		DrawRadar(hud->device, p, flags, hud->showExit);
	}
}
static void DrawPlayerIcon(
	const TActor *a, GraphicsDevice *g, const PicManager *pm, const int flags,
	const SDL_RendererFlip flip, const color_t mask)
{
	const Pic *framePic = PicManagerGetPic(pm, "hud/player_frame");
	const Pic *underlayPic = PicManagerGetPic(pm, "hud/player_frame_underlay");
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
		svec2_one(), flip, Rect2iZero());
	if (a)
	{
		ActorPics pics = GetCharacterPicsFromActor(a);
		pics.ShadowMask = colorTransparent;
		const struct vec2i offset = svec2i(-2, 10);
		struct vec2i pos = svec2i(framePic->size.x / 2, framePic->size.y / 2);
		if (flags & HUDFLAGS_PLACE_RIGHT)
		{
			pos.x = g->cachedConfig.Res.x - pos.x + 4;
		}
		if (flags & HUDFLAGS_PLACE_BOTTOM)
		{
			pos.y = g->cachedConfig.Res.y - pos.y + 2;
		}
		DrawActorPics(
			&pics, svec2i_add(pos, offset),
			Rect2iNew(svec2i(-12, -12), svec2i(12, 5)));
	}
	PicRender(
		framePic, g->gameWindow.renderer, picPos, mask, 0, svec2_one(), flip,
		Rect2iZero());
}
static void DrawScore(
	GraphicsDevice *g, const PicManager *pm, const TActor *a, const int score,
	const int flags, const Rect2i r, const color_t mask)
{
	// Score aligned to the right
	struct vec2i backPos = svec2i(r.Size.x - SCORE_WIDTH, 1);
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		backPos.x = r.Pos.x;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		backPos.y = g->cachedConfig.Res.y - BAR_HEIGHT + backPos.y;
	}

	HUDDrawGauge(g, pm, backPos, SCORE_WIDTH, 0, colorTransparent, mask);

	if (a == NULL)
	{
		return;
	}

	char s[50];
	if (IsScoreNeeded(gCampaign.Entry.Mode))
	{
		if (gCampaign.Setting.Ammo)
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
	FontStrOpt(s, r.Pos, opts);
}
static void DrawLives(
	const GraphicsDevice *device, const PlayerData *player,
	const FontAlign hAlign, const FontAlign vAlign)
{
	const struct vec2i pos = svec2i(PLAYER_ICON_WIDTH + 4, 11);
	const int xStep = (hAlign == ALIGN_START ? 1 : -1) * 8;
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
	GraphicsDevice *g, const PicManager *pm, const TActor *actor,
	const int flags, const Rect2i r, const color_t mask)
{
	// TODO: draw as gauge
	const Pic *backPic = PicManagerGetPic(pm, "hud/gauge_small_back");
	const struct vec2i backPicSize = svec2i(AMMO_WIDTH - 1, backPic->size.y);

	// Aligned to the right
	const int right = AMMO_WIDTH + SCORE_WIDTH + GRENADES_WIDTH;
	struct vec2i pos = svec2i(r.Size.x - right, 2);
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		pos.x = r.Pos.x + SCORE_WIDTH + GRENADES_WIDTH;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		pos.y = g->cachedConfig.Res.y - BAR_HEIGHT + pos.y;
	}

	Draw9Slice(
		g, backPic, Rect2iNew(pos, backPicSize), 0, 2, 0, 4, false, mask,
		SDL_FLIP_NONE);

	if (actor == NULL)
	{
		return;
	}

	const Weapon *weapon = ACTOR_GET_WEAPON(actor);
	const WeaponClass *wc = weapon->Gun;
	if (wc == NULL)
	{
		return;
	}
	// TODO: support drawing multi weapons in HUD
	const int barrel = 0;
	wc = WeaponClassGetBarrel(wc, barrel);

	// Draw gauge and ammo counter if ammo used
	if (gCampaign.Setting.Ammo && wc->u.Normal.AmmoId >= 0)
	{
		const Ammo *ammo = AmmoGetById(&gAmmo, wc->u.Normal.AmmoId);
		const int amount = ActorWeaponGetAmmo(actor, wc, barrel);
		FontOpts opts = FontOptsNew();
		opts.Area = g->cachedConfig.Res;
		opts.Pad = svec2i(pos.x + AMMO_WIDTH / 2, pos.y);

		// Draw ammo level as inner fill
		if (amount > 0)
		{
			const Pic *fillPic = PicManagerGetPic(pm, "hud/gauge_small_inner");
			const int ammoMax = ammo->Max ? ammo->Max : amount;
			const struct vec2i fillPicSize = svec2i(
				MAX(1, (AMMO_WIDTH - 1) * amount / ammoMax),
				fillPic->size.y);
			Draw9Slice(
				g, fillPic, Rect2iNew(svec2i(pos.x, pos.y), fillPicSize), 1, 1,
				1, 1, false, colorBlue, SDL_FLIP_NONE);
		}

		// Include ammo counter
		char buf[128];
		sprintf(buf, "%d", amount);

		// If low / no ammo, draw text with different colours, flashing
		const int fps = ConfigGetInt(&gConfig, "Game.FPS");
		if (amount == 0)
		{
			// No ammo; fast flashing
			const int pulsePeriod = fps / 4;
			if ((gMission.time % pulsePeriod) < (pulsePeriod / 2))
			{
				opts.Mask = colorRed;
			}
		}
		else if (AmmoIsLow(ammo, amount))
		{
			// Low ammo; slow flashing
			const int pulsePeriod = fps / 2;
			if ((gMission.time % pulsePeriod) < (pulsePeriod / 2))
			{
				opts.Mask = colorOrange;
			}
		}
		FontStrOpt(buf, svec2i_zero(), opts);
	}

	// Draw reload balls
	for (int i = 0; i < WeaponClassNumBarrels(weapon->Gun); i++)
	{
		if (weapon->barrels[i].lock > 0)
		{
			const int maxLock = weapon->Gun->Lock;
			const Pic *ballPic = PicManagerGetPic(pm, "hud/gauge_small_ball");
			const int ballAreaWidth = AMMO_WIDTH - 6;
			const struct vec2i ballPos = svec2i(
				pos.x +
					MAX(0, ballAreaWidth *
							   (maxLock - weapon->barrels[i].lock) / maxLock),
				pos.y + 1);
			PicRender(
				ballPic, g->gameWindow.renderer, ballPos, colorWhite, 0.0,
				svec2_one(), SDL_FLIP_NONE, Rect2iZero());
		}
	}
}

static void DrawGunIcons(
	GraphicsDevice *g, const TActor *actor, const int flags, const Rect2i r)
{
	if (actor == NULL)
	{
		return;
	}

	const Weapon *weapon = ACTOR_GET_WEAPON(actor);
	const WeaponClass *wc = weapon->Gun;
	if (wc == NULL)
	{
		return;
	}

	// Aligned right
	const int right =
		GUN_ICON_WIDTH + AMMO_WIDTH + SCORE_WIDTH + GRENADES_WIDTH;
	struct vec2i pos = svec2i(r.Size.x - right, 0);
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		pos.x = r.Pos.x + AMMO_WIDTH + SCORE_WIDTH + GRENADES_WIDTH;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		pos.y = g->cachedConfig.Res.y - BAR_HEIGHT + pos.y;
	}

	// Ammo icon
	// TODO: support multi ammo guns
	const int ammoId = WC_BARREL_ATTR(*wc, AmmoId, 0);
	if (gCampaign.Setting.Ammo && ammoId >= 0)
	{
		const Ammo *ammo = AmmoGetById(&gAmmo, ammoId);
		const struct vec2i ammoPos = svec2i_add(pos, svec2i(6, 5));
		CPicDraw(g, &ammo->Pic, ammoPos, NULL);
	}

	// Gun icon
	PicRender(
		wc->Icon, g->gameWindow.renderer, pos, colorWhite, 0.0, svec2_one(),
		SDL_FLIP_NONE, Rect2iZero());
}

static void DrawGrenadeIcons(
	GraphicsDevice *g, const Pic *icon, const struct vec2i pos,
	const int width, const int amount);
static void DrawGrenadeStatus(
	GraphicsDevice *g, const TActor *a, const int flags, const Rect2i r)
{
	if (a == NULL)
	{
		return;
	}

	const Weapon *grenade = ACTOR_GET_GRENADE(a);
	const WeaponClass *wc = grenade->Gun;
	if (wc == NULL)
	{
		return;
	}
	CASSERT(wc->Type == GUNTYPE_GRENADE, "unexpected gun type");

	// Aligned to the right
	const int right = GRENADES_WIDTH + SCORE_WIDTH;
	struct vec2i pos = svec2i(r.Size.x - right, 1);
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		pos.x = r.Pos.x + SCORE_WIDTH;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		pos.y = g->cachedConfig.Res.y - BAR_HEIGHT + pos.y;
	}

	// Draw number of grenade icons; if there are too many draw one with the
	// amount as text
	const bool useAmmo = gCampaign.Setting.Ammo && wc->u.Normal.AmmoId >= 0;
	const int amount = useAmmo ? ActorWeaponGetAmmo(a, wc, 0) : -1;
	const Pic *icon = wc->Icon;
	if (useAmmo && amount > 0 && amount <= MAX_GRENADE_ICONS)
	{
		DrawGrenadeIcons(g, icon, pos, GRENADES_WIDTH, amount);
	}
	else
	{
		DrawGrenadeIcons(g, icon, pos, GRENADES_WIDTH, 1);
		char buf[256];
		if (amount >= 0)
		{
			sprintf(buf, " x %d", amount);
		}
		else
		{
			sprintf(buf, " x 99");
		}
		FontOpts opts = FontOptsNew();
		opts.HAlign = ALIGN_END;
		FontStrOpt(buf, svec2i(pos.x + GRENADES_WIDTH - 5, pos.y + 1), opts);
	}
}
static void DrawGrenadeIcons(
	GraphicsDevice *g, const Pic *icon, const struct vec2i pos,
	const int width, const int amount)
{
	const int dx = width / MAX_GRENADE_ICONS;
	for (int i = 0; i < amount; i++)
	{
		const struct vec2i drawPos = svec2i(pos.x + i * dx, pos.y);
		PicRender(
			icon, g->gameWindow.renderer, drawPos, colorWhite, 0.0,
			svec2_one(), SDL_FLIP_NONE, Rect2iZero());
	}
}

static void DrawRadar(
	GraphicsDevice *device, const TActor *p, const int flags,
	const bool showExit)
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
			device->gameWindow.renderer, &gMap, pos,
			svec2i(AUTOMAP_SIZE, AUTOMAP_SIZE), playerPos, AUTOMAP_FLAGS_MASK,
			showExit);
	}
}

static void DrawHealth(
	GraphicsDevice *g, const TActor *a, const int flags, const HUDPlayer *h,
	const Rect2i r, const color_t mask)
{
	const int right =
		SCORE_WIDTH + GRENADES_WIDTH + AMMO_WIDTH + GUN_ICON_WIDTH;
	const int width = r.Size.x - right - PLAYER_ICON_WIDTH;

	struct vec2i backPos = svec2i(PLAYER_ICON_WIDTH + 1, 1);
	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_END;
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		backPos.x = r.Pos.x + right;
		opts.HAlign = ALIGN_START;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		backPos.y = g->cachedConfig.Res.y - 10 - 1;
	}

	HealthGaugeDraw(&h->healthGauge, g, a, backPos, width, opts, mask);
}

static void DrawObjectiveCompass(
	GraphicsDevice *g, const struct vec2 playerPos, const Rect2i r,
	const bool showExit);
static void DrawPlayerObjectiveCompass(
	const HUD *hud, const TActor *a, const int hudPlayerIndex,
	const int numViews)
{
	// Draw objective compass
	if (a == NULL)
	{
		return;
	}
	Rect2i r = Rect2iNew(svec2i(hud->device->cachedConfig.Res.x / 4, hud->device->cachedConfig.Res.y / 4), svec2i(hud->device->cachedConfig.Res.x / 2, hud->device->cachedConfig.Res.y / 2));
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
	GraphicsDevice *g, const Rect2i r, const struct vec2 pos,
	const struct vec2 playerPos, const color_t mask, const char *label);
static void DrawObjectiveCompass(
	GraphicsDevice *g, const struct vec2 playerPos, const Rect2i r,
	const bool showExit)
{
	// Draw exit positions
	if (showExit)
	{
		CA_FOREACH(const Exit, e, gMap.exits)
		if (e->Hidden)
			continue;
		DrawCompassArrow(
			g, r, MapGetExitPos(&gMap, _ca_index), playerPos, colorGreen,
			"Exit");
		CA_FOREACH_END()
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
	GraphicsDevice *g, const Rect2i r, const struct vec2 pos,
	const struct vec2 playerPos, const color_t mask, const char *label)
{
	const struct vec2 compassV = svec2_subtract(
		pos,
		// Offset a little so the arrow points exactly at center of objective
		svec2(playerPos.x - TILE_WIDTH / 2, playerPos.y - 20));
	// Don't draw if objective is on screen
	if (fabsf(compassV.x) < r.Size.x / 2 && fabsf(compassV.y) < r.Size.y / 2)
	{
		return;
	}
	// Saturate according to dist from screen edge
	int xDist = (int)fabsf(compassV.x) - r.Size.x / 2;
	int yDist = (int)fabsf(compassV.y) - r.Size.y / 2;
	int lDist;
	xDist > yDist ? lDist = xDist : (lDist = yDist);
	HSV hsv = {
		-1.0, 1.0,
		2.0 - 1.5 * MIN(lDist, COMP_SATURATE_DIST) / COMP_SATURATE_DIST};
	const color_t tintedMask = ColorTint(mask, hsv);
	struct vec2i textPos = svec2i_zero();
	struct vec2i drawPos = svec2i_zero();
	const Pic *p = PicManagerGetPic(&gPicManager, "hud/arrow");
	// Find which edge of screen is the best
	bool draw = false;
	if (compassV.x != 0)
	{
		double sx = r.Size.x / 2.0 / compassV.x;
		int yInt = (int)floor(fabs(sx) * compassV.y + 0.5);
		if (yInt >= -r.Size.y / 2 && yInt <= r.Size.y / 2)
		{
			draw = true;
			// Intercepts either left or right side
			if (compassV.x > 0)
			{
				// right edge
				textPos =
					svec2i(r.Pos.x + r.Size.x, r.Pos.y + r.Size.y / 2 + yInt);
				drawPos =
					svec2i(textPos.x - p->size.x, textPos.y - p->size.y / 2);
			}
			else
			{
				// left edge
				textPos = svec2i(r.Pos.x, r.Pos.y + r.Size.y / 2 + yInt);
				drawPos = svec2i(textPos.x, textPos.y - p->size.y / 2);
			}
		}
	}
	if (!draw && compassV.y != 0)
	{
		double sy = r.Size.y / 2.0 / compassV.y;
		int xInt = (int)floor(fabs(sy) * compassV.x + 0.5);
		if (xInt >= -r.Size.x / 2 && xInt <= r.Size.x / 2)
		{
			draw = true;
			// Intercepts either top or bottom side
			if (compassV.y > 0)
			{
				// bottom edge
				textPos =
					svec2i(r.Pos.x + r.Size.x / 2 + xInt, r.Pos.y + r.Size.y);
				drawPos =
					svec2i(textPos.x - p->size.x / 2, textPos.y - p->size.y);
			}
			else
			{
				// top edge
				textPos = svec2i(r.Pos.x + r.Size.x / 2 + xInt, r.Pos.y);
				drawPos = svec2i(textPos.x - p->size.x / 2, textPos.y);
			}
		}
	}
	if (draw)
	{
		PicRender(
			p, g->gameWindow.renderer, drawPos, tintedMask,
			svec2_angle(compassV) + MPI_2, svec2_one(), SDL_FLIP_NONE,
			Rect2iZero());
		if (label && strlen(label) > 0)
		{
			struct vec2i textSize = FontStrSize(label);
			// Center the text around the target position
			textPos.x -= textSize.x / 2;
			textPos.y -= textSize.y / 2;
			// Make sure the text is inside the screen
			int padding = 8;
			textPos.x = MAX(textPos.x, r.Pos.x + padding);
			textPos.x =
				MIN(textPos.x, r.Pos.x + r.Size.x - textSize.x - padding);
			textPos.y = MAX(textPos.y, r.Pos.y + padding);
			textPos.y =
				MIN(textPos.y, r.Pos.y + r.Size.y - textSize.y - padding);
			FontStrMask(label, textPos, tintedMask);
		}
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
		opts.HAlign = ALIGN_START;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		opts.VAlign = ALIGN_END;
		opts.Pad.y += 1;
	}
	return opts;
}
