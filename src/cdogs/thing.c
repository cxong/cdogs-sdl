/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

    Copyright (c) 2013-2014, 2016-2018 Cong Xu
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
#include "thing.h"

#include "actors.h"
#include "objs.h"
#include "pickup.h"
#include "tile.h"


bool IsThingInsideTile(const Thing *i, const struct vec2i tilePos)
{
	return
		i->Pos.x - i->size.x / 2 >= tilePos.x * TILE_WIDTH &&
		i->Pos.x + i->size.x / 2 < (tilePos.x + 1) * TILE_WIDTH &&
		i->Pos.y - i->size.y / 2 >= tilePos.y * TILE_HEIGHT &&
		i->Pos.y + i->size.y / 2 < (tilePos.y + 1) * TILE_HEIGHT;
}


void ThingInit(
	Thing *t, const int id, const ThingKind kind, const struct vec2i size,
	const int flags)
{
	memset(t, 0, sizeof *t);
	t->id = id;
	t->kind = kind;
	t->size = size;
	t->flags = flags;
	// Ininitalise pos
	t->Pos = svec2(-1, -1);
}

void ThingUpdate(Thing *t, const int ticks)
{
	t->SoundLock = MAX(0, t->SoundLock - ticks);
	CPicUpdate(&t->CPic, ticks);
}


Thing *ThingIdGetThing(const ThingId *tid)
{
	Thing *ti = NULL;
	switch (tid->Kind)
	{
	case KIND_CHARACTER:
		ti = &((TActor *)CArrayGet(&gActors, tid->Id))->thing;
		break;
	case KIND_PARTICLE:
		ti = &((Particle *)CArrayGet(&gParticles, tid->Id))->thing;
		break;
	case KIND_MOBILEOBJECT:
		ti = &((TMobileObject *)CArrayGet(
			&gMobObjs, tid->Id))->thing;
		break;
	case KIND_OBJECT:
		ti = &((TObject *)CArrayGet(&gObjs, tid->Id))->thing;
		break;
	case KIND_PICKUP:
		ti = &((Pickup *)CArrayGet(&gPickups, tid->Id))->thing;
		break;
	default:
		CASSERT(false, "unknown tile item to get");
		break;
	}
	return ti;
}

bool ThingDrawLast(const Thing *t)
{
	return t->flags & THING_DRAW_LAST;
}
