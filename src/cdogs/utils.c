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

	Copyright (c) 2013-2017, 2019-2021 Cong Xu
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
#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include "utils.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <tinydir/tinydir.h>

#include "events.h"
#include "joystick.h"
#include "sys_config.h"

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
static bool IsAbsolutePath(const char *path);
void RealPath(const char *src, char *dest)
{
	char *res;
#ifndef _WIN32
	// realpath will fail if the file does not exist; if this is the
	// case then resolve the path ourselves
	tinydir_file file;
	const bool exists = tinydir_file_open(&file, src) == 0;
	if (!exists)
	{
		char srcBuf[CDOGS_PATH_MAX];
		// First, convert slashes
		srcBuf[0] = '\0';
		strncat(srcBuf, src, CDOGS_PATH_MAX - 1);
		for (char *c = srcBuf; *c != '\0'; c++)
		{
			if (*c == '\\')
			{
				*c = '/';
			}
		}

		// Then, add on the CWD if the path is not absolute
		char resolveBuf[CDOGS_PATH_MAX];
		resolveBuf[0] = '\0';
		if (!IsAbsolutePath(srcBuf))
		{
			CDogsGetCWD(resolveBuf);
			strcat(resolveBuf, "/");
		}
		// Add the path
		strcat(resolveBuf, srcBuf);

		// Finally, resolve the path one level at a time, ignoring '//'s, '.'s
		// and resolving '..'s to the parent level
		char *cOut = dest;
		char cLast = '\0';
		for (const char *c = resolveBuf; *c != '\0'; c++)
		{
			if (*c == '.' && (cLast == '.' || cLast == '/'))
			{
				if (cLast == '.')
				{
					// '..' parent dir
					// Rewind the out ptr to the last path separator
					if (cOut > dest + 1)
					{
						// Skip past the last slash
						cOut -= 2;
						while (*cOut != '/' && cOut > dest)
						{
							cOut--;
						}
						// Go back in front of the slash
						if (*cOut == '/')
						{
							cOut++;
						}
					}
				}
				else if (cLast == '/')
				{
					// ignore for now
				}
			}
			else if (*c == '/' && (cLast == '/' || cLast == '.'))
			{
				// Double slash; ignore
			}
			else
			{
				*cOut = *c;
				cOut++;
			}
			cLast = *c;
		}
		// Write terminating char
		*cOut = '\0';
		res = dest;
	}
	else
#endif
		res = realpath(src, dest);
	if (!res)
	{
		fprintf(
			stderr, "Cannot resolve relative path %s: %s\n", src,
			strerror(errno));
		// Default to relative path
		strcpy(dest, src);
	}
	// Convert \'s to /'s (for consistency)
	for (char *c = dest; *c != '\0'; c++)
	{
		if (*c == '\\')
		{
			*c = '/';
		}
#ifdef _WIN32
		else
		{
			*c = (char)toupper(*c);
		}
#endif
	}
}

// Convert an absolute path to a relative path
// e.g. /a/path/from/here, /a/path/to -> ../to
static void TrimSlashes(char *s);
void RelPath(char *buf, const char *to, const char *from)
{
#ifdef _WIN32
	// If path is on a different Windows drive, just return the path
	if (strlen(from) >= 2 && strlen(to) >= 2 && IsAbsolutePath(from) &&
		IsAbsolutePath(to) && toupper(from[0]) != toupper(to[0]))
	{
		strcpy(buf, to);
		return;
	}
#endif
	// Make sure both to/from paths were generated using RealPath,
	// so that common substring means they share part of their paths,
	// and not due to alternate renderings of the same path like
	// different path separator chars, or relative paths.
	char toBuf[CDOGS_PATH_MAX];
	char fromBuf[CDOGS_PATH_MAX];
	RealPath(to, toBuf);
	RealPath(from, fromBuf);

	// Remove trailing slashes
	TrimSlashes(toBuf);
	TrimSlashes(fromBuf);

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
	if (*tSlash == '/')
	{
		tSlash++;
	}
	strcat(buf, tSlash);
}
static void TrimSlashes(char *s)
{
	char *end = s + strlen(s) - 1;
	while (end > s && *end == '/')
	{
		end--;
	}
	*(end + 1) = '\0';
}

void FixPathSeparator(char *dst, const char *src)
{
	const char *s = src;
	char *d = dst;
#ifdef _WIN32
#define SEP '\\'
#define SEP_OTHER '/'
#else
#define SEP '/'
#define SEP_OTHER '\\'
#endif
	while (*s != '\0')
	{
		if (*s == SEP_OTHER)
			*d = SEP;
		else
			*d = *s;
		d++;
		s++;
	}
#undef SEP
	*d = '\0';
}

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
char *CDogsGetCWD(char *buf)
{
#ifdef __APPLE__
	uint32_t size = CDOGS_PATH_MAX;
	if (_NSGetExecutablePath(buf, &size))
	{
		return NULL;
	}
	// This gives us the executable path; find the dirname
	*strrchr(buf, '/') = '\0';
	return buf;
#else
	return getcwd(buf, CDOGS_PATH_MAX);
#endif
}
void RelPathFromCWD(char *buf, const char *to)
{
	if (to == NULL || strlen(to) == 0)
	{
		return;
	}
	char cwd[CDOGS_PATH_MAX];
	CASSERT(CDogsGetCWD(cwd) != NULL, "error");
	if (CDogsGetCWD(cwd) == NULL)
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
	if (IsAbsolutePath(path))
	{
		strcpy(buf, path);
		return;
	}
	char relbuf[CDOGS_PATH_MAX];
	// Don't bother prepending CWD if data dir already an absolute path
	if (IsAbsolutePath(CDOGS_DATA_DIR))
	{
		sprintf(relbuf, "%s%s", CDOGS_DATA_DIR, path);
	}
	else
	{
		char cwd[CDOGS_PATH_MAX];
		if (CDogsGetCWD(cwd) == NULL)
		{
			fprintf(stderr, "Error getting CWD; %s\n", strerror(errno));
			strcpy(cwd, "");
		}
		sprintf(relbuf, "%s/%s%s", cwd, CDOGS_DATA_DIR, path);
	}
	RealPath(relbuf, buf);
}

static bool IsAbsolutePath(const char *path)
{
#ifdef _WIN32
	return strlen(path) > 1 && path[1] == ':';
#else
	return path[0] == '/';
#endif
}

double Round(double x)
{
	return floor(x + 0.5);
}

double ToDegrees(double radians)
{
	return radians * 180.0 / MPI;
}

struct vec2 CalcClosestPointOnLineSegmentToPoint(
	const struct vec2 l1, const struct vec2 l2, const struct vec2 p)
{
	// Using parametric representation, line l1->l2 is
	// P(t) = l1 + t(l2 - l1)
	// Projection of point p on line is
	// t = ((p.x - l1.x)(l2.x - l1.x) + (p.y - l1.y)(l2.y - l1.y)) / ||l2 -
	// l1||^2
	const float lineDistanceSquared = svec2_distance_squared(l1, l2);
	// Early exit since same point means 0 distance, and div by 0
	if (lineDistanceSquared == 0)
	{
		return l1;
	}
	const float numerator =
		(p.x - l1.x) * (l2.x - l1.x) + (p.y - l1.y) * (l2.y - l1.y);
	const float t = CLAMP(numerator / lineDistanceSquared, 0, 1);
	const struct vec2 closestPoint =
		svec2(l1.x + t * (l2.x - l1.x), l1.y + t * (l2.y - l1.y));
	return closestPoint;
}

const char *InputDeviceName(const int d, const int deviceIndex)
{
	switch (d)
	{
	case INPUT_DEVICE_KEYBOARD:
		return "Keyboard";
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
	static char buf[16];
	sprintf(buf, "%d", i / 8);
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
		if (IS_UPPER(*src) && src != first &&
			(!IS_UPPER(*(src - 1)) || (*(src + 1) && !IS_UPPER(*(src + 1)))))
		{
			*buf++ = ' ';
		}
		*buf++ = *src++;
	}
	*buf = '\0';
}
bool StrStartsWith(const char *str, const char *prefix)
{
	if (str == NULL || prefix == NULL)
	{
		return false;
	}
	const size_t lenStr = strlen(str);
	const size_t lenPrefix = strlen(prefix);
	if (lenPrefix > lenStr)
	{
		return false;
	}
	return strncmp(str, prefix, MIN(lenStr, lenPrefix)) == 0;
}
// From answer by plinth
// http://stackoverflow.com/a/744822/2038264
// License: http://creativecommons.org/licenses/by-sa/3.0/
// Author profile: http://stackoverflow.com/users/20481/plinth
bool StrEndsWith(const char *str, const char *suffix)
{
	if (str == NULL || suffix == NULL)
	{
		return false;
	}
	const size_t lenStr = strlen(str);
	const size_t lenSuffix = strlen(suffix);
	if (lenSuffix > lenStr)
	{
		return false;
	}
	return strncmp(str + lenStr - lenSuffix, suffix, lenSuffix) == 0;
}

// From answer by chux
// https://stackoverflow.com/a/30734030/2038264
// License: http://creativecommons.org/licenses/by-sa/3.0/
int Stricmp(const char *a, const char *b)
{
	int ca, cb;
	do
	{
		ca = (unsigned char)*a++;
		cb = (unsigned char)*b++;
		ca = tolower(toupper(ca));
		cb = tolower(toupper(cb));
	} while (ca == cb && ca != '\0');
	return ca - cb;
}

int CompareIntsAsc(const void *v1, const void *v2)
{
	const int i1 = *(const int *)v1;
	const int i2 = *(const int *)v2;
	if (i1 < i2)
	{
		return -1;
	}
	else if (i1 > i2)
	{
		return 1;
	}
	return 0;
}
int CompareIntsDesc(const void *v1, const void *v2)
{
	const int i1 = *(const int *)v1;
	const int i2 = *(const int *)v2;
	if (i1 > i2)
	{
		return -1;
	}
	else if (i1 < i2)
	{
		return 1;
	}
	return 0;
}

bool IntsEqual(const void *v1, const void *v2)
{
	return *(const int *)v1 == *(const int *)v2;
}

BodyPart StrBodyPart(const char *s)
{
	S2T(BODY_PART_HEAD, "head");
	S2T(BODY_PART_HAIR, "hair");
	S2T(BODY_PART_BODY, "body");
	S2T(BODY_PART_LEGS, "legs");
	S2T(BODY_PART_GUN_R, "gun_r");
	S2T(BODY_PART_GUN_L, "gun_l");
	return BODY_PART_HEAD;
}

int Pulse256(const int t)
{
	const int pulsePeriod = ConfigGetInt(&gConfig, "Game.FPS") / 2;
	int alphaUnscaled = (t % pulsePeriod) * 255 / (pulsePeriod / 2);
	if (alphaUnscaled > 255)
	{
		alphaUnscaled = 255 * 2 - alphaUnscaled;
	}
	return alphaUnscaled;
}
