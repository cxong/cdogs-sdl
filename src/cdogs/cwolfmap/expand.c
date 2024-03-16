/*
** Adapted from ecwolf/wolfmapcommon.cpp
** https://bitbucket.org/ecwolf/ecwolf/src/c7615c4744d6143ad72fc27cb73db9a80b5ba153/src/resourcefiles/wolfmapcommon.cpp?at=master#wolfmapcommon.cpp-40,51,58,63,144,147
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/
#include "expand.h"

#define CARMACK_NEARTAG 0xA7
#define CARMACK_FARTAG 0xA8

static WORD ReadLittleShort(const BYTE *const ptr)
{
	return (WORD)((BYTE)*ptr) | ((WORD)(BYTE)(*(ptr + 1)) << 8);
}
static inline void WriteLittleShort(BYTE *const ptr, WORD value)
{
	ptr[0] = value & 0xFF;
	ptr[1] = (value >> 8) & 0xFF;
}

// http://www.shikadi.net/moddingwiki/Carmack_compression
void ExpandCarmack(const unsigned char *in, unsigned char *out)
{
	const unsigned char *const end = out + ReadLittleShort((const BYTE *)in);
	const unsigned char *const start = out;
	in += 2;

	const unsigned char *copy;
	BYTE length;
	while (out < end)
	{
		length = *in++;
		if (length == 0 && (*in == CARMACK_NEARTAG || *in == CARMACK_FARTAG))
		{
			*out++ = in[1];
			*out++ = in[0];
			in += 2;
			continue;
		}
		else if (*in == CARMACK_NEARTAG)
		{
			copy = out - (in[1] * 2);
			in += 2;
		}
		else if (*in == CARMACK_FARTAG)
		{
			copy = start + (ReadLittleShort((const BYTE *)(in + 1)) * 2);
			in += 3;
		}
		else
		{
			*out++ = length;
			*out++ = *in++;
			continue;
		}
		if (out + (length * 2) > end)
			break;
		while (length-- > 0)
		{
			*out++ = *copy++;
			*out++ = *copy++;
		}
	}
}

// https://moddingwiki.shikadi.net/wiki/Id_Software_RLEW_compression
void ExpandRLEW(
	const unsigned char *in, unsigned char *out, const WORD rlewTag,
	const bool hasFinalLength)
{
	DWORD length = 64 * 64 * 2; // width*height*<bytes in 16bit>
	if (hasFinalLength)
	{
		length = ReadLittleShort((const BYTE *)in);
		in += 2;
	}
	const unsigned char *const end = out + length;

	while (out < end)
	{
		if (ReadLittleShort((const BYTE *)in) != rlewTag)
		{
			*out++ = *in++;
			*out++ = *in++;
		}
		else
		{
			WORD count = ReadLittleShort((const BYTE *)(in + 2));
			WORD input = ReadLittleShort((const BYTE *)(in + 4));
			in += 6;
			for (int i = 0; i < count; i++)
			{
				WriteLittleShort((BYTE *)out, input);
				out += 2;
			}
		}
	}
}
