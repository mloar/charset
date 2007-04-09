/*
 * local.c - translate our internal character set codes to and from
 * our own set of plausibly legible character-set names. Also
 * provides a canonical name for each encoding (useful for software
 * announcing what character set it will be using), and a set of
 * enumeration functions which return a list of supported
 * encodings one by one.
 * 
 * Also in this table are other ways people might plausibly refer
 * to a charset (for example, Win1252 as well as CP1252). Where
 * more than one string is given for a particular character set,
 * the first one is the canonical one returned by
 * charset_to_localenc.
 * 
 * charset_from_localenc will attempt all other text translations
 * as well as this table, to maximise the number of different ways
 * you can select a supported charset.
 */

#include <ctype.h>
#include "charset.h"
#include "internal.h"

static const struct {
    const char *name;
    int charset;
    int return_in_enum;   /* enumeration misses some charsets */
} localencs[] = {
    { "<UNKNOWN>", CS_NONE, 0 },
    { "ASCII", CS_ASCII, 1 },
    { "BS 4730", CS_BS4730, 1 },
    { "ISO-8859-1", CS_ISO8859_1, 1 },
    { "ISO-8859-1 with X11 line drawing", CS_ISO8859_1_X11, 0 },
    { "ISO-8859-2", CS_ISO8859_2, 1 },
    { "ISO-8859-3", CS_ISO8859_3, 1 },
    { "ISO-8859-4", CS_ISO8859_4, 1 },
    { "ISO-8859-5", CS_ISO8859_5, 1 },
    { "ISO-8859-6", CS_ISO8859_6, 1 },
    { "ISO-8859-7", CS_ISO8859_7, 1 },
    { "ISO-8859-8", CS_ISO8859_8, 1 },
    { "ISO-8859-9", CS_ISO8859_9, 1 },
    { "ISO-8859-10", CS_ISO8859_10, 1 },
    { "ISO-8859-11", CS_ISO8859_11, 1 },
    { "ISO-8859-13", CS_ISO8859_13, 1 },
    { "ISO-8859-14", CS_ISO8859_14, 1 },
    { "ISO-8859-15", CS_ISO8859_15, 1 },
    { "ISO-8859-16", CS_ISO8859_16, 1 },
    { "CP437", CS_CP437, 1 },
    { "CP850", CS_CP850, 1 },
    { "CP866", CS_CP866, 1 },
    { "CP1250", CS_CP1250, 1 },
    { "Win1250", CS_CP1250, 0 },
    { "CP1251", CS_CP1251, 1 },
    { "Win1251", CS_CP1251, 0 },
    { "CP1252", CS_CP1252, 1 },
    { "Win1252", CS_CP1252, 0 },
    { "CP1253", CS_CP1253, 1 },
    { "Win1253", CS_CP1253, 0 },
    { "CP1254", CS_CP1254, 1 },
    { "Win1254", CS_CP1254, 0 },
    { "CP1255", CS_CP1255, 1 },
    { "Win1255", CS_CP1255, 0 },
    { "CP1256", CS_CP1256, 1 },
    { "Win1256", CS_CP1256, 0 },
    { "CP1257", CS_CP1257, 1 },
    { "Win1257", CS_CP1257, 0 },
    { "CP1258", CS_CP1258, 1 },
    { "Win1258", CS_CP1258, 0 },
    { "KOI8-R", CS_KOI8_R, 1 },
    { "KOI8-U", CS_KOI8_U, 1 },
    { "KOI8-RU", CS_KOI8_RU, 1 },
    { "JIS X 0201", CS_JISX0201, 1 },
    { "JIS-X-0201", CS_JISX0201, 0 },
    { "JIS_X_0201", CS_JISX0201, 0 },
    { "JISX0201", CS_JISX0201, 0 },
    { "Mac Roman", CS_MAC_ROMAN, 1 },
    { "Mac Turkish", CS_MAC_TURKISH, 1 },
    { "Mac Croatian", CS_MAC_CROATIAN, 1 },
    { "Mac Iceland", CS_MAC_ICELAND, 1 },
    { "Mac Romanian", CS_MAC_ROMANIAN, 1 },
    { "Mac Greek", CS_MAC_GREEK, 1 },
    { "Mac Cyrillic", CS_MAC_CYRILLIC, 1 },
    { "Mac Thai", CS_MAC_THAI, 1 },
    { "Mac Centeuro", CS_MAC_CENTEURO, 1 },
    { "Mac Symbol", CS_MAC_SYMBOL, 1 },
    { "Mac Dingbats", CS_MAC_DINGBATS, 1 },
    { "Mac Roman (old)", CS_MAC_ROMAN_OLD, 0 },
    { "Mac Croatian (old)", CS_MAC_CROATIAN_OLD, 0 },
    { "Mac Iceland (old)", CS_MAC_ICELAND_OLD, 0 },
    { "Mac Romanian (old)", CS_MAC_ROMANIAN_OLD, 0 },
    { "Mac Greek (old)", CS_MAC_GREEK_OLD, 0 },
    { "Mac Cyrillic (old)", CS_MAC_CYRILLIC_OLD, 0 },
    { "Mac Ukraine", CS_MAC_UKRAINE, 1 },
    { "Mac VT100", CS_MAC_VT100, 1 },
    { "Mac VT100 (old)", CS_MAC_VT100_OLD, 0 },
    { "VISCII", CS_VISCII, 1 },
    { "HP ROMAN8", CS_HP_ROMAN8, 1 },
    { "DEC MCS", CS_DEC_MCS, 1 },
    { "DEC graphics", CS_DEC_GRAPHICS, 1 },
    { "DEC-graphics", CS_DEC_GRAPHICS, 0 },
    { "DECgraphics", CS_DEC_GRAPHICS, 0 },
    { "UTF-8", CS_UTF8, 1 },
    { "UTF-7", CS_UTF7, 1 },
    { "UTF-7-conservative", CS_UTF7_CONSERVATIVE, 0 },
    { "EUC-CN", CS_EUC_CN, 1 },
    { "EUC-KR", CS_EUC_KR, 1 },
    { "EUC-JP", CS_EUC_JP, 1 },
    { "EUC-TW", CS_EUC_TW, 1 },
    { "ISO-2022-JP", CS_ISO2022_JP, 1 },
    { "ISO-2022-KR", CS_ISO2022_KR, 1 },
    { "Big5", CS_BIG5, 1 },
    { "Shift-JIS", CS_SHIFT_JIS, 1 },
    { "HZ", CS_HZ, 1 },
    { "UTF-16BE", CS_UTF16BE, 1 },
    { "UTF-16LE", CS_UTF16LE, 1 },
    { "UTF-16", CS_UTF16, 1 },
    { "CP949", CS_CP949, 1 },
    { "PDFDocEncoding", CS_PDF, 1 },
    { "StandardEncoding", CS_PSSTD, 1 },
    { "COMPOUND_TEXT", CS_CTEXT, 1 },
    { "COMPOUND-TEXT", CS_CTEXT, 0 },
    { "COMPOUND TEXT", CS_CTEXT, 0 },
    { "COMPOUNDTEXT", CS_CTEXT, 0 },
    { "CTEXT", CS_CTEXT, 0 },
    { "ISO-2022", CS_ISO2022, 1 },
    { "ISO2022", CS_ISO2022, 0 },
};

const char *charset_to_localenc(int charset)
{
    int i;

    for (i = 0; i < (int)lenof(localencs); i++)
	if (charset == localencs[i].charset)
	    return localencs[i].name;

    return NULL;		       /* not found */
}

int charset_from_localenc(const char *name)
{
    int i;

    if ( (i = charset_from_mimeenc(name)) != CS_NONE)
	return i;
    if ( (i = charset_from_xenc(name)) != CS_NONE)
	return i;
    if ( (i = charset_from_emacsenc(name)) != CS_NONE)
	return i;

    for (i = 0; i < (int)lenof(localencs); i++) {
	const char *p, *q;
	p = name;
	q = localencs[i].name;
	while (*p || *q) {
	    if (tolower(*p) != tolower(*q))
		break;
	    p++; q++;
	}
	if (!*p && !*q)
	    return localencs[i].charset;
    }

    return CS_NONE;		       /* not found */
}

int charset_localenc_nth(int n)
{
    int i;

    for (i = 0; i < (int)lenof(localencs); i++)
	if (localencs[i].return_in_enum && !n--)
	    return localencs[i].charset;

    return CS_NONE;		       /* end of list */
}
