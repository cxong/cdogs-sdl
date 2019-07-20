/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2019 Cong Xu
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
#include "nine_slice.h"

#include "log.h"
#include "texture.h"


void Draw9Slice(
	GraphicsDevice *g, const Pic *pic,
	const Rect2i target,
	const int top, const int right, const int bottom, const int left,
	const bool repeat, const color_t mask, const SDL_RendererFlip flip)
{
	const int srcX[] = {0, left, pic->size.x - right};
	const int srcY[] = {0, top, pic->size.y - bottom};
	const int srcW[] = {left, pic->size.x - right - left, right};
	const int srcH[] = {top, pic->size.y - bottom - top, bottom};
	const int dstX[] = {
		target.Pos.x,
		target.Pos.x + left,
		target.Pos.x + target.Size.x - right,
		target.Pos.x + target.Size.x
	};
	const int dstY[] = {
		target.Pos.y,
		target.Pos.y + top,
		target.Pos.y + target.Size.y - bottom,
		target.Pos.y + target.Size.y
	};
	const int dstW[] = {left, target.Size.x - right - left, right};
	const int dstH[] = {top, target.Size.y - bottom - top, bottom};
	Rect2i src;
	Rect2i dst;
	for (int i = 0; i < 3; i++)
	{
		src.Pos.x = srcX[i];
		src.Size.x = srcW[i];
		dst.Size.x = repeat ? srcW[i] : dstW[i];
		for (dst.Pos.x = dstX[i];
			dst.Pos.x < dstX[i + 1];
			dst.Pos.x += dst.Size.x)
		{
			if (dst.Pos.x + dst.Size.x > dstX[i + 1])
			{
				src.Size.x = dst.Size.x = dstX[i + 1] - dst.Pos.x;
			}
			for (int j = 0; j < 3; j++)
			{
				src.Pos.y = srcY[j];
				src.Size.y = srcH[j];
				dst.Size.y = repeat ? srcH[j] : dstH[j];
				for (dst.Pos.y = dstY[j];
					dst.Pos.y < dstY[j + 1];
					dst.Pos.y += dst.Size.y)
				{
					if (dst.Pos.y + dst.Size.y > dstY[j + 1])
					{
						src.Size.y = dst.Size.y = dstY[j + 1] - dst.Pos.y;
					}
					TextureRender(
						pic->Tex, g->gameWindow.renderer, src, dst, mask, 0,
						flip);
				}
			}
		}
	}
}
