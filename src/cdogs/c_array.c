/*
    Copyright (c) 2013-2014, Cong Xu
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
#include "c_array.h"

#include <assert.h>
#include <string.h>

#include "utils.h"

void CArrayInit(CArray *a, size_t elemSize)
{
	a->data = NULL;
	a->elemSize = elemSize;
	a->size = 0;
	a->capacity = 1;
	CMALLOC(a->data, a->capacity * a->elemSize);
}
void CArrayCopy(CArray *dst, CArray *src)
{
	int i;
	assert(dst->size == 0);
	dst->elemSize = src->elemSize;
	for (i = 0; i < (int)src->size; i++)
	{
		CArrayPushBack(dst, CArrayGet(src, i));
	}
}

void CArrayPushBack(CArray *a, void *elem)
{
	if (a->size == a->capacity)
	{
		a->capacity *= 2;
		CREALLOC(a->data, a->capacity * a->elemSize);
	}
	memcpy(CArrayGet(a, a->size), elem, a->elemSize);
	a->size++;
}
void CArrayInsert(CArray *a, int index, void *elem)
{
	int i;
	if (a->size == a->capacity)
	{
		a->capacity *= 2;
		CREALLOC(a->data, a->capacity * a->elemSize);
	}
	for (i = a->size; i > index; i--)
	{
		memcpy(CArrayGet(a, i), CArrayGet(a, i - 1), a->elemSize);
	}
	memcpy(CArrayGet(a, index), elem, a->elemSize);
	a->size++;
}
void CArrayDelete(CArray *a, int index)
{
	int i;
	if (a->size == 0)
	{
		return;
	}
	a->size--;
	memset(CArrayGet(a, index), 0, a->elemSize);
	for (i = index; i < (int)a->size; i++)
	{
		memcpy(CArrayGet(a, i), CArrayGet(a, i + 1), a->elemSize);
	}
}

void *CArrayGet(CArray *a, int index)
{
	return &((char *)a->data)[index * a->elemSize];
}

void CArrayClear(CArray *a)
{
	a->size = 0;
}

void CArrayTerminate(CArray *a)
{
	if (!a)
	{
		return;
	}
	CFREE(a->data);
	memset(a, 0, sizeof *a);
}
