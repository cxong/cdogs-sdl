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

#include <cdogs/text.h>


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
	}
	CArrayInit(&o->Children, sizeof o);
	return o;
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
	// do not copy children
	switch (o->Type)
	{
	case UITYPE_LABEL:
		res->u.LabelFunc = o->u.LabelFunc;
		break;
	case UITYPE_TEXTBOX:
		res->u.Textbox = o->u.Textbox;
		break;
	case UITYPE_TAB:
		// do nothing; since we cannot copy children
		break;
	case UITYPE_CUSTOM:
		res->u.CustomDrawFunc = o->u.CustomDrawFunc;
		break;
	}
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
}

void UIObjectDraw(UIObject *o, GraphicsDevice *g)
{
	int isHighlighted;
	if (!o)
	{
		return;
	}
	isHighlighted = UIObjectIsHighlighted(o);
	switch (o->Type)
	{
	case UITYPE_LABEL:
		{
			int isText = !!o->u.LabelFunc;
			const char *text = isText ? o->u.LabelFunc(o, o->Data) : NULL;
			color_t textMask = isHighlighted ? colorRed : colorWhite;
			Vec2i pos = o->Pos;
			if (!o->IsVisible)
			{
				return;
			}
			if (!text)
			{
				break;
			}
			pos = DrawTextStringMaskedWrapped(
				text, g, pos, textMask, o->Pos.x + o->Size.x - pos.x);
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
			Vec2i pos = o->Pos;
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
				pos = DrawTextCharMasked('\020', g, pos, bracketMask);
			}
			pos = DrawTextStringMaskedWrapped(
				text, g, pos, textMask,
				o->Pos.x + o->Size.x - pos.x);
			if (o->u.Textbox.IsEditable)
			{
				pos = DrawTextCharMasked('\021', g, pos, bracketMask);
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
				*labelp, g, o->Pos, textMask, o->Pos.x + o->Size.x - o->Pos.x);
			if (!((*objp)->Flags & UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY) ||
				isHighlighted)
			{
				UIObjectDraw(*objp, g);
			}
		}
		break;
	case UITYPE_CUSTOM:
		o->u.CustomDrawFunc(o, g, o->Data);
		if (!o->IsVisible)
		{
			return;
		}
		break;
	}

	// draw children
	// Note: tab type draws its own children (one)
	if (o->Type != UITYPE_TAB)
	{
		size_t i;
		UIObject **objs = o->Children.data;
		for (i = 0; i < o->Children.size; i++, objs++)
		{
			if (!((*objs)->Flags & UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY) ||
				isHighlighted)
			{
				UIObjectDraw(*objs, g);
			}
		}
	}
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
	if (IsInside(pos, o->Pos, o->Size))
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
				UITryGetObject(*objp, pos, out))
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
				UITryGetObject(*objs, pos, out))
			{
				return 1;
			}
		}
	}
	return 0;
}