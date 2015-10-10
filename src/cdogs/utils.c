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

    Copyright (c) 2013-2015, Cong Xu
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
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#include "utils.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <string.h>

#include <tinydir/tinydir.h>

#include "events.h"
#include "joystick.h"
#include "sys_config.h"

bool debug = false;
int debug_level = D_NORMAL;

bool gTrue = true;
bool gFalse = false;

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

// Note: includes trailing slash
void PathGetDirname(char *buf, const char *path)
{
	const char *basename = PathGetBasename(path);
	if (basename == path)
	{
		strcpy(buf, "");
	}
	else
	{
		strncpy(buf, path, basename - path);
		buf[basename - path] = '\0';
	}
}
const char *PathGetBasename(const char *path)
{
	const char *fslash = strrchr(path, '/');
	const char *bslash = strrchr(path, '\\');
	const char *slash =
		fslash ? (bslash ? MAX(fslash, bslash) : fslash) : bslash;
	if (slash == NULL)
	{
		// no slashes found, simply return path
		return path;
	}
	else
	{
		return slash + 1;
	}
}
void PathGetWithoutExtension(char *buf, const char *path)
{
	const char *dot = strrchr(path, '.');
	if (dot)
	{
		strncpy(buf, path, dot - path);
		buf[dot - path] = '\0';
	}
	else
	{
		strcpy(buf, path);
	}
}
void PathGetBasenameWithoutExtension(char *buf, const char *path)
{
	const char *basename = PathGetBasename(path);
	PathGetWithoutExtension(buf, basename);
}

#ifdef _WIN32
#include "sys_config.h"
#define realpath(src, dst) _fullpath(dst, src, CDOGS_PATH_MAX)
#endif
void RealPath(const char *src, char *dest)
{
#ifndef _WIN32
	// realpath will fail if the file does not exist; if this is the
	// case then create a file temporarily, return
	// the canonical path, then remove the temporary file.
	tinydir_file file;
	const bool exists = tinydir_file_open(&file, src) == 0;
	if (!exists)
	{
		// First, convert slashes
		char srcBuf[CDOGS_PATH_MAX];
		strcpy(srcBuf, src);
		for (char *c = srcBuf; *c != '\0'; c++)
		{
			if (*c == '\\')
			{
				*c = '/';
			}
		}
		FILE *f = fopen(srcBuf, "ab+");
		CASSERT(f != NULL, "internal error: cannot create temp file");
		if (f != NULL)
		{
			fclose(f);
		}
	}
#endif
	char *res = realpath(src, dest);
#ifndef _WIN32
	if (!exists)
	{
		// delete the temporary file we created
		const int res2 = remove(src);
		if (res2 != 0)
		{
			fprintf(stderr, "Internal error: cannot delete\n");
		}
	}
#endif
	if (!res)
	{
		fprintf(stderr, "Cannot resolve relative path %s: %s\n",
			src, strerror(errno));
		// Default to relative path
		strcpy(dest, src);
	}
	// Convert \'s to /'s (for PhysFS's benefit)
	for (char *c = dest; *c != '\0'; c++)
	{
		if (*c == '\\')
		{
			*c = '/';
		}
	}
}
// Convert an absolute path to a relative path
// e.g. /a/path/from/here, /a/path/to -> ../to
void RelPath(char *buf, const char *to, const char *from)
{
	// Make sure both to/from paths were generated using RealPath,
	// so that common substring means they share part of their paths,
	// and not due to alternate renderings of the same path like
	// different path separator chars, or relative paths.
	char toBuf[CDOGS_PATH_MAX];
	char fromBuf[CDOGS_PATH_MAX];
	RealPath(to, toBuf);
	RealPath(from, fromBuf);
	
	// First, find common string prefix
	const char *t = toBuf;
	const char *f = fromBuf;
	const char *tSlash = t;
	const char *fSlash = f;
	while (*t && *f)
	{
		if (*t != *f)
		{
			break;
		}
		if (*t == '/')
		{
			tSlash = t;
			fSlash = f;
		}
		t++;
		f++;
	}
	// e.g.: tSlash = "/to", fSlash = "/from/here"

	// Check if one is a complete substring of the other
	if (!*f)
	{
		tSlash = t;
		fSlash = f;
	}

	// For every folder in "from", add "..", and finally add "to"
	strcpy(buf, "");
	while (*fSlash)
	{
		if (*fSlash == '/')
		{
			strcat(buf, "../");
		}
		fSlash++;
	}
	tSlash++;
	strcat(buf, tSlash);
}
void RelPathFromCWD(char *buf, const char *to)
{
	if (to == NULL || strlen(to) == 0)
	{
		return;
	}
	char cwd[CDOGS_PATH_MAX];
	if (getcwd(cwd, CDOGS_PATH_MAX) == NULL)
	{
		fprintf(stderr, "Error getting CWD; %s\n", strerror(errno));
		strcpy(buf, to);
	}
	else
	{
		RelPath(buf, to, cwd);
	}
}

void GetDataFilePath(char *buf, const char *path)
{
	char relbuf[CDOGS_PATH_MAX];
	sprintf(relbuf, "%s%s", CDOGS_DATA_DIR, path);
	RealPath(relbuf, buf);
}

double Round(double x)
{
	return floor(x + 0.5);
}

double ToDegrees(double radians)
{
	return radians * 180.0 / PI;
}
double ToRadians(double degrees)
{
	return degrees * PI / 180.0;
}

const char *InputDeviceName(const int d, const int deviceIndex)
{
	switch (d)
	{
	case INPUT_DEVICE_KEYBOARD:
		return "Keyboard";
	case INPUT_DEVICE_MOUSE:
		return "Mouse";
	case INPUT_DEVICE_JOYSTICK:
		return JoyName(deviceIndex);
	case INPUT_DEVICE_AI:
		return "AI";
	default:
		return "";
	}
}

const char *AllyCollisionStr(int a)
{
	switch (a)
	{
		T2S(ALLYCOLLISION_NORMAL, "Normal");
		T2S(ALLYCOLLISION_REPEL, "Repel");
		T2S(ALLYCOLLISION_NONE, "None");
	default:
		return "";
	}
}
int StrAllyCollision(const char *s)
{
	S2T(ALLYCOLLISION_NORMAL, "Normal");
	S2T(ALLYCOLLISION_REPEL, "Repel");
	S2T(ALLYCOLLISION_NONE, "None");
	return ALLYCOLLISION_NORMAL;
}

char *IntStr(int i)
{
	static char buf[32];
	sprintf(buf, "%d", i);
	return buf;
}
char *PercentStr(int p)
{
	static char buf[32];
	sprintf(buf, "%d%%", p);
	return buf;
}
char *Div8Str(int i)
{
	static char buf[8];
	sprintf(buf, "%d", i/8);
	return buf;
}
void CamelToTitle(char *buf, const char *src)
{
	const char *first = src;
#define IS_UPPER(_x) ((_x) >= 'A' && (_x) <= 'Z')
	while (*src)
	{
		// Word boundaries marked by capital letters, as long as:
		// - It's not the first letter, and
		// - The previous letter is lower case, or
		// - The next letter is lower case
		if (IS_UPPER(*src) &&
			src != first &&
			(!IS_UPPER(*(src - 1)) || (*(src + 1) && !IS_UPPER(*(src + 1)))))
		{
			*buf++ = ' ';
		}
		*buf++ = *src++;
	}
	*buf = '\0';
}
