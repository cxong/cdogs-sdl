/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014, Cong Xu
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
#include "mission_convert.h"

#include <assert.h>


void MissionConvertToType(Mission *m, Map *map, MapType type)
{
	memset(&m->u, 0, sizeof m->u);
	switch (type)
	{
	case MAPTYPE_STATIC:
		{
			Vec2i v;
			// Take all the tiles from the current map
			// and save them in the static map
			CArrayInit(&m->u.StaticTiles, sizeof(unsigned short));
			for (v.y = 0; v.y < m->Size.y; v.y++)
			{
				for (v.x = 0; v.x < m->Size.x; v.x++)
				{
					Vec2i mapPos = Vec2iNew(
						v.x + (XMAX - m->Size.x) / 2,
						v.y + (YMAX - m->Size.y) / 2);
					unsigned short tile = IMapGet(map, mapPos);
					CArrayPushBack(&m->u.StaticTiles, &tile);
				}
			}
		}
		break;
	}
	m->Type = type;
}

void MissionSetTile(Mission *m, Vec2i pos, unsigned short tile)
{
	int index = pos.y * m->Size.x + pos.x;
	assert(m->Type == MAPTYPE_STATIC && "cannot set tile for map type");
	*(unsigned short *)CArrayGet(&m->u.StaticTiles, index) = tile;
}
