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
#include "pic.h"

#include "blit.h"
#include "palette.h"
#include "utils.h"

Pic picNone = { { 0, 0 }, { 0, 0 }, NULL };

void PicFromPicPaletted(GraphicsDevice *g, Pic *pic, PicPaletted *picP)
{
	int i;
	pic->size = Vec2iNew(picP->w, picP->h);
	pic->offset = Vec2iZero();
	CMALLOC(pic->Data, pic->size.x * pic->size.y * sizeof *pic->Data);
	for (i = 0; i < pic->size.x * pic->size.y; i++)
	{
		unsigned char palette = *(picP->data + i);
		pic->Data[i] = PixelFromColor(g, PaletteToColor(palette));
		// Special case: if the palette colour is 0, it's transparent
		if (palette == 0)
		{
			pic->Data[i] = 0;
		}
	}
}

void PicCopy(Pic *dst, const Pic *src)
{
	dst->size = src->size;
	dst->offset = src->offset;
	size_t size = dst->size.x * dst->size.y * sizeof *dst->Data;
	CMALLOC(dst->Data, size);
	memcpy(dst->Data, src->Data, size);
}

void PicFree(Pic *pic)
{
	CFREE(pic->Data);
}

int PicIsNotNone(Pic *pic)
{
	return pic->size.x > 0 && pic->size.y > 0 && pic->Data != NULL;
}


void NamedSpritesInit(NamedSprites *ns, const char *name)
{
	CSTRDUP(ns->name, name);
	CArrayInit(&ns->pics, sizeof(Pic));
}
void NamedSpritesFree(NamedSprites *ns)
{
	if (ns == NULL)
	{
		return;
	}
	CFREE(ns->name);
	for (int i = 0; i < (int)ns->pics.size; i++)
	{
		PicFree(CArrayGet(&ns->pics, i));
	}
	CArrayTerminate(&ns->pics);
}

void CPicUpdate(CPic *p, const int ticks)
{
	switch (p->Type)
	{
	case PICTYPE_ANIMATED:
		{
			p->u.Animated.Count += ticks;
			while (p->u.Animated.Count >= p->u.Animated.TicksPerFrame)
			{
				p->u.Animated.Frame++;
				p->u.Animated.Count -= p->u.Animated.TicksPerFrame;
			}
			while (p->u.Animated.Frame >= (int)p->u.Animated.Sprites->size)
			{
				p->u.Animated.Frame -= (int)p->u.Animated.Sprites->size;
			}
		}
		break;
	case PICTYPE_ANIMATED_RANDOM:
		p->u.Animated.Count += ticks;
		if (p->u.Animated.Count >= p->u.Animated.TicksPerFrame)
		{
			p->u.Animated.Frame = rand() % (int)p->u.Animated.Sprites->size;
			p->u.Animated.Count = 0;
		}
		break;
	default:
		// Do nothing
		break;
	}
}
const Pic *CPicGetPic(const CPic *p, direction_e d)
{
	switch (p->Type)
	{
	case PICTYPE_NORMAL:
		return p->u.Pic;
	case PICTYPE_DIRECTIONAL:
		return CArrayGet(p->u.Sprites, d);
	case PICTYPE_ANIMATED:
	case PICTYPE_ANIMATED_RANDOM:
		return CArrayGet(p->u.Animated.Sprites, p->u.Animated.Frame);
	default:
		CASSERT(false, "unknown pic type");
		return NULL;
	}
}
void CPicDraw(
	GraphicsDevice *g, const CPic *p,
	const Vec2i pos, const CPicDrawContext *context)
{
	const Pic *pic = CPicGetPic(p, context->Dir);
	const Vec2i picPos = Vec2iAdd(pos, context->Offset);
	if (p->UseMask)
	{
		// TODO: RGB mask
		Blit(g, pic, picPos);
	}
	else
	{
		BlitBackground(g, pic, picPos, &p->Tint, true);
	}
}
