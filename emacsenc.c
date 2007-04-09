/*
 * emacsenc.c - translate our internal character set codes to and from
 * GNU Emacs coding system symbols.  Derived from running M-x
 * list-coding-systems in Emacs 21.3.
 * 
 */

#include <ctype.h>
#include "charset.h"
#include "internal.h"

static const struct {
    const char *name;
    int charset;
} emacsencs[] = {
    /*
     * Where multiple encoding names map to the same encoding id
     * (such as iso-latin-1 and iso-8859-1), the first is considered
     * canonical and will be returned when translating the id to a
     * string.
     */
    { "us-ascii", CS_ASCII },
    { "iso-latin-9", CS_ISO8859_15 },
    { "iso-8859-15", CS_ISO8859_15 },
    { "latin-9", CS_ISO8859_15 },
    { "latin-0", CS_ISO8859_15 },
    { "iso-latin-1", CS_ISO8859_1 },
    { "iso-8859-1", CS_ISO8859_1 },
    { "latin-1", CS_ISO8859_1 },
    { "iso-latin-2", CS_ISO8859_2 },
    { "iso-8859-2", CS_ISO8859_2 },
    { "latin-2", CS_ISO8859_2 },
    { "iso-latin-3", CS_ISO8859_3 },
    { "iso-8859-3", CS_ISO8859_3 },
    { "latin-3", CS_ISO8859_3 },
    { "iso-latin-4", CS_ISO8859_4 },
    { "iso-8859-4", CS_ISO8859_4 },
    { "latin-4", CS_ISO8859_4 },
    { "cyrillic-iso-8bit", CS_ISO8859_5 },
    { "iso-8859-5", CS_ISO8859_5 },
    { "greek-iso-8bit", CS_ISO8859_7 },
    { "iso-8859-7", CS_ISO8859_7 },
    { "hebrew-iso-8bit", CS_ISO8859_8 },
    { "iso-8859-8", CS_ISO8859_8 },
    { "iso-8859-8-e", CS_ISO8859_8 },
    { "iso-8859-8-i", CS_ISO8859_8 },
    { "iso-latin-5", CS_ISO8859_9 },
    { "iso-8859-9", CS_ISO8859_9 },
    { "latin-5", CS_ISO8859_9 },
    { "chinese-big5", CS_BIG5 },
    { "big5", CS_BIG5 },
    { "cn-big5", CS_BIG5 },
    { "cp437", CS_CP437 },
    { "cp850", CS_CP850 },
    { "cp866", CS_CP866 },
    { "cp1250", CS_CP1250 },
    { "cp1251", CS_CP1251 },
    { "cp1253", CS_CP1253 },
    { "cp1257", CS_CP1257 },
    { "japanese-iso-8bit", CS_EUC_JP },
    { "euc-japan-1990", CS_EUC_JP },
    { "euc-japan", CS_EUC_JP },
    { "euc-jp", CS_EUC_JP },
    { "iso-2022-jp", CS_ISO2022_JP },
    { "junet", CS_ISO2022_JP },
    { "korean-iso-8bit", CS_EUC_KR },
    { "euc-kr", CS_EUC_KR },
    { "euc-korea", CS_EUC_KR },
    { "iso-2022-kr", CS_ISO2022_KR },
    { "korean-iso-7bit-lock", CS_ISO2022_KR },
    { "mac-roman", CS_MAC_ROMAN },
    { "cyrillic-koi8", CS_KOI8_R },
    { "koi8-r", CS_KOI8_R },
    { "koi8", CS_KOI8_R },
    { "japanese-shift-jis", CS_SHIFT_JIS },
    { "shift_jis", CS_SHIFT_JIS },
    { "sjis", CS_SHIFT_JIS },
    { "thai-tis620", CS_ISO8859_11 },
    { "th-tis620", CS_ISO8859_11 },
    { "tis620", CS_ISO8859_11 },
    { "tis-620", CS_ISO8859_11 },
    { "mule-utf-16-be", CS_UTF16BE },
    { "utf-16-be", CS_UTF16BE },
    { "mule-utf-16-le", CS_UTF16LE },
    { "utf-16-le", CS_UTF16LE },
    { "mule-utf-8", CS_UTF8 },
    { "utf-8", CS_UTF8 },
    { "vietnamese-viscii", CS_VISCII },
    { "viscii", CS_VISCII },
    { "iso-latin-8", CS_ISO8859_14 },
    { "iso-8859-14", CS_ISO8859_14 },
    { "latin-8", CS_ISO8859_14 },
    { "compound-text", CS_CTEXT },
    { "x-ctext", CS_CTEXT },
    { "ctext", CS_CTEXT },
    { "chinese-hz", CS_HZ },
    { "hz-gb-2312", CS_HZ },
    { "hz", CS_HZ },
};

const char *charset_to_emacsenc(int charset)
{
    int i;

    for (i = 0; i < (int)lenof(emacsencs); i++)
	if (charset == emacsencs[i].charset)
	    return emacsencs[i].name;

    return NULL;		       /* not found */
}

int charset_from_emacsenc(const char *name)
{
    int i;

    for (i = 0; i < (int)lenof(emacsencs); i++) {
	const char *p, *q;
	p = name;
	q = emacsencs[i].name;
	while (*p || *q) {
	    if (tolower(*p) != tolower(*q))
		break;
	    p++; q++;
	}
	if (!*p && !*q)
	    return emacsencs[i].charset;
    }

    return CS_NONE;		       /* not found */
}
