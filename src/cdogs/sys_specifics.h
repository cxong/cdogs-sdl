/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

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
#pragma once

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#ifdef _MSC_VER
#define inline __inline
#endif

#ifdef _WIN32
#define HOME_DIR_ENV "AppData"
#else
#define HOME_DIR_ENV "HOME"
#endif

#ifdef _MSC_VER
#include <direct.h>
#include <io.h>
#define access _access
/* Values for the second argument to access.
   These may be OR'd together.  */
#define R_OK    4       /* Test for read permission.  */
#define W_OK    2       /* Test for write permission.  */
#define F_OK    0       /* Test for existence.  */
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <MacTypes.h>
#endif

#ifdef _MSC_VER
#define MKDIR_MODE 0
#else
#include <sys/stat.h>
#define MKDIR_MODE (S_IRUSR | S_IXUSR | S_IWUSR)
#endif

#ifdef _MSC_VER
#define mkdir(p, a) _mkdir(p)
#elif defined(_WIN32)
#define mkdir(p, a) mkdir(p)
#endif

#ifndef __func__
#define __func__ __FUNCTION__
#endif
