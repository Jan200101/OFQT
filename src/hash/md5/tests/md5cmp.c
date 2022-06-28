/**
 * Compares the hash of a given string against a
 * given hash
 * 
 * Used for testing to ensure we don't run into
 * Windows weirdness again
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md5.h"


int main(int argc, char** argv)
{
	if (argc < 3)
	{
		puts("md5cmp input expected-hash");
		return 1;
	}

    MD5_CTX context;
    MD5_Init(&context);
    MD5_Update(&context, argv[1], strlen(argv[1]));

    unsigned char digest[16];
    MD5_Final(digest, &context);

    char md5string[33];
    for(int i = 0; i < 16; ++i)
        snprintf(&md5string[i*2], 3, "%02x", (unsigned int)digest[i]);

    if (!strncmp(md5string, argv[2], sizeof(md5string)))
        return 0;

    return 1;
}