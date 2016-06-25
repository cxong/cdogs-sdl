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
#include "cpic.h"

#include "blit.h"
#include "json_utils.h"
#include "palette.h"
#include "pic_manager.h"
#include "utils.h"


PicType StrPicType(const char *s)
{
	S2T(PICTYPE_NORMAL, "Normal");
	S2T(PICTYPE_DIRECTIONAL, "Directional");
	S2T(PICTYPE_ANIMATED, "Animated");
	S2T(PICTYPE_ANIMATED_RANDOM, "AnimatedRandom");
	CASSERT(false, "unknown pic type");
	return PICTYPE_NORMAL;
}


void PicFromPicPaletted(Pic *pic, const PicPaletted *picP)
{
	pic->size = Vec2iNew(picP->w, picP->h);
	pic->offset = Vec2iZero();
	CMALLOC(pic->Data, pic->size.x * pic->size.y * sizeof *pic->Data);
	for (int i = 0; i < pic->size.x * pic->size.y; i++)
	{
		unsigned char palette = *(picP->data + i);
		pic->Data[i] = COLOR2PIXEL(PaletteToColor(palette));
		// Special case: if the palette colour is 0, it's transparent
		if (palette == 0)
		{
			pic->Data[i] = 0;
		}
	}
}


void NamedPicFree(NamedPic *n)
{
	PicFree(&n->pic);
	CFREE(n->name);
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

void CPicLoadJSON(CPic *p, json_t *node)
{
	char *tmp = GetString(node, "Type");
	p->Type = StrPicType(tmp);
	CFREE(tmp);
	bool picLoaded = false;
	switch (p->Type)
	{
	case PICTYPE_NORMAL:
		tmp = GetString(node, "Pic");
		p->u.Pic = PicManagerGetPic(&gPicManager, tmp);
		CFREE(tmp);
		picLoaded = p->u.Pic != NULL;
		break;
	case PICTYPE_DIRECTIONAL:
		tmp = GetString(node, "Sprites");
		p->u.Sprites = &PicManagerGetSprites(&gPicManager, tmp)->pics;
		CFREE(tmp);
		picLoaded = p->u.Sprites != NULL;
		break;
	case PICTYPE_ANIMATED:	// fallthrough
	case PICTYPE_ANIMATED_RANDOM:
		tmp = GetString(node, "Sprites");
		p->u.Animated.Sprites =
			&PicManagerGetSprites(&gPicManager, tmp)->pics;
		CFREE(tmp);
		LoadInt(&p->u.Animated.Count, node, "Count");
		LoadInt(&p->u.Animated.TicksPerFrame, node, "TicksPerFrame");
		// Set safe default ticks per frame 1;
		// if 0 then this leads to infinite loop when animating
		p->u.Animated.TicksPerFrame = MAX(p->u.Animated.TicksPerFrame, 1);
		picLoaded = p->u.Animated.Sprites != NULL;
		break;
	default:
		CASSERT(false, "unknown pic type");
		break;
	}
	p->UseMask = true;
	p->u1.Mask = colorWhite;
	if (json_find_first_label(node, "Mask"))
	{
		tmp = GetString(node, "Mask");
		p->u1.Mask = StrColor(tmp);
		CFREE(tmp);
	}
	else if (json_find_first_label(node, "Tint"))
	{
		p->UseMask = false;
		json_t *tint = json_find_first_label(node, "Tint")->child->child;
		p->u1.Tint.h = atof(tint->text);
		tint = tint->next;
		p->u1.Tint.s = atof(tint->text);
		tint = tint->next;
		p->u1.Tint.v = atof(tint->text);
	}
}
void CPicUpdate(CPic *p, const int ticks)
{
	switch (p->Type)
	{
	case PICTYPE_ANIMATED:
		{
			p->u.Animated.Count += ticks;
			CASSERT(p->u.Animated.TicksPerFrame > 0, "0 ticks per frame");
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
		if (p->u.Animated.Frame < 0 ||
			p->u.Animated.Frame >= (int)p->u.Animated.Sprites->size)
		{
			return NULL;
		}
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
	if (pic == NULL)
	{
		return;
	}
	const Vec2i picPos = Vec2iAdd(pos, context->Offset);
	if (p->UseMask)
	{
		BlitMasked(g, pic, picPos, p->u1.Mask, true);
	}
	else
	{
		BlitBackground(g, pic, picPos, &p->u1.Tint, true);
	}
}
