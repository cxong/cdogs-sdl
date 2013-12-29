/*
 C-Dogs SDL
 A port of the legendary (and fun) action/arcade cdogs.
 
 Copyright (c) 2013, Cong Xu
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

#include <string.h>

#include <cdogs/text.h>


UIObject *UIObjectCreate(int id, Vec2i pos, Vec2i size)
{
	UIObject *o;
	CCALLOC(o, sizeof *o);
	o->Id = id;
	o->Pos = pos;
	o->Size = size;
	CArrayInit(&o->Children, sizeof o);
	return o;
}

UIObject *UIObjectCopy(UIObject *o)
{
	UIObject *res = UIObjectCreate(o->Id, o->Pos, o->Size);
	res->Type = o->Type;
	res->Id2 = o->Id2;
	res->Flags = o->Flags;
	res->Tooltip = o->Tooltip;
	res->Parent = o->Parent;
	// do not copy children
	switch (o->Type)
	{
	case UITYPE_TEXTBOX:
		res->u.Textbox = o->u.Textbox;
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
	switch (o->Type)
	{
	case UITYPE_TEXTBOX:
		CFREE(o->u.Textbox.Hint);
		break;
	}
	CFREE(o);
}

void UIObjectAddChild(UIObject *o, UIObject *c)
{
	CArrayPushBack(&o->Children, &c);
	c->Parent = o;
}

void UIObjectHighlight(UIObject *o)
{
	if (o->Parent)
	{
		o->Parent->Highlighted = o;
		UIObjectHighlight(o->Parent);
	}
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
	size_t i;
	UIObject **objs;
	int isHighlighted;
	if (!o)
	{
		return;
	}
	isHighlighted = o->Parent && o->Parent->Highlighted == o;
	switch (o->Type)
	{
	case UITYPE_TEXTBOX:
		{
			int isText = !!o->u.Textbox.TextLinkFunc;
			char *text = isText ? o->u.Textbox.TextLinkFunc(
			o->u.Textbox.TextLinkData) : NULL;
			int isEmptyText = !isText || !text || strlen(text) == 0;
			color_t bracketMask = isHighlighted ? colorRed : colorWhite;
			color_t textMask = isEmptyText ? colorGray : colorWhite;
			Vec2i pos = o->Pos;
			if (isEmptyText)
			{
				text = o->u.Textbox.Hint;
			}
			pos = DrawTextCharMasked('\020', g, pos, bracketMask);
			pos = DrawTextStringMaskedWrapped(
				text, g, pos, textMask,
				o->Pos.x + o->Size.x - pos.x);
			pos = DrawTextCharMasked('\021', g, pos, bracketMask);
		}
		break;
	}
	objs = o->Children.data;
	for (i = 0; i < o->Children.size; i++, objs++)
	{
		if (!((*objs)->Flags & UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY) ||
			isHighlighted)
		{
			UIObjectDraw(*objs, g);
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
	size_t i;
	UIObject **objs = o->Children.data;
	int isHighlighted = o->Parent && o->Parent->Highlighted == o;
	if (IsInside(pos, o->Pos, o->Size))
	{
		*out = o;
		return 1;
	}
	for (i = 0; i < o->Children.size; i++, objs++)
	{
		if ((!((*objs)->Flags & UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY) ||
			isHighlighted) &&
			UITryGetObject(*objs, pos, out))
		{
			return 1;
		}
	}
	return 0;
}