/*
 * cstable.c - libcharset supporting utility which draws up a map
 * of the whole Unicode BMP and annotates it with details of which
 * other character sets each character appears in.
 * 
 * Note this is not a libcharset _client_; it is part of the
 * libcharset _package_, using libcharset internals.
 */

#include <stdio.h>
#include <string.h>

#include "charset.h"
#include "internal.h"
#include "sbcsdat.h"

#define ENUM_CHARSET(x) extern charset_spec const charset_##x;
#include "enum.c"
#undef ENUM_CHARSET
static charset_spec const *const cs_table[] = {
#define ENUM_CHARSET(x) &charset_##x,
#include "enum.c"
#undef ENUM_CHARSET
};
static const char *const cs_names[] = {
#define ENUM_CHARSET(x) #x,
#include "enum.c"
#undef ENUM_CHARSET
};

int main(int argc, char **argv)
{
    long int c;
    int internal_names = FALSE;
    int verbose = FALSE;

    while (--argc) {
        char *p = *++argv;
        if (!strcmp(p, "-i"))
            internal_names = TRUE;
        else if (!strcmp(p, "-v"))
            verbose = TRUE;
    }

    for (c = 0; c < 0x30000; c++) {
	int i, plane, row, col, chr;
	char const *sep = "";

	printf("U+%04x:", c);

	/*
	 * Look up in SBCSes.
	 */
	for (i = 0; i < lenof(cs_table); i++)
	    if (cs_table[i]->read == read_sbcs &&
		(chr = sbcs_from_unicode(cs_table[i]->data, c)) != ERROR) {
		printf("%s %s", sep,
		       (internal_names ? cs_names[i] :
			charset_to_localenc(cs_table[i]->charset)));
		if (verbose)
		    printf("[%d]", chr);
		sep = ";";
	    }

	/*
	 * Look up individually in MBCS base charsets. The
	 * `internal_names' flag does not affect these, because
	 * MBCS base charsets aren't directly encoded by CS_*
	 * constants.
	 */
	if (unicode_to_big5(c, &row, &col)) {
	    printf("%s Big5", sep);
	    if (verbose)
		printf("[%d,%d]", row, col);
	    sep = ";";
	}

	if (unicode_to_gb2312(c, &row, &col)) {
	    printf("%s GB2312", sep);
	    if (verbose)
		printf("[%d,%d]", row, col);
	    sep = ";";
	}

	if (unicode_to_jisx0208(c, &row, &col)) {
	    printf("%s JIS X 0208", sep);
	    if (verbose)
		printf("[%d,%d]", row, col);
	    sep = ";";
	}

	if (unicode_to_ksx1001(c, &row, &col)) {
	    printf("%s KS X 1001", sep);
	    if (verbose)
		printf("[%d,%d]", row, col);
	    sep = ";";
	}

	if (unicode_to_cp949(c, &row, &col)) {
	    printf("%s CP949", sep);
	    if (verbose)
		printf("[%d,%d]", row, col);
	    sep = ";";
	}

	if (unicode_to_cns11643(c, &plane, &row, &col)) {
	    printf("%s CNS11643", sep);
	    if (verbose)
		printf("[%d,%d,%d]", plane, row, col);
	    sep = ";";
	}

	if (!*sep)
	    printf(" unicode-only");

	printf("\n");
    }

    return 0;
}
