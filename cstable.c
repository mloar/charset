/*
 * cstable.c - libcharset supporting utility which draws up a map
 * of the whole Unicode BMP and annotates it with details of which
 * other character sets each character appears in.
 * 
 * Note this is not a libcharset _client_; it is part of the
 * libcharset _package_, using libcharset internals.
 */

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

int main(void)
{
    long int c;

    for (c = 0; c < 0x10000; c++) {
	int i, row, col;
	char const *sep = "";

	printf("U+%04x:", c);

	/*
	 * Look up in SBCSes.
	 */
	for (i = 0; i < lenof(cs_table); i++)
	    if (cs_table[i]->read == read_sbcs &&
		sbcs_from_unicode(cs_table[i]->data, c) != ERROR) {
		printf("%s %s", sep,
		       charset_to_localenc(cs_table[i]->charset));
		sep = ";";
	    }

	/*
	 * Look up individually in MBCS base charsets.
	 */
	if (unicode_to_big5(c, &row, &col)) {
	    printf("%s Big5", sep);
	    sep = ";";
	}
	if (unicode_to_gb2312(c, &row, &col)) {
	    printf("%s GB2312", sep);
	    sep = ";";
	}

	if (unicode_to_jisx0208(c, &row, &col)) {
	    printf("%s JIS X 0208", sep);
	    sep = ";";
	}

	if (unicode_to_ksx1001(c, &row, &col)) {
	    printf("%s KS X 1001", sep);
	    sep = ";";
	}

	if (unicode_to_cp949(c, &row, &col)) {
	    printf("%s CP949", sep);
	    sep = ";";
	}

	if (!*sep)
	    printf(" unicode-only");

	printf("\n");
    }

    return 0;
}