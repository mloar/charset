/*
 * superset.c: deal with character sets which are supersets of
 * others.
 */

#include "charset.h"

/*
 * Just in case it's ever useful again, this rather simplistic
 * piece of Perl/sh analyses sbcs.dat and determines which pairs of
 * character sets are identical in the A0-FF region. This doesn't
 * prove supersethood, but it spots obvious cases.

perl -ne '/^[^ ]{4} / and defined ($line) and $line < 16 and do {' \
      -e '  chomp; print " $_" if $line>=10; print "\n" if ++$line==16; };' \
      -e '/^charset (.*)$/ and do { $line = 0; printf "%30s:", $1; };' \
  sbcs.dat | sort +1 | uniq -f1 -D

 * When run on sbcs.dat rev 1.3, it reports only two sets of matches:
 * 
 *  - ISO8859_1, ISO8859_1_X11 and CP1252 all match.
 *  - ISO8859_4 and CP1254 match.
 * 
 * FIXME: There is more to it than this, and in particular there's
 * even more to it than simple subsethood. Look at CP1255 and
 * ISO8859_8: they match at every code point defined in both, but
 * they each define at least one code point the other doesn't. It
 * isn't clear how I should handle this. The right thing might be
 * to define yet another SBCS which is the union of both, and
 * upgrade both to that. Or it might be that the unicode.org
 * mapping table for CP1255 is simply out of date, and the mapping
 * ISO8859_8 has which it doesn't (DF -> U+2017 DOUBLE LOW LINE)
 * should be present in it too, which would make it a proper
 * superset of ISO8859_8 and solve the problem.
 * 
 * However, for the moment I'm satisfied with enhancing this table
 * as and when necessary; the idea is not to include _all_ superset
 * relations here, the idea is to spot charset IDs which are used
 * _in practice_ to mean other charset IDs. So unless and until I
 * find out that there really is confusion between ISO8859_8 and
 * CP1255, I don't need to do anything about it here.
 */

int charset_upgrade(int charset)
{
    if (charset == CS_ASCII || charset == CS_ISO8859_1)
	charset = CS_CP1252;
    if (charset == CS_ISO8859_4)
	charset = CS_CP1254;
    if (charset == CS_EUC_KR)
	charset = CS_CP949;
    return charset;
}

/*
 * This function returns TRUE if the input charset is a vaguely
 * sensible superset of ASCII. That is, it returns FALSE for 7-bit
 * encoding formats such as HZ and UTF-7.
 */
int charset_contains_ascii(int charset)
{
    return (charset != CS_HZ &&
	    charset != CS_UTF7 &&
	    charset != CS_UTF7_CONSERVATIVE);
}
