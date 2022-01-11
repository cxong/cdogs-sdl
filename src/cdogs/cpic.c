/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2016, 2018-2019 Cong Xu
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
#include "log.h"
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


CPicDrawContext CPicDrawContextNew(void)
{
	CPicDrawContext c;
	c.Dir = DIRECTION_UP;
	c.Offset = svec2i_zero();
	c.Radians = 0;
	c.Scale = svec2_one();
	c.Mask = colorWhite;
	c.Flip = SDL_FLIP_NONE;
	return c;
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

static void LoadNormal(CPic *p, const json_t *node);
static void LoadMaskTint(CPic *p, json_t *node);

void CPicLoadJSON(CPic *p, json_t *node)
{
	char *tmp = GetString(node, "Type");
	p->Type = StrPicType(tmp);
	CFREE(tmp);
	tmp = NULL;
	switch (p->Type)
	{
	case PICTYPE_NORMAL:
		LoadNormal(p, json_find_first_label(node, "Pic")->child);
		break;
	case PICTYPE_DIRECTIONAL:
		LoadStr(&tmp, node, "Sprites");
		if (tmp == NULL)
		{
			LOG(LM_GFX, LL_ERROR, "cannot load sprites");
			goto bail;
		}
		p->u.Sprites = &PicManagerGetSprites(&gPicManager, tmp)->pics;
		CFREE(tmp);
		break;
	case PICTYPE_ANIMATED:	// fallthrough
	case PICTYPE_ANIMATED_RANDOM:
		LoadStr(&tmp, node, "Sprites");
		if (tmp == NULL)
		{
			LOG(LM_GFX, LL_ERROR, "cannot load sprites");
			goto bail;
		}
		p->u.Animated.Sprites =
			&PicManagerGetSprites(&gPicManager, tmp)->pics;
		CFREE(tmp);
		LoadInt(&p->u.Animated.Count, node, "Count");
		LoadInt(&p->u.Animated.TicksPerFrame, node, "TicksPerFrame");
		p->u.Animated.TicksPerFrame = MAX(p->u.Animated.TicksPerFrame, 0);
		break;
	default:
		CASSERT(false, "unknown pic type");
		break;
	}
	LoadMaskTint(p, node);
bail:
	// TODO: return error
	return;
}

void CPicInitNormal(CPic *p, const Pic *pic)
{
	p->Type = PICTYPE_NORMAL;
	p->u.Pic = pic;
	p->Mask = colorWhite;
}
void CPicInitNormalFromName(CPic *p, const char *name)
{
	p->Type = PICTYPE_NORMAL;
	p->u.Pic = PicManagerGetPic(&gPicManager, name);
	p->Mask = colorWhite;
}

void CPicLoadNormal(CPic *p, const json_t *node)
{
	p->Type = PICTYPE_NORMAL;
	LoadNormal(p, node);
	p->Mask = colorWhite;
}

static void LoadNormal(CPic *p, const json_t *node)
{
	char *tmp = json_unescape(node->text);
	p->u.Pic = PicManagerGetPic(&gPicManager, tmp);
	CFREE(tmp);
}

static void LoadMaskTint(CPic *p, json_t *node)
{
	p->Mask = colorWhite;
	if (json_find_first_label(node, "Mask"))
	{
		char *tmp = GetString(node, "Mask");
		p->Mask = StrColor(tmp);
		CFREE(tmp);
	}
	else if (json_find_first_label(node, "Tint"))
	{
		// TODO: create new pic, as tinting does not work correctly using mask
		// Mask only darkens, whereas tinting can change hue without affecting
		// value, for example
		json_t *tint = json_find_first_label(node, "Tint")->child->child;
		HSV hsv;
		hsv.h = atof(tint->text);
		tint = tint->next;
		hsv.s = atof(tint->text);
		tint = tint->next;
		hsv.v = atof(tint->text);
		p->Mask = ColorTint(colorWhite, hsv);
		p->Mask.a = 0x40;
	}
}

bool CPicIsLoaded(const CPic *p)
{
	switch (p->Type)
	{
	case PICTYPE_NORMAL:
		return p->u.Pic != NULL;
	case PICTYPE_DIRECTIONAL:
		return p->u.Sprites != NULL;
	case PICTYPE_ANIMATED:
	case PICTYPE_ANIMATED_RANDOM:
		return p->u.Animated.Sprites != NULL;
	default:
		CASSERT(false, "unknown pic type");
		return false;
	}
}

struct vec2i CPicGetSize(const CPic *p)
{
	const Pic *pic = CPicGetPic(p, 0);
	if (pic == NULL)
	{
		return svec2i_zero();
	}
	return pic->size;
}

void CPicCopyPic(CPic *dest, const CPic *src)
{
	memcpy(dest, src, sizeof *src);
	if (dest->Type == PICTYPE_ANIMATED_RANDOM)
	{
		// initialise frame with a random value
		dest->u.Animated.Frame = rand() % (int)dest->u.Animated.Sprites->size;
	}
}

void CPicUpdate(CPic *p, const int ticks)
{
	switch (p->Type)
	{
	case PICTYPE_ANIMATED:
		{
			p->u.Animated.Count += ticks;
			if (p->u.Animated.TicksPerFrame > 0)
			{
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
		}
		break;
	case PICTYPE_ANIMATED_RANDOM:
		if (p->u.Animated.Count == 0)
		{
			// Initial frame
			p->u.Animated.Frame = rand() % (int)p->u.Animated.Sprites->size;
		}
		p->u.Animated.Count += ticks;
		if (p->u.Animated.TicksPerFrame > 0 &&
			p->u.Animated.Count >= p->u.Animated.TicksPerFrame)
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
const Pic *CPicGetPic(const CPic *p, const int idx)
{
	switch (p->Type)
	{
	case PICTYPE_NORMAL:
		return p->u.Pic;
	case PICTYPE_DIRECTIONAL:
		return p->u.Sprites != NULL ? CArrayGet(p->u.Sprites, idx) : NULL;
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
	const struct vec2i pos, const CPicDrawContext *context)
{
	CPicDrawContext ctx;
	if (context == NULL)
	{
		ctx = CPicDrawContextNew();
		context = &ctx;
	}
	const Pic *pic = CPicGetPic(p, context->Dir);
	if (pic == NULL)
	{
		return;
	}
	const struct vec2i picPos = svec2i_add(pos, context->Offset);
	PicRender(
		pic, g->gameWindow.renderer, picPos, ColorMult(p->Mask, context->Mask),
		context->Radians,
		context->Scale, context->Flip, Rect2iZero());
}
