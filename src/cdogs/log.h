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
	LM_INPUT,
	LM_ACTOR,
	LM_SOUND,
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

void LogSetLevelColor(const LogLevel l);
void LogSetModuleColor(void);
void LogSetFileColor(void);
void LogSetFuncColor(void);
void LogResetColor(void);

#define LOG(_module, _level, ...)\
	do\
	{\
		if (_level >= LogModuleGetLevel(_module))\
		{\
			LogSetLevelColor(_level);\
			fprintf(stderr, "%-5s ", LogLevelName(_level));\
			LogResetColor();\
			fprintf(stderr, "[");\
			LogSetModuleColor();\
			fprintf(stderr, "%-5s", LogModuleName(_module));\
			LogResetColor();\
			fprintf(stderr, "] [");\
			LogSetFileColor();\
			fprintf(stderr, "%s:%d", __FILE__, __LINE__);\
			LogResetColor();\
			fprintf(stderr, "] ");\
			LogSetFuncColor();\
			fprintf(stderr, "%s()", __FUNCTION__);\
			LogResetColor();\
			fprintf(stderr, ": ");\
			LogSetLevelColor(_level);\
			fprintf(stderr, __VA_ARGS__);\
			LogResetColor();\
			fprintf(stderr, "\n");\
		}\
	} while ((void)0, 0)
