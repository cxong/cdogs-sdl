/*
    Copyright (c) 2015, Cong Xu
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

#include <stdbool.h>

#include "sys_specifics.h"


typedef enum
{
	LM_MAIN,
	LM_NET,
	LM_COUNT
} LogModule;
const char *LogModuleName(const LogModule m);
LogModule StrLogModule(const char *s);
typedef enum
{
	LL_TRACE,
	LL_DEBUG,
	LL_INFO,
	LL_WARN,
	LL_ERROR
} LogLevel;
LogLevel LogModuleGetLevel(const LogModule m);
void LogModuleSetLevel(const LogModule m, const LogLevel l);
const char *LogLevelName(const LogLevel l);
LogLevel StrLogLevel(const char *s);

#define LOG(_module, _level, ...)\
	do\
	{\
		if (_level >= LogModuleGetLevel(_module))\
		{\
			fprintf(stderr, "%-5s [%-4s] [%s:%d] %s(): ",\
				LogLevelName(_level), LogModuleName(_module),\
				__FILE__, __LINE__, __FUNCTION__);\
			fprintf(stderr, __VA_ARGS__);\
			fprintf(stderr, "\n");\
		}\
	} while ((void)0, 0)
