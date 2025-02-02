/*
	Copyright (c) 2013-2018, 2021, 2024 Cong Xu
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
#include <stddef.h>

// dynamic array
typedef struct
{
	void *data;
	size_t elemSize;
	size_t size;
	size_t capacity;
} CArray;

void CArrayInit(CArray *a, const size_t elemSize);
void CArrayInitFill(
	CArray *a, const size_t elemSize, const size_t size, const void *value);
void CArrayInitFillZero(CArray *a, const size_t elemSize, const size_t size);
void CArrayReserve(CArray *a, size_t capacity);
void CArrayCopy(CArray *dst, const CArray *src);
void *CArrayPushBack(CArray *a, const void *elem); // insert address
void *CArrayInsert(CArray *a, const size_t index, const void *elem);
void CArrayDelete(CArray *a, const size_t index);
void CArrayPopBack(CArray *a);
void CArrayResize(CArray *a, const size_t size, const void *value);
void *CArrayGet(const CArray *a, const size_t index); // gets address
void *CArraySet(CArray *a, const size_t idx, const void *elem);
void CArrayClear(CArray *a);
void CArrayRemoveIf(CArray *a, bool (*removeIf)(const void *));
void CArrayFill(CArray *a, const void *elem);
void CArrayFillZero(CArray *a);
void CArrayConcat(CArray *a, const CArray *a2);
void CArrayShuffle(CArray *a);
// Remove consecutive duplicates
void CArrayUnique(CArray *a, bool (*isEqual)(const void *, const void *));
void CArrayTerminate(CArray *a);

// Convenience macro for looping through a CArray
#define CA_FOREACH(_type, _var, _a)                                           \
	for (int _ca_index = 0; _ca_index < (int)(_a).size; _ca_index++)          \
	{                                                                         \
		_type *_var = CArrayGet(&(_a), _ca_index);
#define CA_FOREACH_END() }
