/*
 * locale.c: try very hard to figure out the libcharset charset
 * identifier corresponding to the current C library locale.
 * 
 * This function works by calling nl_langinfo(CODESET) if
 * available; failing that, it will try to figure it out manually
 * by examining the locale environment variables. Code for the
 * latter is adapted from Markus Kuhn's public-domain
 * implementation of nl_langinfo(CODESET), available at
 * 
 *   http://www.cl.cam.ac.uk/~mgk25/ucs/langinfo.c
 */

#include <stdlib.h>
#include <string.h>
#include "charset.h"

#ifdef HAS_LANGINFO_CODESET
#include "langinfo.h"
#endif

int charset_from_locale(void)
{
    char *l, *p;

#ifdef HAS_LANGINFO_CODESET
    int charset;
    char const *csname;

    csname = nl_langinfo(CODESET);
    if (csname &&
	(charset = charset_from_localenc(csname)) != CS_NONE)
	return charset;
#endif

    if (((l = getenv("LC_ALL"))   && *l) ||
	((l = getenv("LC_CTYPE")) && *l) ||
	((l = getenv("LANG"))     && *l)) {
	/* check standardized locales */
	if (!strcmp(l, "C") || !strcmp(l, "POSIX"))
	    return CS_ASCII;
	/* check for encoding name fragment */
	if (strstr(l, "UTF") || strstr(l, "utf"))
	    return CS_UTF8;
	if ((p = strstr(l, "8859-"))) {
	    char buf[16];
	    int charset;

	    memcpy(buf, "ISO-8859-\0\0", 12);
	    p += 5;
	    if ((*p) >= '0' && (*p) <= '9') {
		buf[9] = *p++;
		if ((*p) >= '0' && (*p) <= '9') buf[10] = *p++;
		if ((charset = charset_from_localenc(buf)) != CS_NONE)
		    return charset;
	    }
	}
	if (strstr(l, "KOI8-RU")) return CS_KOI8_RU;
	if (strstr(l, "KOI8-R")) return CS_KOI8_R;
	if (strstr(l, "KOI8-U")) return CS_KOI8_U;
	/* if (strstr(l, "620")) return "TIS-620"; */
	if (strstr(l, "2312")) return CS_EUC_CN;
	/* if (strstr(l, "HKSCS")) return "Big5HKSCS"; */
	if (strstr(l, "Big5") || strstr(l, "BIG5")) return CS_BIG5;
	/* if (strstr(l, "GBK")) return "GBK"; */
	/* if (strstr(l, "18030")) return "GB18030"; */
	if (strstr(l, "Shift_JIS") || strstr(l, "SJIS")) return CS_SHIFT_JIS;
	/* check for conclusive modifier */
	if (strstr(l, "euro")) return CS_ISO8859_15;
	/* check for language (and perhaps country) codes */
	if (strstr(l, "zh_TW")) return CS_BIG5;
	/* if (strstr(l, "zh_HK")) return "Big5HKSCS"; */
	if (strstr(l, "zh")) return CS_EUC_CN;
	if (strstr(l, "ja")) return CS_EUC_JP;
	if (strstr(l, "ko")) return CS_EUC_KR;
	if (strstr(l, "ru")) return CS_KOI8_R;
	if (strstr(l, "uk")) return CS_KOI8_U;
	if (strstr(l, "pl") || strstr(l, "hr") ||
	    strstr(l, "hu") || strstr(l, "cs") ||
	    strstr(l, "sk") || strstr(l, "sl")) return CS_ISO8859_2;
	if (strstr(l, "eo") || strstr(l, "mt")) return CS_ISO8859_3;
	if (strstr(l, "el")) return CS_ISO8859_7;
	if (strstr(l, "he")) return CS_ISO8859_8;
	if (strstr(l, "tr")) return CS_ISO8859_9;
	/* if (strstr(l, "th")) return "TIS-620"; */
	if (strstr(l, "lt")) return CS_ISO8859_13;
	if (strstr(l, "cy")) return CS_ISO8859_14;
	if (strstr(l, "ro")) return CS_ISO8859_2;   /* or ISO-8859-16 */
	if (strstr(l, "am") || strstr(l, "vi")) return CS_UTF8;
	return CS_ISO8859_1;
    }
    return CS_ASCII;
}
