/*
    Copyright (c) 2013-2018 Cong Xu
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

void CArrayInit(CArray *a, size_t elemSize)
{
	a->data = NULL;
	a->elemSize = elemSize;
	a->size = 0;
	a->capacity = 0;
}
void CArrayReserve(CArray *a, size_t capacity)
{
	if (a->capacity >= capacity)
	{
		return;
	}
	a->capacity = capacity;
	const size_t size = a->capacity * a->elemSize;
	if (size)
	{
		CREALLOC(a->data, size);
	}
}
static void GrowIfFull(CArray *a)
{
	if (a->size == a->capacity)
	{
		const size_t capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		CArrayReserve(a, capacity);
	}
}
void CArrayCopy(CArray *dst, const CArray *src)
{
	if (dst->elemSize > 0)
	{
		CArrayTerminate(dst);
	}
	CArrayInit(dst, src->elemSize);
	CArrayResize(dst, src->size, NULL);
	memcpy(dst->data, src->data, src->size * src->elemSize);
}

void CArrayPushBack(CArray *a, const void *elem)
{
	CASSERT(a->elemSize > 0, "array has not been initialised");
	GrowIfFull(a);
	a->size++;
	memcpy(CArrayGet(a, (int)a->size - 1), elem, a->elemSize);
}
void CArrayInsert(CArray *a, const size_t idx, const void *elem)
{
	CASSERT(a->elemSize > 0, "array has not been initialised");
	GrowIfFull(a);
	a->size++;
	if (idx + 1 < a->size)
	{
		memmove(
			CArrayGet(a, idx + 1),
			CArrayGet(a, idx),
			a->elemSize * (a->size - 1 - idx));
	}
	CArraySet(a, idx, elem);
}
void CArrayDelete(CArray *a, const size_t idx)
{
	CASSERT(a->size != 0, "Cannot delete from empty array");
	CASSERT(a->elemSize > 0, "array has not been initialised");
	CASSERT(idx < a->size, "array index out of bounds");
	if (idx + 1 < a->size)
	{
		memmove(
			CArrayGet(a, idx),
			CArrayGet(a, idx + 1),
			a->elemSize * (a->size - 1 - idx));
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

void *CArrayGet(const CArray *a, const size_t idx)
{
	CASSERT(a->elemSize > 0, "array has not been initialised");
	CASSERT(idx < a->size, "array index out of bounds");
	return &((char *)a->data)[idx * a->elemSize];
}
void CArraySet(CArray *a, const size_t idx, const void *elem)
{
	memcpy(CArrayGet(a, idx), elem, a->elemSize);
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

// Fill all entries with the same element
void CArrayFill(CArray *a, const void *elem)
{
	CA_FOREACH(void, e, *a)
		memcpy(e, elem, a->elemSize);
	CA_FOREACH_END()
}

void CArrayFillZero(CArray *a)
{
	memset(a->data, 0, a->size * a->elemSize);
}

void CArrayShuffle(CArray *a)
{
	void *buf;
	CMALLOC(buf, a->elemSize);
	CA_FOREACH(void, e, *a)
		const int j = rand() % (_ca_index + 1);
		void *je = CArrayGet(a, j);
		// Swap index and j elements
		memcpy(buf, e, a->elemSize);
		memcpy(e, je, a->elemSize);
		memcpy(je, buf, a->elemSize);
	CA_FOREACH_END()
	CFREE(buf);
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
