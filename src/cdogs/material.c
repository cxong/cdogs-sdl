/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2021 Cong Xu
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
#include "material.h"

void MatGetFootstepSound(const CharacterClass *c, const TileClass *tc, char *out)
{
	if (c->Footsteps)
	{
		sprintf(out, "footsteps/%s", c->Footsteps);
		return;
	}

	// Determine material type based on tile
	if (tc != NULL && tc->Type == TILE_CLASS_FLOOR && tc->Style != NULL)
	{
		if (StrStartsWith(tc->Style, "checker") || StrStartsWith(tc->Style, "tile"))
		{
			strcpy(out, "footsteps/tile");
			return;
		}
		if (StrStartsWith(tc->Style, "dirt"))
		{
			strcpy(out, "footsteps/grass");
			return;
		}
		if (StrStartsWith(tc->Style, "water"))
		{
			strcpy(out, "footsteps/water");
			return;
		}
		if (StrStartsWith(tc->Style, "wood"))
		{
			strcpy(out, "footsteps/wood");
			return;
		}
	}
	
	// Default footstep sound
	strcpy(out, "footsteps/boots");
}
