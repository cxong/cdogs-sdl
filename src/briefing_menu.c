#include "briefing_menu.h"

static void BriefingMenuDraw(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data);
static int BriefingMenuInput(int cmd, void *data);

menu_t *MenuCreateBriefing(const char *name, BriefingMenuData *data)
{
  menu_t *menu = MenuCreate(name, MENU_TYPE_CUSTOM);
  menu->u.customData.displayFunc = BriefingMenuDraw;
  menu->u.customData.inputFunc = BriefingMenuInput;

  const int w = data->g->cachedConfig.Res.x;
  const int h = data->g->cachedConfig.Res.y;
  const int y = h / 4;

  // Title
  if (data->m->missionData->Title)
  {
    CMALLOC(data->Title, strlen(data->m->missionData->Title) + 32);
    sprintf(
      data->Title, "Mission %d: %s", data->m->index + 1,
      data->m->missionData->Title);
    data->TitleOpts = FontOptsNew();
    data->TitleOpts.HAlign = ALIGN_CENTER;
    data->TitleOpts.Area = data->g->cachedConfig.Res;
    data->TitleOpts.Pad.y = y - 25;
  }

  // allow some slack for newlines
  CMALLOC(
    data->Description, strlen(data->m->missionData->Description) * 2 + 1);
  // Pad about 1/6th of the screen width total (1/12th left and right)
  FontSplitLines(
    data->m->missionData->Description, data->Description, w * 5 / 6);
  data->DescriptionPos = svec2i(w / 12, y);

  // Objectives
  data->ObjectiveDescPos =
    svec2i(w / 6, y + FontStrH(data->Description) + h / 10);
  data->ObjectiveInfoPos =
    svec2i(w - (w / 6), data->ObjectiveDescPos.y + FontH());
  data->ObjectiveHeight = h / 12;

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
  const BriefingMenuData *mData = (const BriefingMenuData*)data;

	FontStrOpt(mData->Title, svec2i_zero(), mData->TitleOpts);
	FontStr(mData->Description, mData->DescriptionPos);
	// Display objectives
	CA_FOREACH(
		const Objective, o, mData->m->missionData->Objectives)
	// Do not brief optional objectives
	if (o->Required == 0)
	{
		continue;
	}
	struct vec2i offset = svec2i(0, _ca_index * mData->ObjectiveHeight);
	FontStr(o->Description, svec2i_add(mData->ObjectiveDescPos, offset));
	// Draw the icons slightly offset so that tall icons don't overlap each
	// other
	offset.x = -16 * (_ca_index & 1);
	DrawObjectiveInfo(o, svec2i_add(mData->ObjectiveInfoPos, offset));
	CA_FOREACH_END()
}
