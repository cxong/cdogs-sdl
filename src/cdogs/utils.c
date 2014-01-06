/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003-2007 Lucas Martin-King

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

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
#include "utils.h"

#include <assert.h>
#include <math.h>
#include <string.h>

int debug = 0;
int debug_level = D_NORMAL;

int MemIsZero(const void *buf, size_t len)
{
	const char *bytes = (const char *)buf;
	size_t i;
	for (i = 0; i < len; i++)
	{
		if (bytes[i])
		{
			return 0;
		}
	}
	return 1;
}

// From answer by ThiefMaster
// http://stackoverflow.com/a/5309508/2038264
// License: http://creativecommons.org/licenses/by-sa/3.0/
// Author profile: http://stackoverflow.com/users/298479/thiefmaster
const char *StrGetFileExt(const char *filename)
{
	const char *dot = strrchr(filename, '.');
	if (!dot || dot == filename)
	{
		return "";
	}
	return dot + 1;
}

double Round(double x)
{
	return floor(x + 0.5);
}

double ToDegrees(double radians)
{
	return radians * 180.0 / PI;
}

const char *ObjectiveTypeStr(ObjectiveType t)
{
	switch (t)
	{
	case OBJECTIVE_KILL:
		return "Kill";
	case OBJECTIVE_COLLECT:
		return "Collect";
	case OBJECTIVE_DESTROY:
		return "Destroy";
	case OBJECTIVE_RESCUE:
		return "Rescue";
	case OBJECTIVE_INVESTIGATE:
		return "Explore";
	default:
		return "";
	}
}
ObjectiveType StrObjectiveType(const char *s)
{
	if (strcmp(s, "Kill") == 0)
	{
		return OBJECTIVE_KILL;
	}
	else if (strcmp(s, "Collect") == 0)
	{
		return OBJECTIVE_COLLECT;
	}
	else if (strcmp(s, "Destroy") == 0)
	{
		return OBJECTIVE_DESTROY;
	}
	else if (strcmp(s, "Rescue") == 0)
	{
		return OBJECTIVE_RESCUE;
	}
	else if (strcmp(s, "Explore") == 0)
	{
		return OBJECTIVE_INVESTIGATE;
	}
	assert(0 && "unknown objective name");
	return OBJECTIVE_KILL;
}
