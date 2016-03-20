/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2014 Cong Xu
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

#include "ui_object.h"

#define YC_APPEARANCE 0
#define YC_ATTRIBUTES 1
#define YC_FLAGS      2
#define YC_WEAPON     3

#define XC_FACE       0
#define XC_SKIN       1
#define XC_HAIR       2
#define XC_BODY       3
#define XC_ARMS       4
#define XC_LEGS       5

#define XC_SPEED      0
#define XC_HEALTH     1
#define XC_MOVE       2
#define XC_TRACK      3
#define XC_SHOOT      4
#define XC_DELAY      5

#define XC_ASBESTOS      0
#define XC_IMMUNITY      1
#define XC_SEETHROUGH    2
#define XC_RUNS_AWAY     3
#define XC_SNEAKY        4
#define XC_GOOD_GUY      5
#define XC_SLEEPING      6
#define XC_PRISONER      7
#define XC_INVULNERABLE  8
#define XC_FOLLOWER      9
#define XC_PENALTY       10
#define XC_VICTIM        11
#define XC_AWAKE         12

UIObject *CreateCharEditorObjs(void);
