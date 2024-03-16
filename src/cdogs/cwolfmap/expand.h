#pragma once
#include <stdbool.h>
#include <stdint.h>

#define BYTE uint8_t
#define WORD uint16_t
#define DWORD uint32_t

void ExpandCarmack(const unsigned char *in, unsigned char *out);
void ExpandRLEW(
	const unsigned char *in, unsigned char *out, const WORD rlewTag,
	const bool hasFinalLength);
