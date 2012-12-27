/*
 * libcharset client utility which, given two Unicode code points,
 * will search for character sets which encode the two code points the
 * same way. The idea is that if you see some piece of misencoded text
 * which uses (say) an oe ligature where you expected (as it might be)
 * a pound sign, you can use this utility to suggest which two
 * character sets might have been confused with each other to cause
 * that effect.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#include "charset.h"

#define MAXENCLEN 20

int main(int argc, char **argv)
{
    wchar_t *chars;
    struct enc { char string[MAXENCLEN]; int len; } *encodings;
    int nchars;
    int i, j, k, cs;
    const char *sep;

    setlocale(LC_ALL, "");

    chars = malloc(argc * sizeof(wchar_t));
    if (!chars) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    nchars = 0;

    while (--argc) {
        char *p = *++argv;
        char *orig = p;
        char *end;
        int base = 16, semi_ok = 0;
        wchar_t ch;

        if ((p[0] == 'U' || p[0] == 'u') &&
            (p[1] == '-' || p[1] == '+')) {
            p += 2;
        } else if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
            p += 2;
        } else if (p[0] == '&' && p[1] == '#') {
            p += 2;
            if (p[0] == 'x' || p[0] == 'X')
                p++;
            else
                base = 10;
            semi_ok = 1;
        } else if (mbtowc(&ch, p, strlen(p)) == strlen(p)) {
            chars[nchars++] = ch;
            continue;
        }

        chars[nchars++] = strtoul(p, &end, base);
        if (!*end || (semi_ok && !strcmp(end, ";")))
            continue;
        else {
            fprintf(stderr, "unable to parse '%s' as a Unicode code point\n",
                    orig);
            return 1;
        }
    }

    encodings = malloc(nchars * CS_LIMIT * sizeof(struct enc));
    for (cs = 0; cs < CS_LIMIT; cs++) {
        for (i = 0; i < nchars; i++) {
            wchar_t inbuf[1];
            const wchar_t *inptr;
            int inlen, error, ret;

            if (!charset_exists(cs)) {
                encodings[i*CS_LIMIT+cs].len = 0;
                continue;
            }

            inbuf[0] = chars[i];
            inptr = inbuf;
            inlen = 1;
            error = 0;
            ret = charset_from_unicode(&inptr, &inlen,
                                       encodings[i*CS_LIMIT+cs].string,
                                       MAXENCLEN, cs, NULL, &error);
            if (error || inlen > 0)
                encodings[i*CS_LIMIT+cs].len = 0;
            else
                encodings[i*CS_LIMIT+cs].len = ret;
        }
    }

    /*
     * Really simple and slow approach to finding each distinct string
     * and outputting it.
     */
    for (i = 0; i < nchars*CS_LIMIT; i++) {
        const char *thisstr = encodings[i].string;
        int thislen = encodings[i].len;

        if (thislen == 0)
            continue;
        for (j = 0; j < i; j++)
            if (encodings[j].len == thislen &&
                !memcmp(encodings[j].string, thisstr, thislen))
                break;
        if (j < i)
            continue;        /* not the first instance of this encoding */

        /*
         * See if every character is encoded like this somewhere.
         */
        for (j = 0; j < nchars; j++) {
            for (cs = 0; cs < CS_LIMIT; cs++) {
                if (encodings[j*CS_LIMIT+cs].len == thislen &&
                    !memcmp(encodings[j*CS_LIMIT+cs].string, thisstr, thislen))
                    break;
            }
            if (cs == CS_LIMIT)
                break;                 /* this char not in any cs */
        }
        if (j < nchars)
            continue;                  /* some char not in any cs */

        /*
         * Match! Print the encoding, then all charsets.
         */
        for (j = 0; j < nchars; j++) {
            for (k = 0; k < thislen; k++)
                printf("%s%02X", k>0?" ":"", (unsigned)(thisstr[k] & 0xFF));
            printf(" = ");
            if (chars[j] >= 0x10000)
                printf("U-%08X", (unsigned)chars[j]);
            else
                printf("U+%04X", (unsigned)chars[j]);
            printf(" in:");
            sep = " ";
            for (cs = 0; cs < CS_LIMIT; cs++)
                if (encodings[j*CS_LIMIT+cs].len == thislen &&
                    !memcmp(encodings[j*CS_LIMIT+cs].string, thisstr, thislen))
                {
                    printf("%s%s", sep, charset_to_localenc(cs));
                    sep = ", ";
                }
            printf("\n");
        }
        printf("\n");
    }

    return 0;
}
