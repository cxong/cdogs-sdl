/*
    Copyright (c) 2015-2017, 2019 Cong Xu
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

#include <errno.h>
#include <stdarg.h>
#include <time.h>

#include "rlutil/rlutil.h"
#include "utils.h"


typedef struct
{
	LogLevel Level;
	const char *Name;
} LogModuleInfo;
// Log modules and default logging levels; indexed by LogModule
static LogModuleInfo sModuleInfo[] =
{
	{ LL_INFO, "MAIN" },
	{ LL_INFO, "NET" },
	{ LL_INFO, "INPUT" },
	{ LL_INFO, "ACTOR" },
	{ LL_INFO, "SOUND" },
	{ LL_INFO, "GFX" },
	{ LL_WARN, "MAP" },
	{ LL_INFO, "EDIT" },
	{ LL_INFO, "PATH" },
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

#ifdef __APPLE__
#include <os/log.h>
#endif
void LogInit(void)
{
	saveStreamDefaultColor(stderr);
}
void LogOpenFile(const char *filename)
{
	// Optionally log to file
	gLogFile = fopen(filename, "a+");
	if (gLogFile == NULL)
	{
		LOG(LM_MAIN, LL_ERROR, "Error opening log filename (%s): %s",
			filename, strerror(errno));
	}
}
void LogTerminate(void)
{
	if (gLogFile != NULL)
	{
		fclose(gLogFile);
	}
}

static void LogSetColor(const int color)
{
#ifdef __EMSCRIPTEN__
	UNUSED(color);
	return;
#else
	setStreamColor(stderr, color);
#endif
}

static void LogSetLevelColor(const LogLevel l)
{
	switch (l)
	{
	case LL_TRACE: LogSetColor(GREY); break;
	case LL_DEBUG: LogSetColor(WHITE); break;
	case LL_INFO: LogSetColor(GREEN); break;
	case LL_WARN: LogSetColor(YELLOW); break;
	case LL_ERROR: LogSetColor(RED); break;
	default: CASSERT(false, "Unknown log level"); break;
	}
}
static void LogSetModuleColor(void)
{
	LogSetColor(LIGHTBLUE);
}
static void LogSetFileColor(void)
{
	LogSetColor(BROWN);
}
static void LogSetFuncColor(void)
{
	LogSetColor(CYAN);
}
static void LogResetColor(void)
{
#ifndef __EMSCRIPTEN__
	resetStreamColor(stderr);
#endif
}

// LOG_STR / LOG_VSTR
#if defined(__EMSCRIPTEN__)

#define LOG_STR(_level, _stream, _fmt, ...)\
	if (_level >= LL_WARN)\
	{\
		fprintf(_stream, _fmt, ##__VA_ARGS__);\
	}\
	else\
	{\
		printf(_fmt, ##__VA_ARGS__);\
	}

#define LOG_VSTR(_level, _stream, _fmt, _args)\
	if (_level >= LL_WARN)\
	{\
		vfprintf(_stream, _fmt, _args);\
	}\
	else\
	{\
		vprintf(_fmt, _args);\
	}

#else

#define LOG_STR(_level, _stream, ...)\
	fprintf(_stream, ##__VA_ARGS__)
#define LOG_VSTR(_level, _stream, _fmt, _args)\
	vfprintf(_stream, _fmt, _args)

#endif
void LogLine(
	FILE *stream, const LogModule m, const LogLevel l, const char *filename,
	const int line, const char *function, const char *fmt, ...)
{
	if (stream == NULL
#if defined(__APPLE__) && !defined(NDEBUG)
		// Prevent SIGPIPE when writing to stderr
		|| stream == stderr
#endif
		)
	{
		return;
	}
	time_t now;
	time(&now);
	struct tm *ti = localtime(&now);
	LOG_STR(l, stream, "%d%02d%02d-%02d%02d%02d ",
		ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
		ti->tm_hour, ti->tm_min, ti->tm_sec);
	LogSetLevelColor(l);
	LOG_STR(l, stream, "%-5s ", LogLevelName(l));
	LogResetColor();
	LOG_STR(l, stream, "[");
	LogSetModuleColor();
	LOG_STR(l, stream, "%-5s", LogModuleName(m));
	LogResetColor();
	LOG_STR(l, stream, "] [");
	LogSetFileColor();
	LOG_STR(l, stream, "%s:%d", filename, line);
	LogResetColor();
	LOG_STR(l, stream, "] ");
	LogSetFuncColor();
	LOG_STR(l, stream, "%s()", function);
	LogResetColor();
	LOG_STR(l, stream, ": ");
	LogSetLevelColor(l);
	va_list args;
	va_start(args, fmt);
	LOG_VSTR(l, stream, fmt, args);
	va_end(args);
	LogResetColor();
	LOG_STR(l, stream, "\n");
	if (l >= LL_WARN)
	{
		fflush(stream);
	}
}

void LogFlush(FILE *stream)
{
	fflush(stream);
}
