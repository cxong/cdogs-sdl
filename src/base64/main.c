#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "base64.h"

static void
test(unsigned char *encode, unsigned int encodelen,
     char *decode, unsigned int decodelen)
{
	char *encode_out;;
	unsigned char *decode_out;

	encode_out = malloc(BASE64_ENCODE_OUT_SIZE(encodelen));
	decode_out = malloc(BASE64_DECODE_OUT_SIZE(decodelen));
	assert(encode_out);
	assert(decode_out);

	assert(base64_encode(encode, encodelen, encode_out) == decodelen);
	assert(memcmp(encode_out, decode, decodelen) == 0);
	assert(base64_decode(decode, decodelen, decode_out) == encodelen);
	assert(memcmp(decode_out, encode, encodelen) == 0);

	free(encode_out);
	free(decode_out);
}

int
main(void)
{
	test((void *)"", 0, "", 0);
	test((void *)"f", 1, "Zg==", 4);
	test((void *)"fo", 2, "Zm8=", 4);
	test((void *)"foo", 3, "Zm9v", 4);
	test((void *)"foob", 4, "Zm9vYg==", 8);
	test((void *)"fooba", 5, "Zm9vYmE=", 8);
	test((void *)"foobar", 6, "Zm9vYmFy", 8);

	return 0;
}
