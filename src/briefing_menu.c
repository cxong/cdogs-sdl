#include "briefing_menu.h"

static void BriefingMenuDraw(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data);
static int BriefingMenuInput(int cmd, void *data);

menu_t *MenuCreateBriefing(const char *name, MissionBriefingMenuData *data,
    const struct MissionOptions *m)
{
  menu_t *menu = MenuCreate(name, MENU_TYPE_CUSTOM);
  menu->u.customData.displayFunc = BriefingMenuDraw;
  menu->u.customData.inputFunc = BriefingMenuInput;

  const int w = data->g->cachedConfig.Res.x;
  const int h = data->g->cachedConfig.Res.y;
  const int y = h / 4;
	CCALLOC(data->bData, sizeof *(data->bData));
  
  // Title
  if (m->missionData->Title)
  {
    MissionBriefingTitle(data->bData, m, y);
  }

  MissionBriefingDescription(data->bData, m, w, h, y);

  data->bData->MissionOptions = m;
  menu->u.customData.data = (void*)data;

  return menu;
}

static int BriefingMenuInput(int cmd, void *data) {
  (void)data;
  if (cmd == CMD_BUTTON1 || cmd == CMD_BUTTON2) {
    return 1;
  }
  return 0;
}
static void BriefingMenuDraw(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data) {
  (void)menu;(void)g;(void)pos;(void)size;
  const MissionBriefingMenuData *mData = (const MissionBriefingMenuData*)data;

	FontStr(mData->bData->Description, mData->bData->DescriptionPos);
  MissionBriefingDrawData(mData->bData);
}
