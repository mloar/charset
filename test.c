/*
 * test.c - general libcharset test/demo program which converts
 * between two arbitrary charsets.
 */

#include <stdio.h>
#include <string.h>
#include "charset.h"

#define lenof(x) ( sizeof((x)) / sizeof(*(x)) )

int main(int argc, char **argv)
{
    int srcset, dstset;
    charset_state instate = CHARSET_INIT_STATE;
    charset_state outstate = CHARSET_INIT_STATE;
    char inbuf[256], outbuf[256];
    wchar_t midbuf[256];
    const char *inptr;
    const wchar_t *midptr;
    int rdret, inlen, midlen, inret, midret;

    if (argc != 3) {
	fprintf(stderr, "usage: convcs <charset> <charset>\n");
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

	rdret = fread(inbuf, 1, sizeof(inbuf), stdin);

	if (rdret <= 0)
	    break;		       /* EOF */

	inlen = rdret;
	inptr = inbuf;
	while ( (inret = charset_to_unicode(&inptr, &inlen, midbuf,
					    lenof(midbuf), srcset,
					    &instate, NULL, 0)) > 0) {
	    midlen = inret;
	    midptr = midbuf;
	    while ( (midret = charset_from_unicode(&midptr, &midlen, outbuf,
						   lenof(outbuf), dstset,
						   &outstate, NULL)) > 0) {
		fwrite(outbuf, 1, midret, stdout);
	    }
	}
    }

    /*
     * Reset encoding state.
     */
    while ( (midret = charset_from_unicode(NULL, NULL, outbuf,
					   lenof(outbuf), dstset,
					   &outstate, NULL)) > 0) {
	fwrite(outbuf, 1, midret, stdout);
    }

    return 0;
}
