/*
 * mimeenc.c - translate our internal character set codes to and
 * from MIME standard character-set names.
 * 
 */

#include <ctype.h>
#include "charset.h"
#include "internal.h"

static const struct {
    const char *name;
    int charset;
} mimeencs[] = {
    /*
     * Most of these names are taken from
     * 
     *   http://www.iana.org/assignments/character-sets
     * 
     * Where multiple encoding names map to the same encoding id
     * (such as the variety of aliases for ISO-8859-1), the first
     * is considered canonical and will be returned when
     * translating the id to a string.
     * 
     * I also list here a few names which aren't in the above web
     * page, but which I've seen in the wild in real mail. These
     * are marked with a comment saying WILD.
     */

    { "US-ASCII", CS_ASCII },
    { "ANSI_X3.4-1968", CS_ASCII },
    { "iso-ir-6", CS_ASCII },
    { "ANSI_X3.4-1986", CS_ASCII },
    { "ISO_646.irv:1991", CS_ASCII },
    { "ASCII", CS_ASCII },
    { "ISO646-US", CS_ASCII },
    { "us", CS_ASCII },
    { "IBM367", CS_ASCII },
    { "cp367", CS_ASCII },
    { "csASCII", CS_ASCII },
    { "646", CS_ASCII },	       /* WILD */

    { "BS_4730", CS_BS4730 },
    { "iso-ir-4", CS_BS4730 },
    { "ISO646-GB", CS_BS4730 },
    { "gb", CS_BS4730 },
    { "uk", CS_BS4730 },
    { "csISO4UnitedKingdom", CS_BS4730 },

    { "ISO-8859-1", CS_ISO8859_1 },
    { "ISO8859-1", CS_ISO8859_1 },     /* WILD */
    { "iso-ir-100", CS_ISO8859_1 },
    { "ISO_8859-1", CS_ISO8859_1 },
    { "ISO_8859-1:1987", CS_ISO8859_1 },
    { "latin1", CS_ISO8859_1 },
    { "l1", CS_ISO8859_1 },
    { "IBM819", CS_ISO8859_1 },
    { "CP819", CS_ISO8859_1 },
    { "csISOLatin1", CS_ISO8859_1 },

    { "ISO-8859-2", CS_ISO8859_2 },
    { "ISO8859-2", CS_ISO8859_2 },     /* WILD */
    { "ISO_8859-2:1987", CS_ISO8859_2 },
    { "iso-ir-101", CS_ISO8859_2 },
    { "ISO_8859-2", CS_ISO8859_2 },
    { "latin2", CS_ISO8859_2 },
    { "l2", CS_ISO8859_2 },
    { "csISOLatin2", CS_ISO8859_2 },

    { "ISO-8859-3", CS_ISO8859_3 },
    { "ISO8859-3", CS_ISO8859_3 },     /* WILD */
    { "ISO_8859-3:1988", CS_ISO8859_3 },
    { "iso-ir-109", CS_ISO8859_3 },
    { "ISO_8859-3", CS_ISO8859_3 },
    { "latin3", CS_ISO8859_3 },
    { "l3", CS_ISO8859_3 },
    { "csISOLatin3", CS_ISO8859_3 },

    { "ISO-8859-4", CS_ISO8859_4 },
    { "ISO8859-4", CS_ISO8859_4 },     /* WILD */
    { "ISO_8859-4:1988", CS_ISO8859_4 },
    { "iso-ir-110", CS_ISO8859_4 },
    { "ISO_8859-4", CS_ISO8859_4 },
    { "latin4", CS_ISO8859_4 },
    { "l4", CS_ISO8859_4 },
    { "csISOLatin4", CS_ISO8859_4 },

    { "ISO-8859-5", CS_ISO8859_5 },
    { "ISO8859-5", CS_ISO8859_5 },     /* WILD */
    { "ISO_8859-5:1988", CS_ISO8859_5 },
    { "iso-ir-144", CS_ISO8859_5 },
    { "ISO_8859-5", CS_ISO8859_5 },
    { "cyrillic", CS_ISO8859_5 },
    { "csISOLatinCyrillic", CS_ISO8859_5 },

    { "ISO-8859-6", CS_ISO8859_6 },
    { "ISO8859-6", CS_ISO8859_6 },     /* WILD */
    { "ISO_8859-6:1987", CS_ISO8859_6 },
    { "iso-ir-127", CS_ISO8859_6 },
    { "ISO_8859-6", CS_ISO8859_6 },
    { "ECMA-114", CS_ISO8859_6 },
    { "ASMO-708", CS_ISO8859_6 },
    { "arabic", CS_ISO8859_6 },
    { "csISOLatinArabic", CS_ISO8859_6 },

    { "ISO-8859-7", CS_ISO8859_7 },
    { "ISO8859-7", CS_ISO8859_7 },     /* WILD */
    { "ISO_8859-7:1987", CS_ISO8859_7 },
    { "iso-ir-126", CS_ISO8859_7 },
    { "ISO_8859-7", CS_ISO8859_7 },
    { "ELOT_928", CS_ISO8859_7 },
    { "ECMA-118", CS_ISO8859_7 },
    { "greek", CS_ISO8859_7 },
    { "greek8", CS_ISO8859_7 },
    { "csISOLatinGreek", CS_ISO8859_7 },

    { "ISO-8859-8", CS_ISO8859_8 },
    { "ISO8859-8", CS_ISO8859_8 },     /* WILD */
    { "ISO_8859-8:1988", CS_ISO8859_8 },
    { "iso-ir-138", CS_ISO8859_8 },
    { "ISO_8859-8", CS_ISO8859_8 },
    { "hebrew", CS_ISO8859_8 },
    { "csISOLatinHebrew", CS_ISO8859_8 },

    { "ISO-8859-9", CS_ISO8859_9 },
    { "ISO8859-9", CS_ISO8859_9 },     /* WILD */
    { "ISO_8859-9:1989", CS_ISO8859_9 },
    { "iso-ir-148", CS_ISO8859_9 },
    { "ISO_8859-9", CS_ISO8859_9 },
    { "latin5", CS_ISO8859_9 },
    { "l5", CS_ISO8859_9 },
    { "csISOLatin5", CS_ISO8859_9 },

    { "ISO-8859-10", CS_ISO8859_10 },
    { "ISO8859-10", CS_ISO8859_10 },   /* WILD */
    { "iso-ir-157", CS_ISO8859_10 },
    { "l6", CS_ISO8859_10 },
    { "ISO_8859-10:1992", CS_ISO8859_10 },
    { "csISOLatin6", CS_ISO8859_10 },
    { "latin6", CS_ISO8859_10 },

    { "TIS-620", CS_ISO8859_11 },

    { "ISO-8859-13", CS_ISO8859_13 },
    { "ISO8859-13", CS_ISO8859_13 },   /* WILD */

    { "ISO-8859-14", CS_ISO8859_14 },
    { "ISO8859-14", CS_ISO8859_14 },   /* WILD */
    { "iso-ir-199", CS_ISO8859_14 },
    { "ISO_8859-14:1998", CS_ISO8859_14 },
    { "ISO_8859-14", CS_ISO8859_14 },
    { "latin8", CS_ISO8859_14 },
    { "iso-celtic", CS_ISO8859_14 },
    { "l8", CS_ISO8859_14 },

    { "ISO-8859-15", CS_ISO8859_15 },
    { "ISO8859-15", CS_ISO8859_15 },   /* WILD */
    { "ISO_8859-15", CS_ISO8859_15 },
    { "Latin-9", CS_ISO8859_15 },

    { "ISO-8859-16", CS_ISO8859_16 },
    { "ISO8859-16", CS_ISO8859_16 },   /* WILD */
    { "iso-ir-226", CS_ISO8859_16 },
    { "ISO_8859-16", CS_ISO8859_16 },
    { "ISO_8859-16:2001", CS_ISO8859_16 },
    { "latin10", CS_ISO8859_16 },
    { "l10", CS_ISO8859_16 },

    { "IBM437", CS_CP437 },
    { "cp437", CS_CP437 },
    { "437", CS_CP437 },
    { "csPC8CodePage437", CS_CP437 },

    { "IBM850", CS_CP850 },
    { "cp850", CS_CP850 },
    { "850", CS_CP850 },
    { "csPC850Multilingual", CS_CP850 },

    { "IBM866", CS_CP866 },
    { "cp866", CS_CP866 },
    { "866", CS_CP866 },
    { "csIBM866", CS_CP866 },

    { "windows-1250", CS_CP1250 },
    { "win-1250", CS_CP1250 },	       /* WILD */

    { "windows-1251", CS_CP1251 },
    { "win-1251", CS_CP1251 },	       /* WILD */

    { "windows-1252", CS_CP1252 },
    { "win-1252", CS_CP1252 },	       /* WILD */

    { "windows-1253", CS_CP1253 },
    { "win-1253", CS_CP1253 },	       /* WILD */

    { "windows-1254", CS_CP1254 },
    { "win-1254", CS_CP1254 },	       /* WILD */

    { "windows-1255", CS_CP1255 },
    { "win-1255", CS_CP1255 },	       /* WILD */

    { "windows-1256", CS_CP1256 },
    { "win-1256", CS_CP1256 },	       /* WILD */

    { "windows-1257", CS_CP1257 },
    { "win-1257", CS_CP1257 },	       /* WILD */

    { "windows-1258", CS_CP1258 },
    { "win-1258", CS_CP1258 },	       /* WILD */

    { "KOI8-R", CS_KOI8_R },
    { "csKOI8R", CS_KOI8_R },

    { "KOI8-U", CS_KOI8_U },

    { "KOI8-RU", CS_KOI8_RU },	       /* WILD */

    { "JIS_X0201", CS_JISX0201 },
    { "X0201", CS_JISX0201 },
    { "csHalfWidthKatakana", CS_JISX0201 },

    { "macintosh", CS_MAC_ROMAN_OLD },
    { "mac", CS_MAC_ROMAN_OLD },
    { "csMacintosh", CS_MAC_ROMAN_OLD },

    { "VISCII", CS_VISCII },
    { "csVISCII", CS_VISCII },

    { "hp-roman8", CS_HP_ROMAN8 },
    { "roman8", CS_HP_ROMAN8 },
    { "r8", CS_HP_ROMAN8 },
    { "csHPRoman8", CS_HP_ROMAN8 },

    { "DEC-MCS", CS_DEC_MCS },
    { "dec", CS_DEC_MCS },
    { "csDECMCS", CS_DEC_MCS },

    { "UTF-8", CS_UTF8 },

    { "UTF-7", CS_UTF7 },
    { "UNICODE-1-1-UTF-7", CS_UTF7 },
    { "csUnicode11UTF7", CS_UTF7 },

    /*
     * Quite why the EUC-CN encoding is known to MIME by the name
     * of its underlying character set, I'm not entirely sure, but
     * it is. Shrug.
     */
    { "GB2312", CS_EUC_CN },
    { "csGB2312", CS_EUC_CN },

    { "EUC-KR", CS_EUC_KR },
    { "csEUCKR", CS_EUC_KR },

    { "EUC-JP", CS_EUC_JP },
    { "csEUCPkdFmtJapanese", CS_EUC_JP },
    { "Extended_UNIX_Code_Packed_Format_for_Japanese", CS_EUC_JP },

    { "ISO-2022-JP", CS_ISO2022_JP },
    { "csISO2022JP", CS_ISO2022_JP },

    { "ISO-2022-KR", CS_ISO2022_KR },
    { "csISO2022KR", CS_ISO2022_KR },

    { "Big5", CS_BIG5 },
    { "csBig5", CS_BIG5 },
    { "Big-5", CS_BIG5 },	       /* WILD */
    { "ChineseBig5", CS_BIG5 },	       /* WILD */

    { "Shift_JIS", CS_SHIFT_JIS },
    { "MS_Kanji", CS_SHIFT_JIS },
    { "csShiftJIS", CS_SHIFT_JIS },

    { "HZ-GB-2312", CS_HZ },

    { "UTF-16BE", CS_UTF16BE },

    { "UTF-16LE", CS_UTF16LE },

    { "UTF-16", CS_UTF16 },

    /*
     * This bit is fiddly and possibly technically incorrect; but
     * rumour has it that the KSC 5601 encoding is a subset of
     * Microsoft CP949, and that MS products tend to announce CP949
     * as KSC 5601 in much the same way they seem willing to
     * announce CP1252 as its subset ISO 8859-1. So I cheat
     * shamelessly here by letting KSC 5601 map to CP949.
     */
    { "KS_C_5601-1987", CS_CP949 },
    { "iso-ir-149", CS_CP949 },
    { "KS_C_5601-1989", CS_CP949 },
    { "KSC_5601", CS_CP949 },
    { "korean", CS_CP949 },
    { "csKSC56011987", CS_CP949 },
    { "KSC5601", CS_CP949 },	       /* WILD */

#if 0
    { "ISO-2022-JP-2", CS_ISO2022_JP_2 },
    { "csISO2022JP2", CS_ISO2022_JP_2 },
#endif
};

const char *charset_to_mimeenc(int charset)
{
    int i;

    for (i = 0; i < (int)lenof(mimeencs); i++)
	if (charset == mimeencs[i].charset)
	    return mimeencs[i].name;

    return NULL;		       /* not found */
}

int charset_from_mimeenc(const char *name)
{
    int i;

    for (i = 0; i < (int)lenof(mimeencs); i++) {
	const char *p, *q;
	p = name;
	q = mimeencs[i].name;
	while (*p || *q) {
	    if (tolower(*p) != tolower(*q))
		break;
	    p++; q++;
	}
	if (!*p && !*q)
	    return mimeencs[i].charset;
    }

    return CS_NONE;		       /* not found */
}
