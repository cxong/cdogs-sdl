/*
 C-Dogs SDL
 A port of the legendary (and fun) action/arcade cdogs.
 
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
#include "ui_object.h"

#include <assert.h>
#include <string.h>

#include <cdogs/blit.h>
#include <cdogs/drawtools.h>
#include <cdogs/text.h>

color_t bgColor = { 32, 32, 64, 255 };
color_t hiliteColor = { 64, 64, 128, 255 };
#define TOOLTIP_PADDING 4


UIObject *UIObjectCreate(UIType type, int id, Vec2i pos, Vec2i size)
{
	UIObject *o;
	CCALLOC(o, sizeof *o);
	o->Type = type;
	o->Id = id;
	o->Pos = pos;
	o->Size = size;
	o->IsVisible = 1;
	switch (type)
	{
	case UITYPE_TEXTBOX:
		o->u.Textbox.IsEditable = 1;
		break;
	case UITYPE_TAB:
		CArrayInit(&o->u.Tab.Labels, sizeof(char *));
		o->u.Tab.Index = 0;
		break;
	case UITYPE_CONTEXT_MENU:
		// Context menu always disabled unless parent highlighted
		o->Flags |= UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;
		break;
	}
	CArrayInit(&o->Children, sizeof o);
	return o;
}
void UIButtonSetPic(UIObject *o, Pic *pic)
{
	assert(o->Type == UITYPE_BUTTON && "invalid UI type");
	o->u.Button.Pic = pic;
	if (Vec2iEqual(o->Size, Vec2iZero()))
	{
		o->Size = o->u.Button.Pic->size;
	}
}

UIObject *UIObjectCopy(UIObject *o)
{
	UIObject *res = UIObjectCreate(o->Type, o->Id, o->Pos, o->Size);
	res->IsVisible = o->IsVisible;
	res->Id2 = o->Id2;
	res->Flags = o->Flags;
	res->Tooltip = o->Tooltip;
	res->Parent = o->Parent;
	res->Data = o->Data;
	assert(!o->IsDynamicData && "Cannot copy unknown dynamic data size");
	res->IsDynamicData = 0;
	res->ChangeFunc = o->ChangeFunc;
	res->ChangesData = o->ChangesData;
	res->OnFocusFunc = o->OnFocusFunc;
	res->OnUnfocusFunc = o->OnUnfocusFunc;
	memcpy(&res->u, &o->u, sizeof res->u);
	return res;
}

void UIObjectDestroy(UIObject *o)
{
	size_t i;
	UIObject **objs = o->Children.data;
	CFREE(o->Tooltip);
	for (i = 0; i < o->Children.size; i++, objs++)
	{
		UIObjectDestroy(*objs);
	}
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
	case UITYPE_TAB:
		CArrayTerminate(&o->u.Tab.Labels);
		break;
	}
	CFREE(o);
}

void UIObjectAddChild(UIObject *o, UIObject *c)
{
	CArrayPushBack(&o->Children, &c);
	c->Parent = o;
	assert(o->Type != UITYPE_TAB && "need special add child for TAB type");
	if (o->Type == UITYPE_CONTEXT_MENU)
	{
		// Resize context menu based on children
		o->Size = Vec2iMax(o->Size, Vec2iAdd(c->Pos, c->Size));
	}
}
void UITabAddChild(UIObject *o, UIObject *c, char *label)
{
	CArrayPushBack(&o->Children, &c);
	c->Parent = o;
	CArrayPushBack(&o->u.Tab.Labels, &label);
}

void UIObjectHighlight(UIObject *o)
{
	if (o->Parent)
	{
		o->Parent->Highlighted = o;
		UIObjectHighlight(o->Parent);
	}
	if (o->OnFocusFunc)
	{
		o->OnFocusFunc(o->Data);
	}
}

int UIObjectIsHighlighted(UIObject *o)
{
	return o->Parent && o->Parent->Highlighted == o;
}

void UIObjectUnhighlight(UIObject *o)
{
	if (o->Highlighted)
	{
		UIObjectUnhighlight(o->Highlighted);
	}
	o->Highlighted = NULL;
	if (o->OnUnfocusFunc)
	{
		o->OnUnfocusFunc(o->Data);
	}
}

int UIObjectChange(UIObject *o, int d)
{
	switch (o->Type)
	{
	case UITYPE_TAB:
		// switch child
		o->u.Tab.Index = CLAMP_OPPOSITE(
			o->u.Tab.Index + d, 0, (int)o->u.Tab.Labels.size - 1);
		break;
	}
	if (o->ChangeFunc)
	{
		o->ChangeFunc(o->Data, d);
		if (o->Parent && o->Parent->Type == UITYPE_CONTEXT_MENU)
		{
			// Must unhighlight parent of context menu
			UIObjectUnhighlight(o->Parent->Parent);
		}
		return o->ChangesData;
	}
	return 0;
}

static int IsInside(Vec2i pos, Vec2i rectPos, Vec2i rectSize);

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
	Vec2i pos;
} UIObjectDrawContext;
static void UIObjectDrawAndAddChildren(
	UIObject *o, GraphicsDevice *g, Vec2i pos, Vec2i mouse, CArray *objs)
{
	int isHighlighted;
	if (!o)
	{
		return;
	}
	isHighlighted = UIObjectIsHighlighted(o);
	Vec2i oPos = Vec2iAdd(pos, o->Pos);
	switch (o->Type)
	{
	case UITYPE_LABEL:
		{
			if (!o->IsVisible)
			{
				return;
			}
			const char *text = LabelGetText(o);
			if (!text)
			{
				break;
			}
			color_t textMask = isHighlighted ? colorRed : colorWhite;
			DrawTextStringMaskedWrapped(
				text, g, oPos, textMask, o->Size.x);
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
			if (!o->IsVisible)
			{
				return;
			}
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
				oPos = DrawTextCharMasked('\020', g, oPos, bracketMask);
			}
			oPos = DrawTextStringMaskedWrapped(
				text, g, oPos, textMask,
				o->Pos.x + o->Size.x - oPosX);
			if (o->u.Textbox.IsEditable)
			{
				oPos = DrawTextCharMasked('\021', g, oPos, bracketMask);
			}
		}
		break;
	case UITYPE_TAB:
		if (o->Children.size > 0)
		{
			color_t textMask = isHighlighted ? colorRed : colorWhite;
			char **labelp = CArrayGet(&o->u.Tab.Labels, o->u.Tab.Index);
			UIObject **objp = CArrayGet(&o->Children, o->u.Tab.Index);
			if (!o->IsVisible)
			{
				return;
			}
			DrawTextStringMaskedWrapped(
				*labelp, g, Vec2iAdd(pos, o->Pos), textMask, o->Size.x);
			if (!((*objp)->Flags & UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY) ||
				isHighlighted)
			{
				UIObjectDraw(*objp, g, oPos, mouse);
			}
		}
		break;
	case UITYPE_BUTTON:
		{
			int isDown =
				o->u.Button.IsDownFunc && o->u.Button.IsDownFunc(o->Data);
			if (isDown)
			{
				BlitMasked(g, o->u.Button.Pic, oPos, colorGray, 1);
			}
			else
			{
				BlitMasked(g, o->u.Button.Pic, oPos, colorWhite, 1);
			}
		}
		break;
	case UITYPE_CONTEXT_MENU:
		{
			// Draw background
			DrawRectangle(
				g,
				Vec2iAdd(oPos, Vec2iScale(Vec2iUnit(), -TOOLTIP_PADDING)),
				Vec2iAdd(o->Size, Vec2iScale(Vec2iUnit(), 2 * TOOLTIP_PADDING)),
				bgColor,
				0);
			// Find if mouse over any children, and draw highlight
			for (int i = 0; i < (int)o->Children.size; i++)
			{
				UIObject *child = *(UIObject **)CArrayGet(&o->Children, i);
				if (IsInside(mouse, Vec2iAdd(oPos, child->Pos), child->Size))
				{
					DrawRectangle(
						g,
						Vec2iAdd(oPos, child->Pos),
						child->Size,
						hiliteColor,
						0);
				}
			}
		}
		break;
	case UITYPE_CUSTOM:
		o->u.CustomDrawFunc(o, g, pos, o->Data);
		if (!o->IsVisible)
		{
			return;
		}
		break;
	}

	// add children
	// Note: tab type draws its own children (one)
	if (o->Type != UITYPE_TAB)
	{
		size_t i;
		UIObject **childPtr = o->Children.data;
		for (i = 0; i < o->Children.size; i++, childPtr++)
		{
			if (!((*childPtr)->Flags & UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY) ||
				isHighlighted)
			{
				UIObjectDrawContext c;
				c.obj = *childPtr;
				c.pos = oPos;
				CArrayPushBack(objs, &c);
			}
		}
	}
}
void UIObjectDraw(UIObject *o, GraphicsDevice *g, Vec2i pos, Vec2i mouse)
{
	// Draw this UIObject and its children in BFS order
	// Maintain a queue of UIObjects to draw
	CArray objs;	// of UIObjectDrawContext
	CArrayInit(&objs, sizeof(UIObjectDrawContext));
	UIObjectDrawContext c;
	c.obj = o;
	c.pos = pos;
	CArrayPushBack(&objs, &c);
	for (int i = 0; i < (int)objs.size; i++)
	{
		UIObjectDrawContext *cPtr = CArrayGet(&objs, i);
		UIObjectDrawAndAddChildren(
			cPtr->obj, g, cPtr->pos, mouse, &objs);
	}
	CArrayTerminate(&objs);
}

static int IsZeroUIObject(UIObject *o)
{
	return
		o->Id == 0 && o->Flags == 0 &&
		Vec2iEqual(o->Pos, Vec2iZero()) && Vec2iEqual(o->Size, Vec2iZero());
}

static int IsInside(Vec2i pos, Vec2i rectPos, Vec2i rectSize)
{
	return
		pos.x >= rectPos.x &&
		pos.x < rectPos.x + rectSize.x &&
		pos.y >= rectPos.y &&
		pos.y < rectPos.y + rectSize.y;
}

int UITryGetObject(UIObject *o, Vec2i pos, UIObject **out)
{
	int isHighlighted = o->Parent && o->Parent->Highlighted == o;
	if (IsInside(pos, o->Pos, o->Size) &&
		o->Type != UITYPE_CONTEXT_MENU)
	{
		*out = o;
		return 1;
	}
	if (o->Type == UITYPE_TAB)
	{
		// only recurse to the chosen child
		if (o->Children.size > 0)
		{
			UIObject **objp = CArrayGet(&o->Children, o->u.Tab.Index);
			if ((!((*objp)->Flags & UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY) ||
				isHighlighted) &&
				(*objp)->IsVisible &&
				UITryGetObject(*objp, Vec2iMinus(pos, o->Pos), out))
			{
				return 1;
			}
		}
	}
	else
	{
		size_t i;
		UIObject **objs = o->Children.data;
		for (i = 0; i < o->Children.size; i++, objs++)
		{
			if ((!((*objs)->Flags & UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY) ||
				isHighlighted) &&
				(*objs)->IsVisible &&
				UITryGetObject(*objs, Vec2iMinus(pos, o->Pos), out))
			{
				return 1;
			}
		}
	}
	return 0;
}

void UITooltipDraw(GraphicsDevice *device, Vec2i pos, const char *s)
{
	Vec2i bgSize = TextGetSize(s);
	pos = Vec2iAdd(pos, Vec2iNew(10, 10));	// add offset
	DrawRectangle(
		device,
		Vec2iAdd(pos, Vec2iScale(Vec2iUnit(), -TOOLTIP_PADDING)),
		Vec2iAdd(bgSize, Vec2iScale(Vec2iUnit(), 2 * TOOLTIP_PADDING)),
		bgColor,
		0);
	DrawTextString(s, device, pos);
}
