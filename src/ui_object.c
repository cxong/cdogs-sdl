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


void UICollectionTerminate(UICollection *c)
{
	size_t i;
	UIObject *objs = c->Objs.data;
	for (i = 0; i < c->Objs.size; i++, objs++)
	{
		UICollectionTerminate(&objs->Children);
		CFREE(objs->Tooltip);
	}
	CArrayTerminate(&c->Objs);
	memset(c, 0, sizeof *c);
}

void UICollectionDraw(UICollection *c, GraphicsDevice *g)
{
	size_t i;
	UIObject *objs;
	if (!c)
	{
		return;
	}
	objs = c->Objs.data;
	for (i = 0; i < c->Objs.size; i++, objs++)
	{
		switch (objs->Type)
		{
		case UITYPE_TEXTBOX:
			{
				int isText = !!objs->u.Textbox.TextLinkFunc;
				char *text = isText ? objs->u.Textbox.TextLinkFunc(
					objs->u.Textbox.TextLinkData) : NULL;
				int isEmptyText = !isText || !text || strlen(text) == 0;
				int isHighlighted = c->Highlighted == objs;
				color_t bracketMask = isHighlighted ? colorRed : colorWhite;
				color_t textMask = isEmptyText ? colorGray : colorWhite;
				Vec2i pos = objs->Pos;
				if (isEmptyText)
				{
					text = objs->u.Textbox.Hint;
				}
				pos = DrawTextCharMasked('\020', g, pos, bracketMask);
				pos = DrawTextStringMaskedWrapped(
					text, g, pos, textMask, objs->Pos.x + objs->Size.x - pos.x);
				pos = DrawTextCharMasked('\021', g, pos, bracketMask);
			}
			break;
		}
		if (c->Highlighted == objs)
		{
			UICollectionDraw(&objs->Children, g);
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

int UITryGetObject(UICollection *c, Vec2i pos, UIObject **out)
{
	size_t i;
	UIObject *objs = c->Objs.data;
	for (i = 0; i < c->Objs.size; i++, objs++)
	{
		if (IsInside(pos, objs->Pos, objs->Size))
		{
			*out = objs;
			return 1;
		}
		if (c->Highlighted == objs &&
			UITryGetObject(&objs->Children, pos, out))
		{
			return 1;
		}
	}
	return 0;
}