/*
	Copyright (c) 2014-2015, 2019-2022 Cong Xu
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
#pragma once

#include <SDL_surface.h>

#include "c_array.h"
#include "vector.h"

#define ARROW_LEFT "\x11"
#define ARROW_RIGHT "\x10"
#define ARROW_UP "\x1e"
#define ARROW_DOWN "\x1f"

// Defines interfaces for bitmap fonts

typedef struct
{
	struct vec2i Size;
	int Stride;
	struct
	{
		int Left;
		int Top;
		int Right;
		int Bottom;
	} Padding;
	struct vec2i Gap;
	CArray Chars; // of Pic
} Font;

typedef enum
{
	ALIGN_START = 0,
	ALIGN_CENTER,
	ALIGN_END
} FontAlign;
typedef struct
{
	FontAlign HAlign;
	FontAlign VAlign;
	struct vec2i Area;
	struct vec2i Pad;
	color_t Mask;
} FontOpts;

extern Font gFont;

FontOpts FontOptsNew(void);

void FontLoad(
	Font *f, const char *imgPath, const bool isProportional,
	const struct vec2i spaceSize);
void FontTerminate(Font *f);

int FontW(const char c);
int FontH(void);
struct vec2i FontChSize(const char c);
int FontStrW(const char *s);
int FontSubstrW(const char *s, int len);
int FontStrH(const char *s);
struct vec2i FontStrSize(const char *s);
int FontStrNumLines(const char *s);

// Returns updated cursor position
struct vec2i FontCh(const char c, const struct vec2i pos);
struct vec2i FontChMask(
	const char c, const struct vec2i pos, const color_t mask);
struct vec2i FontStr(const char *s, struct vec2i pos);
struct vec2i FontStrMask(const char *s, struct vec2i pos, const color_t mask);
struct vec2i FontStrMaskWrap(
	const char *s, struct vec2i pos, color_t mask, const int width);
void FontStrOpt(const char *s, struct vec2i pos, const FontOpts opts);
void FontStrCenter(const char *s);

void FontSplitLines(const char *text, char *buf, const int width);

struct vec2i Vec2iAligned(
	const struct vec2i v, const struct vec2i size, const FontAlign hAlign,
	const FontAlign vAlign, const struct vec2i area);
