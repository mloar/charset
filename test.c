/*
 * test.c - general test program which converts between two
 * arbitrary charsets.
 */

#include <stdio.h>
#include <string.h>
#include "charset.h"

#define lenof(x) ( sizeof((x)) / sizeof(*(x)) )

int main(int argc, char **argv)
{
    int srcset, dstset;
    charset_state instate = {0}, outstate = {0};

    if (argc != 3) {
	fprintf(stderr, "usage: test <charset> <charset>\n");
	return 1;
    }

    srcset = charset_from_localenc(argv[1]);
    if (srcset == CS_NONE) {
	fprintf(stderr, "unknown source charset '%s'\n", argv[1]);
	return 1;
    }

    dstset = charset_from_localenc(argv[2]);
    if (dstset == CS_NONE) {
	fprintf(stderr, "unknown destination charset '%s'\n", argv[2]);
	return 1;
    }

    while (1) {
	char inbuf[256], outbuf[256];
	wchar_t midbuf[256];
	char *inptr;
	wchar_t *midptr;
	int inlen, midlen, inret, midret;

	if (!fgets(inbuf, sizeof(inbuf), stdin))
	    break;		       /* EOF */

	inlen = strlen(inbuf);
	inptr = inbuf;
	while ( (inret = charset_to_unicode(&inptr, &inlen, midbuf,
					    lenof(midbuf), srcset,
					    &instate, NULL, 0)) > 0) {
	    midlen = inret;
	    midptr = midbuf;
	    while ( (midret = charset_from_unicode(&midptr, &midlen, outbuf,
						   lenof(outbuf), dstset,
						   &outstate, NULL, 0)) > 0) {
		fwrite(outbuf, 1, midret, stdout);
	    }
	}

    }

    return 0;
}
