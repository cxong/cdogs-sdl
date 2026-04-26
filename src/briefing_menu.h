#pragma once

#include <cdogs/font.h>

#include "briefing_screens.h"
#include "files.h"
#include "menu.h"

typedef struct
{
  const struct MissionOptions *m;
  GraphicsDevice *g;
	void (*gfxChangeCallback)(void *, const bool);
	void *gfxChangeData;
	char *Title;
	FontOpts TitleOpts;
	char *Description;
	struct vec2i DescriptionPos;
	struct vec2i ObjectiveDescPos;
	struct vec2i ObjectiveInfoPos;
	int ObjectiveHeight;
} BriefingMenuData;

menu_t *MenuCreateBriefing(const char *name, BriefingMenuData *data);
