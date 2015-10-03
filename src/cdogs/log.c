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
#include "log.h"

#include "rlutil/rlutil.h"
#include "utils.h"


typedef struct
{
	LogLevel Level;
	const char *Name;
} LogModuleInfo;
// Indexed by LogModule
static LogModuleInfo sModuleInfo[] =
{
	{ LL_INFO, "MAIN" },
	{ LL_INFO, "NET" },
	{ LL_INFO, "INPUT" },
	{ LL_INFO, "ACTOR" },
	{ LL_INFO, "SOUND" },
};


const char *LogModuleName(const LogModule m)
{
	if ((int)m >= 0 && (int)m < LM_COUNT) return sModuleInfo[(int)m].Name;
	CASSERT(false, "Unknown log module");
	return "";
}
LogModule StrLogModule(const char *s)
{
	for (int i = 0; i < (int)LM_COUNT; i++)
	{
		if (strcmp(s, sModuleInfo[i].Name) == 0) return (LogModule)i;
	}
	return LM_MAIN;
}

LogLevel LogModuleGetLevel(const LogModule m)
{
	if ((int)m >= 0 && (int)m < LM_COUNT) return sModuleInfo[(int)m].Level;
	CASSERT(false, "Unknown log module");
	return LL_ERROR;
}
void LogModuleSetLevel(const LogModule m, const LogLevel l)
{
	if ((int)m >= 0 && (int)m < LM_COUNT)
	{
		sModuleInfo[(int)m].Level = l;
	}
	else
	{
		CASSERT(false, "Unknown log module");
	}
}
const char *LogLevelName(const LogLevel l)
{
	switch (l)
	{
		T2S(LL_TRACE, "TRACE");
		T2S(LL_DEBUG, "DEBUG");
		T2S(LL_INFO, "INFO");
		T2S(LL_WARN, "WARN");
		T2S(LL_ERROR, "ERROR");
	default:
		return "";
	}
}
LogLevel StrLogLevel(const char *s)
{
	S2T(LL_TRACE, "TRACE");
	S2T(LL_DEBUG, "DEBUG");
	S2T(LL_INFO, "INFO");
	S2T(LL_WARN, "WARN");
	S2T(LL_ERROR, "ERROR");
	return LL_ERROR;
}

void LogSetLevelColor(const LogLevel l)
{
	switch (l)
	{
	case LL_TRACE: setColor(GREY); break;
	case LL_DEBUG: setColor(WHITE); break;
	case LL_INFO: setColor(GREEN); break;
	case LL_WARN: setColor(YELLOW); break;
	case LL_ERROR: setColor(RED); break;
	default: CASSERT(false, "Unknown log level"); break;
	}
}
void LogSetModuleColor(void)
{
	setColor(LIGHTBLUE);
}
void LogSetFileColor(void)
{
	setColor(BROWN);
}
void LogSetFuncColor(void)
{
	setColor(CYAN);
}
void LogResetColor(void)
{
	setColor(GREY);
}
