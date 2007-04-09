/*
 * charset.h - header file for general character set conversion
 * routines.
 */

#ifndef charset_charset_h
#define charset_charset_h

#include <stddef.h>

/*
 * Enumeration that lists all the multibyte or single-byte
 * character sets known to this library.
 */
typedef enum {
    CS_NONE,			       /* used for reporting errors, etc */
    CS_ASCII,			       /* ordinary US-ASCII is worth having! */
    CS_ISO8859_1,
    CS_ISO8859_1_X11,		       /* X font encoding with VT100 glyphs */
    CS_ISO8859_2,
    CS_ISO8859_3,
    CS_ISO8859_4,
    CS_ISO8859_5,
    CS_ISO8859_6,
    CS_ISO8859_7,
    CS_ISO8859_8,
    CS_ISO8859_9,
    CS_ISO8859_10,
    CS_ISO8859_11,
    CS_ISO8859_13,
    CS_ISO8859_14,
    CS_ISO8859_15,
    CS_ISO8859_16,
    CS_CP437,
    CS_CP850,
    CS_CP866,
    CS_CP1250,
    CS_CP1251,
    CS_CP1252,
    CS_CP1253,
    CS_CP1254,
    CS_CP1255,
    CS_CP1256,
    CS_CP1257,
    CS_CP1258,
    CS_KOI8_R,
    CS_KOI8_U,
    CS_KOI8_RU,
    CS_JISX0201,
    CS_MAC_ROMAN,
    CS_MAC_TURKISH,
    CS_MAC_CROATIAN,
    CS_MAC_ICELAND,
    CS_MAC_ROMANIAN,
    CS_MAC_GREEK,
    CS_MAC_CYRILLIC,
    CS_MAC_THAI,
    CS_MAC_CENTEURO,
    CS_MAC_SYMBOL,
    CS_MAC_DINGBATS,
    CS_MAC_ROMAN_OLD,
    CS_MAC_CROATIAN_OLD,
    CS_MAC_ICELAND_OLD,
    CS_MAC_ROMANIAN_OLD,
    CS_MAC_GREEK_OLD,
    CS_MAC_CYRILLIC_OLD,
    CS_MAC_UKRAINE,
    CS_MAC_VT100,
    CS_MAC_VT100_OLD,
    CS_VISCII,
    CS_HP_ROMAN8,
    CS_DEC_MCS,
    CS_UTF8,
    CS_UTF7,
    CS_UTF7_CONSERVATIVE,
    CS_UTF16,
    CS_UTF16BE,
    CS_UTF16LE,
    CS_EUC_JP,
    CS_EUC_CN,
    CS_EUC_KR,
    CS_ISO2022_JP,
    CS_ISO2022_KR,
    CS_BIG5,
    CS_SHIFT_JIS,
    CS_HZ,
    CS_CP949,
    CS_PDF,
    CS_PSSTD,
    CS_CTEXT,
    CS_ISO2022,
    CS_BS4730,
    CS_DEC_GRAPHICS,
    CS_EUC_TW
} charset_t;

typedef struct {
    unsigned long s0, s1;
} charset_state;

/*
 * This macro is used to initialise a charset_state structure:
 * 
 *   charset_state mystate = CHARSET_INIT_STATE;
 */
#define CHARSET_INIT_STATE { 0L, 0L }  /* a suitable initialiser */

/*
 * This external variable contains the same data, but is provided
 * for easy structure-copy assignment:
 * 
 *   mystate = charset_init_state;
 */
extern const charset_state charset_init_state;

/*
 * Routine to convert a MB/SB character set to Unicode.
 * 
 * This routine accepts some number of bytes, updates a state
 * variable, and outputs some number of Unicode characters. There
 * are no guarantees. You can't even guarantee that at most one
 * Unicode character will be output per byte you feed in; for
 * example, suppose you're reading UTF-8, you've seen E1 80, and
 * then you suddenly see FE. Now you need to output _two_ error
 * characters - one for the incomplete sequence E1 80, and one for
 * the completely invalid UTF-8 byte FE.
 * 
 * Returns the number of wide characters output; will never output
 * more than the size of the buffer (as specified on input).
 * Advances the `input' pointer and decrements `inlen', to indicate
 * how far along the input string it got.
 * 
 * The sequence of `errlen' wide characters pointed to by `errstr'
 * will be used to indicate a conversion error. If `errstr' is
 * NULL, `errlen' will be ignored, and the library will choose
 * something sensible to do on its own. For Unicode, this will be
 * U+FFFD (REPLACEMENT CHARACTER).
 */

int charset_to_unicode(const char **input, int *inlen,
		       wchar_t *output, int outlen,
		       int charset, charset_state *state,
		       const wchar_t *errstr, int errlen);

/*
 * Routine to convert Unicode to an MB/SB character set.
 * 
 * This routine accepts some number of Unicode characters, updates
 * a state variable, and outputs some number of bytes.
 * 
 * Returns the number of bytes output; will never output more than
 * the size of the buffer (as specified on input), and will never
 * output a partial MB character. Advances the `input' pointer and
 * decrements `inlen', to indicate how far along the input string
 * it got.
 * 
 * If `error' is non-NULL and a character is found which cannot be
 * expressed in the output charset, conversion will terminate at
 * that character (so `input' points to the offending character)
 * and `*error' will be set to TRUE; if `error' is non-NULL and no
 * difficult characters are encountered, `*error' will be set to
 * FALSE. If `error' is NULL, difficult characters will simply be
 * ignored.
 * 
 * If `input' is NULL, this routine will output the necessary bytes
 * to reset the encoding state in any way which might be required
 * at the end of an output piece of text.
 */

int charset_from_unicode(const wchar_t **input, int *inlen,
			 char *output, int outlen,
			 int charset, charset_state *state, int *error);

/*
 * Convert X11 encoding names to and from our charset identifiers.
 */
const char *charset_to_xenc(int charset);
int charset_from_xenc(const char *name);

/*
 * Convert MIME encoding names to and from our charset identifiers.
 */
const char *charset_to_mimeenc(int charset);
int charset_from_mimeenc(const char *name);

/*
 * Convert our own encoding names to and from our charset
 * identifiers.
 */
const char *charset_to_localenc(int charset);
int charset_from_localenc(const char *name);
int charset_localenc_nth(int n);

/*
 * Convert Mac OS script/region/font to our charset identifiers.
 */
int charset_from_macenc(int script, int region, int sysvers,
			const char *fontname);

/*
 * Convert GNU Emacs coding system symbol to and from our charset
 * identifiers.
 */
const char *charset_to_emacsenc(int charset);
int charset_from_emacsenc(const char *name);

/*
 * Upgrade a charset identifier to a superset charset which is
 * often confused with it. For example, people whose MUAs report
 * their mail as ASCII or ISO8859-1 often in practice turn out to
 * be using CP1252 quote characters, so when parsing incoming mail
 * it is prudent to treat ASCII and ISO8859-1 as aliases for CP1252
 * - and since it's a superset of both, this will cause no
 * genuinely correct mail to be parsed wrongly.
 */
int charset_upgrade(int charset);

/*
 * This function returns TRUE if the input charset is a vaguely
 * sensible superset of ASCII. That is, it returns FALSE for 7-bit
 * encoding formats such as HZ and UTF-7.
 */
int charset_contains_ascii(int charset);

/*
 * This function tries to deduce the CS_* identifier of the charset
 * used in the current C locale. It falls back to CS_ASCII if it
 * can't figure it out at all, so it will always return a valid
 * charset.
 * 
 * (Note that you should have already called setlocale(LC_CTYPE,
 * "") to guarantee that this function will do the right thing.)
 */
int charset_from_locale(void);

#endif /* charset_charset_h */
