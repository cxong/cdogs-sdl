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

    Copyright (c) 2013, Cong Xu
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
#ifndef __UTILS
#define __UTILS

#include <stdio.h> /* for stderr */
#include <stdlib.h>

extern int debug;
extern int debug_level;

#define D_NORMAL	0
#define D_VERBOSE	1
#define D_MAX		2

#define debug(n,...)	if (debug && (n <= debug_level)) { fprintf(stderr, "[%s:%d] %s(): ", __FILE__, __LINE__, __FUNCTION__); fprintf(stderr, __VA_ARGS__); }

#define _CCHECKALLOC(_func, _var, _size)\
{\
	if (_var == NULL)\
	{\
		debug(D_MAX,\
			_func "(" #_var " size %d) failed\n",\
			(int)_size);\
		exit(1);\
	}\
	debug(D_MAX,\
		_func "(" #_var " size %d) at 0x%p\n",\
		(int)_size, _var);\
}

#define CMALLOC(_var, _size)\
{\
	_var = malloc(_size);\
	_CCHECKALLOC("CMALLOC", _var, (_size))\
}
#define CCALLOC(_var, _size)\
{\
	_var = calloc(1, _size);\
	_CCHECKALLOC("CCALLOC", _var, (_size))\
}
#define CREALLOC(_var, _size)\
{\
	_var = realloc(_var, _size);\
	_CCHECKALLOC("CREALLOC", _var, (_size))\
}

#define CFREE(_var)\
{\
	debug(D_MAX,\
		"CFREE(" #_var ") at 0x%p\n",\
		_var);\
	free(_var);\
}

#define UNUSED(expr) (void)(expr);

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define CLAMP(v, _min, _max) MAX((_min), MIN((_max), (v)))
#define CLAMP_OPPOSITE(v, _min, _max)\
	((v) > (_max) ? (_min) : ((v) < (_min) ? (_max) : (v)))

int MemIsZero(const void *buf, size_t len);

const char *StrGetFileExt(const char *filename);

#define PI 3.14159265

double Round(double x);

double ToDegrees(double radians);

typedef enum
{
	INPUT_DEVICE_UNSET,
	INPUT_DEVICE_KEYBOARD,
	INPUT_DEVICE_MOUSE,
	INPUT_DEVICE_JOYSTICK,

	// Fake device used for co-op AI
	INPUT_DEVICE_AI,
	
	INPUT_DEVICE_COUNT
} input_device_e;

#endif
