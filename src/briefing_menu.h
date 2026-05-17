#pragma once

#include <cdogs/font.h>

#include "briefing_screens.h"
#include "files.h"
#include "menu.h"

typedef struct
{
  MissionBriefingData *bData;
  GraphicsDevice *g;
	void (*gfxChangeCallback)(void *, const bool);
	void *gfxChangeData;
} MissionBriefingMenuData;

menu_t *MenuCreateBriefing(const char *name, MissionBriefingMenuData *data,
    const struct MissionOptions *m);
