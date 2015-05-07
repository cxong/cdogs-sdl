/*
 * Copyright (C) 2003 Maxim Stepin ( maxst@hiend3d.com )
 *
 * Copyright (C) 2010 Cameron Zemek ( grom@zeminvaders.net)
 * Copyright (C) 2011 Francois Gannaz <mytskine@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __HQX_COMMON_H_
#define __HQX_COMMON_H_

#include <stdlib.h>
#include <stdint.h>

#define MASK_2     0x0000FF00
#define MASK_13    0x00FF00FF
#define MASK_RGB   0x00FFFFFF
#define MASK_ALPHA 0xFF000000

#define Ymask 0x00FF0000
#define Umask 0x0000FF00
#define Vmask 0x000000FF
#define trY   0x00300000
#define trU   0x00000700
#define trV   0x00000006

/* RGB to YUV lookup table */
extern uint32_t RGBtoYUV[16777216];

uint32_t rgb_to_yuv(uint32_t c);

/* Test if there is difference in color */
int yuv_diff(uint32_t yuv1, uint32_t yuv2);

int Diff(uint32_t c1, uint32_t c2);

/* Interpolate functions */
uint32_t Interpolate_2(uint32_t c1, int w1, uint32_t c2, int w2, int s);

uint32_t Interpolate_3(uint32_t c1, int w1, uint32_t c2, int w2, uint32_t c3, int w3, int s);

void Interp1(uint32_t * pc, uint32_t c1, uint32_t c2);

void Interp2(uint32_t * pc, uint32_t c1, uint32_t c2, uint32_t c3);

void Interp3(uint32_t * pc, uint32_t c1, uint32_t c2);

void Interp4(uint32_t * pc, uint32_t c1, uint32_t c2, uint32_t c3);

void Interp5(uint32_t * pc, uint32_t c1, uint32_t c2);

void Interp6(uint32_t * pc, uint32_t c1, uint32_t c2, uint32_t c3);

void Interp7(uint32_t * pc, uint32_t c1, uint32_t c2, uint32_t c3);

void Interp8(uint32_t * pc, uint32_t c1, uint32_t c2);

void Interp9(uint32_t * pc, uint32_t c1, uint32_t c2, uint32_t c3);

void Interp10(uint32_t * pc, uint32_t c1, uint32_t c2, uint32_t c3);

#endif
