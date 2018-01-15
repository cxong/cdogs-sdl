/*
 C-Dogs SDL
 A port of the legendary (and fun) action/arcade cdogs.
 
 Copyright (c) 2013-2016, Cong Xu
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
#include "ui_object.h"

#include <assert.h>
#include <string.h>

#include <cdogs/blit.h>
#include <cdogs/font.h>
#include <cdogs/draw/drawtools.h>


color_t bgColor = { 32, 32, 64, 255 };
color_t menuBGColor = { 48, 48, 48, 255 };
color_t hiliteColor = { 96, 96, 96, 255 };
#define TOOLTIP_PADDING 4


UIObject *UIObjectCreate(UIType type, int id, struct vec2i pos, struct vec2i size)
{
	UIObject *o;
	CCALLOC(o, sizeof *o);
	o->Type = type;
	o->Id = id;
	o->Pos = pos;
	o->Size = size;
	o->IsVisible = true;
	o->ChangeDisablesContext = true;
	switch (type)
	{
	case UITYPE_TEXTBOX:
		o->u.Textbox.IsEditable = true;
		o->ChangesData = 1;
		break;
	case UITYPE_CONTEXT_MENU:
		// Context menu always starts as invisible
		o->IsVisible = false;
		break;
	default:
		// do nothing
		break;
	}
	CArrayInit(&o->Children, sizeof o);
	return o;
}

void UIObjectSetDynamicLabel(UIObject *o, const char *label)
{
	CSTRDUP(o->Label, label);
	o->IsDynamicLabel = true;
}

void UIButtonSetPic(UIObject *o, Pic *pic)
{
	assert(o->Type == UITYPE_BUTTON && "invalid UI type");
	o->u.Button.Pic = pic;
	if (svec2i_is_zero(o->Size))
	{
		o->Size = o->u.Button.Pic->size;
	}
}

UIObject *UIObjectCopy(const UIObject *o)
{
	UIObject *res = UIObjectCreate(o->Type, o->Id, o->Pos, o->Size);
	res->IsVisible = o->IsVisible;
	res->Id2 = o->Id2;
	res->Flags = o->Flags;
	res->Tooltip = o->Tooltip;
	res->Parent = o->Parent;
	res->DoNotHighlight = o->DoNotHighlight;
	res->Label = o->Label;
	res->IsDynamicLabel = o->IsDynamicLabel;
	if (o->IsDynamicLabel)
	{
		CSTRDUP(res->Label, o->Label);
	}
	res->Data = o->Data;
	CASSERT(!o->IsDynamicData, "Cannot copy unknown dynamic data size");
	res->IsDynamicData = false;
	res->ChangeFunc = o->ChangeFunc;
	res->ChangeFuncAlt = o->ChangeFuncAlt;
	res->ChangeDisablesContext = o->ChangeDisablesContext;
	res->ChangesData = o->ChangesData;
	res->ReloadData = o->ReloadData;
	res->OnFocusFunc = o->OnFocusFunc;
	res->OnUnfocusFunc = o->OnUnfocusFunc;
	res->CheckVisible = o->CheckVisible;
	memcpy(&res->u, &o->u, sizeof res->u);
	return res;
}

void UIObjectDestroy(UIObject *o)
{
	CFREE(o->Tooltip);
	if (o->IsDynamicLabel)
	{
		CFREE(o->Label);
	}
	CA_FOREACH(UIObject *, obj, o->Children)
		UIObjectDestroy(*obj);
	CA_FOREACH_END()
	CArrayTerminate(&o->Children);
	if (o->IsDynamicData)
	{
		CFREE(o->Data);
	}
	switch (o->Type)
	{
	case UITYPE_TEXTBOX:
		CFREE(o->u.Textbox.Hint);
		break;
	default:
		// do nothing
		break;
	}
	CFREE(o);
}

void UIObjectAddChild(UIObject *o, UIObject *c)
{
	if (c == NULL)
	{
		return;
	}
	CArrayPushBack(&o->Children, &c);
	c->Parent = o;
	if (o->Type == UITYPE_CONTEXT_MENU)
	{
		// Resize context menu based on children
		o->Size = svec2i_max(o->Size, svec2i_add(c->Pos, c->Size));
	}
}

void UIObjectHighlight(UIObject *o, const bool shift)
{
	if (o->DoNotHighlight)
	{
		return;
	}
	if (o->Parent && o->Parent->Highlighted != o)
	{
		o->Parent->Highlighted = o;
		UIObjectHighlight(o->Parent, shift);
	}
	if (o->OnFocusFunc)
	{
		o->OnFocusFunc(o, o->Data);
	}
	// Show any context menu children
	CA_FOREACH(UIObject *, obj, o->Children)
		if ((*obj)->Type == UITYPE_CONTEXT_MENU && !shift)
		{
			(*obj)->IsVisible = true;
			if ((*obj)->OnFocusFunc)
			{
				(*obj)->OnFocusFunc(*obj, (*obj)->Data);
			}
		}
	CA_FOREACH_END()
}

int UIObjectIsHighlighted(UIObject *o)
{
	return o->Parent == NULL || o->Parent->Highlighted == o;
}

bool UIObjectUnhighlight(UIObject *o, const bool unhighlightParents)
{
	bool changed = false;
	if (o->Highlighted)
	{
		changed =
			UIObjectUnhighlight(o->Highlighted, unhighlightParents) || changed;
	}
	o->Highlighted = NULL;
	if (o->OnUnfocusFunc)
	{
		changed = o->OnUnfocusFunc(o->Data) || changed;
	}
	// Disable any context menu children
	if (o->Type == UITYPE_CONTEXT_MENU)
	{
		o->IsVisible = false;
	}
	CA_FOREACH(UIObject *, obj, o->Children)
		if ((*obj)->Type == UITYPE_CONTEXT_MENU)
		{
			(*obj)->IsVisible = false;
			if ((*obj)->OnUnfocusFunc)
			{
				changed = (*obj)->OnUnfocusFunc((*obj)->Data) || changed;
			}
		}
	CA_FOREACH_END()
	// Unhighlight all parents
	if (unhighlightParents && o->Parent != NULL)
	{
		// Prevent infinite loop
		if (o->Parent->Highlighted == o)
		{
			o->Parent->Highlighted = NULL;
		}
		changed =
			UIObjectUnhighlight(o->Parent, unhighlightParents) || changed;
	}
	return changed;
}

static void DisableContextMenuParents(UIObject *o);
EditorResult UIObjectChange(UIObject *o, const int d, const bool shift)
{
	// Activate change func if available
	bool changed = false;
	if (o->ChangeFuncAlt && shift)
	{
		o->ChangeFuncAlt(o->Data, d);
		changed = true;
	}
	else if (o->ChangeFunc)
	{
		o->ChangeFunc(o->Data, d);
		changed = true;
	}
	if (changed)
	{
		if (o->ChangeDisablesContext)
		{
			DisableContextMenuParents(o);
		}
		return EDITOR_RESULT_NEW(o->ChangesData, o->ReloadData);
	}
	return EDITOR_RESULT_NONE;
}
// Disable all parent context menus once the child is clicked
static void DisableContextMenuParents(UIObject *o)
{
	// Don't disable if we are a textbox
	if (o->Parent && o->Type != UITYPE_TEXTBOX)
	{
		if (o->Parent->Type == UITYPE_CONTEXT_MENU)
		{
			o->Parent->IsVisible = false;
		}
		DisableContextMenuParents(o->Parent);
	}
}

EditorResult UIObjectAddChar(UIObject *o, char c)
{
	if (!o)
	{
		return EDITOR_RESULT_NONE;
	}
	const EditorResult childResult = UIObjectAddChar(o->Highlighted, c);
	if (o->Type != UITYPE_TEXTBOX)
	{
		return childResult;
	}
	else if (childResult != EDITOR_RESULT_NONE)
	{
		// See if there are highlighted textbox children;
		// if so activate them instead
		return childResult;
	}
	if (o->u.Textbox.TextSourceFunc)
	{
		// Dynamically-allocated char buf, expand
		char **s = o->u.Textbox.TextSourceFunc(o->Data);
		if (!s)
		{
			return EDITOR_RESULT_NONE;
		}
		size_t l = *s ? strlen(*s) : 0;
		CREALLOC(*s, l + 2);
		(*s)[l + 1] = 0;
		(*s)[l] = c;
	}
	else
	{
		// Static char buf, simply append
		char *s = o->u.Textbox.TextLinkFunc(o, o->Data);
		size_t l = strlen(s);
		if ((int)l >= o->u.Textbox.MaxLen)
		{
			return EDITOR_RESULT_NONE;
		}
		s[l + 1] = 0;
		s[l] = c;
	}
	if (o->ChangeFunc)
	{
		o->ChangeFunc(o->Data, 1);
	}
	return EDITOR_RESULT_NEW(o->ChangesData, o->ReloadData);
}
EditorResult UIObjectDelChar(UIObject *o)
{
	if (!o)
	{
		return EDITOR_RESULT_NONE;
	}
	const EditorResult childResult = UIObjectDelChar(o->Highlighted);
	if (o->Type != UITYPE_TEXTBOX)
	{
		return childResult;
	}
	else if (childResult != EDITOR_RESULT_NONE)
	{
		// See if there are highlighted textbox children;
		// if so activate them instead
		return childResult;
	}
	char *s = o->u.Textbox.TextLinkFunc(o, o->Data);
	if (!s || s[0] == '\0')
	{
		return EDITOR_RESULT_NONE;
	}
	s[strlen(s) - 1] = 0;
	if (o->ChangeFunc)
	{
		o->ChangeFunc(o->Data, -1);
	}
	return EDITOR_RESULT_NEW(o->ChangesData, o->ReloadData);
}

static int IsInside(struct vec2i pos, struct vec2i rectPos, struct vec2i rectSize);

static const char *LabelGetText(UIObject *o)
{
	assert(o->Type == UITYPE_LABEL && "invalid UIObject type");
	if (o->Label)
	{
		return o->Label;
	}
	else if (o->u.LabelFunc)
	{
		return o->u.LabelFunc(o, o->Data);
	}
	return NULL;
}
typedef struct
{
	UIObject *obj;
	struct vec2i pos;
} UIObjectDrawContext;
static void UIObjectDrawAndAddChildren(
	UIObject *o, GraphicsDevice *g, struct vec2i pos, struct vec2i mouse, CArray *objs)
{
	if (!o)
	{
		return;
	}
	if (o->CheckVisible)
	{
		o->CheckVisible(o, o->Data);
	}
	if (!o->IsVisible)
	{
		return;
	}
	int isHighlighted = UIObjectIsHighlighted(o);
	struct vec2i oPos = svec2i_add(pos, o->Pos);
	switch (o->Type)
	{
	case UITYPE_LABEL:
		{
			const char *text = LabelGetText(o);
			if (!text)
			{
				break;
			}
			color_t textMask = isHighlighted ? colorRed : colorWhite;
			FontStrMaskWrap(text, oPos, textMask, o->Size.x);
		}
		break;
	case UITYPE_TEXTBOX:
		{
			int isText = !!o->u.Textbox.TextLinkFunc;
			const char *text =
				isText ? o->u.Textbox.TextLinkFunc(o, o->Data) : NULL;
			int isEmptyText = !isText || !text || strlen(text) == 0;
			color_t bracketMask = isHighlighted ? colorRed : colorWhite;
			color_t textMask = isEmptyText ? colorGray : colorWhite;
			int oPosX = oPos.x;
			if (isEmptyText)
			{
				text = o->u.Textbox.Hint;
			}
			if (!o->u.Textbox.IsEditable)
			{
				textMask = bracketMask;
			}
			if (o->u.Textbox.IsEditable)
			{
				oPos = FontChMask('>', oPos, bracketMask);
			}
			oPos = FontStrMaskWrap(
				text, oPos, textMask, o->Pos.x + o->Size.x - oPosX);
			if (o->u.Textbox.IsEditable)
			{
				oPos = FontChMask('<', oPos, bracketMask);
			}
			oPos.x = oPosX;
		}
		break;
	case UITYPE_BUTTON:
		{
			int isDown =
				o->u.Button.IsDownFunc && o->u.Button.IsDownFunc(o->Data);
			BlitMasked(
				g, o->u.Button.Pic, oPos, isDown ? colorGray : colorWhite, 1);
		}
		break;
	case UITYPE_CONTEXT_MENU:
		{
			// Draw background
			DrawRectangle(
				g,
				svec2i_add(oPos, svec2i_scale(svec2i_one(), -TOOLTIP_PADDING)),
				svec2i_add(o->Size, svec2i_scale(svec2i_one(), 2 * TOOLTIP_PADDING)),
				menuBGColor,
				0);
			// Find if mouse over any children, and draw highlight
			CA_FOREACH(UIObject *, child, o->Children)
				if (IsInside(mouse, svec2i_add(oPos, (*child)->Pos), (*child)->Size))
				{
					DrawRectangle(
						g,
						svec2i_add(oPos, (*child)->Pos),
						(*child)->Size,
						hiliteColor,
						0);
				}
			CA_FOREACH_END()
		}
		break;
	case UITYPE_CUSTOM:
		o->u.CustomDrawFunc(o, g, pos, o->Data);
		break;
	default:
		// do nothing
		break;
	}

	// add children
	if (objs != NULL)
	{
		CA_FOREACH(UIObject *, obj, o->Children)
			if (!((*obj)->Flags & UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY) ||
				isHighlighted)
			{
				UIObjectDrawContext c;
				c.obj = *obj;
				c.pos = oPos;
				CArrayPushBack(objs, &c);
			}
		CA_FOREACH_END()
	}
}
void UIObjectDraw(
	UIObject *o, GraphicsDevice *g, struct vec2i pos, struct vec2i mouse, CArray *drawObjs)
{
	// Draw this UIObject and its children in BFS order
	// Maintain a queue of UIObjects to draw
	if (drawObjs->elemSize == 0)
	{
		CArrayInit(drawObjs, sizeof(UIObjectDrawContext));
		UIObjectDrawContext c;
		c.obj = o;
		c.pos = pos;
		CArrayPushBack(drawObjs, &c);
		for (int i = 0; i < (int)drawObjs->size; i++)
		{
			UIObjectDrawContext *cPtr = CArrayGet(drawObjs, i);
			UIObjectDrawAndAddChildren(
				cPtr->obj, g, cPtr->pos, mouse, drawObjs);
		}
	}
	else
	{
		for (int i = 0; i < (int)drawObjs->size; i++)
		{
			UIObjectDrawContext *cPtr = CArrayGet(drawObjs, i);
			UIObjectDrawAndAddChildren(
				cPtr->obj, g, cPtr->pos, mouse, NULL);
		}
	}
}

static int IsInside(struct vec2i pos, struct vec2i rectPos, struct vec2i rectSize)
{
	return
		pos.x >= rectPos.x &&
		pos.x < rectPos.x + rectSize.x &&
		pos.y >= rectPos.y &&
		pos.y < rectPos.y + rectSize.y;
}

bool UITryGetObjectImpl(UIObject *o, const struct vec2i pos, UIObject **out);
bool UITryGetObject(UIObject *o, struct vec2i pos, UIObject **out)
{
	if (o == NULL)
	{
		return false;
	}
	// Find the absolute coordinates of the UIObject, by recursing up to its
	// parent
	const UIObject *o2 = o->Parent;
	while (o2 != NULL)
	{
		pos = svec2i_subtract(pos, o2->Pos);
		o2 = o2->Parent;
	}
	return UITryGetObjectImpl(o, pos, out);
}
bool UITryGetObjectImpl(UIObject *o, const struct vec2i pos, UIObject **out)
{
	if (!o->IsVisible)
	{
		return false;
	}
	bool isHighlighted = UIObjectIsHighlighted(o);
	CA_FOREACH(UIObject *, obj, o->Children)
		if ((!((*obj)->Flags & UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY) ||
			isHighlighted) &&
			(*obj)->IsVisible &&
			UITryGetObjectImpl(*obj, svec2i_subtract(pos, o->Pos), out))
		{
			return true;
		}
	CA_FOREACH_END()
	if (IsInside(pos, o->Pos, o->Size) && o->Type != UITYPE_CONTEXT_MENU)
	{
		*out = o;
		return true;
	}
	return false;
}

void UITooltipDraw(GraphicsDevice *device, struct vec2i pos, const char *s)
{
	struct vec2i bgSize = FontStrSize(s);
	pos = svec2i_add(pos, svec2i(10, 10));	// add offset
	DrawRectangle(
		device,
		svec2i_add(pos, svec2i_scale(svec2i_one(), -TOOLTIP_PADDING)),
		svec2i_add(bgSize, svec2i_scale(svec2i_one(), 2 * TOOLTIP_PADDING)),
		bgColor,
		0);
	FontStr(s, pos);
}
