/*
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
#include "c_array.h"

#include <string.h>

#include "utils.h"

void CArrayInit(CArray *a, size_t elemSize)
{
	a->data = NULL;
	a->elemSize = elemSize;
	a->size = 0;
	a->capacity = 0;
	CArrayReserve(a, 1);
}
void CArrayReserve(CArray *a, size_t capacity)
{
	if (a->capacity >= capacity)
	{
		return;
	}
	a->capacity = capacity;
	CREALLOC(a->data, a->capacity * a->elemSize);
}
void CArrayCopy(CArray *dst, const CArray *src)
{
	CArrayTerminate(dst);
	CArrayInit(dst, src->elemSize);
	CArrayReserve(dst, (int)src->size);
	for (int i = 0; i < (int)src->size; i++)
	{
		CArrayPushBack(dst, CArrayGet(src, i));
	}
}

void CArrayPushBack(CArray *a, const void *elem)
{
	CASSERT(a->elemSize > 0, "array has not been initialised");
	if (a->size == a->capacity)
	{
		CArrayReserve(a, a->capacity * 2);
	}
	a->size++;
	memcpy(CArrayGet(a, a->size - 1), elem, a->elemSize);
}
void CArrayInsert(CArray *a, int idx, void *elem)
{
	CASSERT(a->elemSize > 0, "array has not been initialised");
	if (a->size == a->capacity)
	{
		CArrayReserve(a, a->capacity * 2);
	}
	a->size++;
	if (idx < (int)a->size - 1)
	{
		memmove(
			CArrayGet(a, idx + 1),
			CArrayGet(a, idx),
			a->elemSize * ((int)a->size - 1 - idx));
	}
	memcpy(CArrayGet(a, idx), elem, a->elemSize);
}
void CArrayDelete(CArray *a, int idx)
{
	CASSERT(a->size != 0, "Cannot delete from empty array");
	CASSERT(a->elemSize > 0, "array has not been initialised");
	CASSERT(idx >= 0 && idx < (int)a->size, "array index out of bounds");
	if (idx < (int)a->size - 1)
	{
		memmove(
			CArrayGet(a, idx),
			CArrayGet(a, idx + 1),
			a->elemSize * ((int)a->size - 1 - idx));
	}
	a->size--;
}
void CArrayResize(CArray *a, const size_t size, const void *value)
{
	CArrayReserve(a, size);
	if (value != NULL)
	{
		while (a->size < size)
		{
			CArrayPushBack(a, value);
		}
	}
	a->size = size;
}

void *CArrayGet(const CArray *a, int idx)
{
	CASSERT(a->elemSize > 0, "array has not been initialised");
	CASSERT(idx >= 0 && idx < (int)a->size, "array index out of bounds");
	return &((char *)a->data)[idx * a->elemSize];
}

void CArrayClear(CArray *a)
{
	a->size = 0;
}

void CArrayRemoveIf(CArray *a, bool (*removeIf)(const void *))
{
	// Note: this is the erase-remove idiom
	// Check all elements to see if they need to be removed
	// Move all the elements that don't need to be removed to the head
	void *dst = a->data;
	size_t shrunkSize = 0;
	for (int i = 0; i < (int)a->size; i++)
	{
		const void *elem = CArrayGet(a, i);
		if (!removeIf(elem))
		{
			memmove(dst, elem, a->elemSize);
			dst = (char *)dst + a->elemSize;
			shrunkSize++;
		}
	}

	// Shrink the array to fit
	a->size = shrunkSize;
}

void CArrayFillZero(CArray *a)
{
	memset(a->data, 0, a->size * a->elemSize);
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
