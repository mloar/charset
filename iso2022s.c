/*
 * iso2022s.c - support for ISO-2022 subset encodings.
 */

#ifndef ENUM_CHARSETS

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "charset.h"
#include "internal.h"
#include "sbcsdat.h"

#define SO (0x0E)
#define SI (0x0F)
#define ESC (0x1B)

/* Functional description of a single ISO 2022 escape sequence. */
struct iso2022_escape {
    char const *sequence;
    unsigned long andbits, xorbits;
    /*
     * For output, these variables help us figure out which escape
     * sequences we need to get where we want to be.
     * 
     * `container' should be in the range 0-3, but can also be ORed
     * with the bit flag RO to indicate that this is not a
     * preferred container to use for this charset during output.
     */
    int container, subcharset;
};
#define RO 0x80

struct iso2022 {
    /*
     * List of escape sequences supported in this subset. Must be
     * in ASCII order, so that we can narrow down the list as
     * necessary.
     */
    const struct iso2022_escape *escapes;/* must be sorted in ASCII order! */
    int nescapes;

    /*
     * We assign indices from 0 upwards to the sub-charsets of a
     * given ISO 2022 subset. nbytes[i] tells us how many bytes per
     * character are required by sub-charset i. (It's a string
     * mainly because that makes it easier to declare in C syntax
     * than an int array.)
     */
    char const *nbytes;

    /*
     * The characters in this string are indices-plus-one (so that
     * NUL can still terminate) of escape sequences in `escapes'.
     * These escapes are output in the given sequence to reset the
     * encoding state, unless it turns out that a given escape
     * would not change the state at all.
     */
    char const *reset;

    /*
     * Initial value of s1, in case the default container contents
     * needs to be something other than charset 0 in all cases.
     * (Note that this must have the top bit set!)
     */
    unsigned long s1;

    /*
     * For output, some ISO 2022 subsets _mandate_ an initial shift
     * sequence. If so, here it is so we can output it. (For the
     * sake of basic sanity we won't bother to _require_ it on
     * input, although it should of course be listed under
     * `escapes' above so that we ignore it when present.)
     */
    char const *initial_sequence;

    /*
     * Is this an 8-bit ISO 2022 subset?
     */
    int eightbit;

    /*
     * Function calls to do the actual translation.
     */
    long int (*to_ucs)(int subcharset, unsigned long bytes);
    int (*from_ucs)(long int ucs, int *subcharset, unsigned long *bytes);
};

static void read_iso2022s(charset_spec const *charset, long int input_chr,
			  charset_state *state,
			  void (*emit)(void *ctx, long int output),
			  void *emitctx)
{
    struct iso2022 const *iso = (struct iso2022 *)charset->data;

    /*
     * For reading ISO-2022 subsets, we divide up our state
     * variables as follows:
     * 
     * 	- The top byte of s0 (bits 31:24) indicates, if nonzero,
     * 	  that we are part-way through a recognised ISO-2022 escape
     * 	  sequence. Five of those bits (31:27) give the index of
     * 	  the first member of the escapes list matching what we
     * 	  have so far; the remaining three (26:24) give the number
     * 	  of characters we have seen so far.
     * 
     * 	- The top bit of s1 (bit 31) is non-zero at all times, to
     * 	  indicate that we have performed any necessary
     * 	  initialisation. When we start, we detect a zero s1 and
     * 	  respond to it by initialising the default container
     * 	  contents.
     * 
     * 	- The next three bits of s1 (bits 30:28) indicate which
     * 	  _container_ is currently selected. This isn't quite as
     * 	  simple as it sounds, since we have to preserve memory of
     * 	  which of the SI/SO containers we came from when we're
     * 	  temporarily in SS2/SS3. Hence, what happens is:
     *     + bit 28 indicates SI/SO.
     * 	   + if we're in an SS2/SS3 container, that's indicated by
     * 	     the two bits above that being nonzero and holding
     * 	     either 2 or 3.
     * 	   + Hence: 0 is SI, 1 is SO, 4 is SS2-from-SI, 5 is
     * 	     SS2-from-SO, 6 is SS3-from-SI, 7 is SS3-from-SO.
     * 	   + For added fun: in an _8-bit_ ISO 2022 subset, we have
     * 	     the further special value 2, which means that we're
     * 	     theoretically in SI but the current character being
     * 	     accumulated is composed of 8-bit characters and will
     * 	     therefore be interpreted as if in SO.
     * 
     * 	- The next nibble of s1 (27:24) indicates how many bytes
     * 	  have been accumulated in the current character.
     * 
     * 	- The remaining three bytes of s1 are divided into four
     * 	  six-bit sections, and each section gives the current
     * 	  sub-charset selected in one of the possible containers.
     * 	  (Those containers are SI, SO, SS2 and SS3, respectively
     * 	  and in order from the bottom of s0 to the top.)
     * 
     * 	- The bottom 24 bits of s0 give the accumulated character
     * 	  data so far.
     * 
     * (Note that this means s1 contains all the parts of the state
     * which might need to be operated on by escape sequences.
     * Cunning, eh?)
     */

    if (!(state->s1 & 0x80000000)) {
	state->s1 = iso->s1;
    }

    /*
     * So. Firstly, we process escape sequences, if we're in the
     * middle of one or if we see a possible introducer (SI, SO,
     * ESC).
     */
    if ((state->s0 >> 24) ||
	(input_chr == SO || input_chr == SI || input_chr == ESC)) {
	int n = (state->s0 >> 24) & 7, i = (state->s0 >> 27), oi = i, j;

	/*
	 * If this is the start of an escape sequence, we might be
	 * in mid-character. If so, clear the character state and
	 * emit an error token for the incomplete character.
	 */
	if (state->s1 & 0x0F000000) {
	    state->s1 &= ~0x0F000000;
	    state->s0 &= 0xFF000000;
	    /*
	     * If we were in the SS2 or SS3 container, we
	     * automatically exit it.
	     */
	    if (state->s1 & 0x60000000)
		state->s1 &= 0x9FFFFFFF;
	    emit(emitctx, ERROR);
	}

	j = i;
	while (j < iso->nescapes &&
	       !memcmp(iso->escapes[j].sequence,
		       iso->escapes[oi].sequence, n)) {
	    if (iso->escapes[j].sequence[n] < input_chr)
		i = ++j;
	    else
		break;
	}
	if (i >= iso->nescapes ||
	    memcmp(iso->escapes[i].sequence,
		   iso->escapes[oi].sequence, n) ||
	    iso->escapes[i].sequence[n] != input_chr) {
	    /*
	     * This character does not appear in any valid escape
	     * sequence. Therefore, we must emit all the characters
	     * we had previously swallowed, plus this one, and
	     * return to non-escape-sequence state.
	     */
	    for (j = 0; j < n; j++)
		emit(emitctx, iso->escapes[oi].sequence[j]);
	    emit(emitctx, input_chr);
	    state->s0 = 0;
	    return;
	}

	/*
	 * Otherwise, we have found an additional character in our
	 * escape sequence. See if we have reached the _end_ of our
	 * sequence (and therefore must process the sequence).
	 */
	n++;
	if (!iso->escapes[i].sequence[n]) {
	    state->s0 = 0;
	    state->s1 &= iso->escapes[i].andbits;
	    state->s1 ^= iso->escapes[i].xorbits;
	    return;
	}

	/*
	 * Failing _that_, we simply update our escape-sequence-
	 * tracking state.
	 */
	assert(i < 32 && n < 8);
	state->s0 = (i << 27) | (n << 24);
	return;
    }

    /*
     * If this isn't an escape sequence, it must be part of a
     * character. One possibility is that it's a control character
     * (00-20 or 7F-9F; also in non-8-bit ISO 2022 subsets I'm
     * going to treat all top-half characters as controls), in
     * which case we output it verbatim.
     */
    if (input_chr < 0x21 ||
	(input_chr > 0x7E && (!iso->eightbit || input_chr < 0xA0))) {
	/*
	 * We might be in mid-multibyte-character. If so, clear the
	 * character state and emit an error token for the
	 * incomplete character.
	 */
	if (state->s1 & 0x0F000000) {
	    state->s1 &= ~0x0F000000;
	    state->s0 &= 0xFF000000;
	    emit(emitctx, ERROR);
	    /*
	     * If we were in the SS2 or SS3 container, we
	     * automatically exit it.
	     */
	    if (state->s1 & 0x60000000)
		state->s1 &= 0x9FFFFFFF;
	}

	emit(emitctx, input_chr);
	return;
    }

    /*
     * Otherwise, accumulate character data.
     */
    {
	unsigned long chr;
	int chrlen, cont, subcharset, bytes;

	/*
	 * Verify that we've seen the right kind of character for
	 * what we're currently doing. This only matters in 8-bit
	 * subsets.
	 */
	if (iso->eightbit) {
	    cont = (state->s1 >> 28) & 7;
	    /*
	     * If cont==0, we're entitled to see either GL or GR
	     * characters. If cont==2, we expect only GR; otherwise
	     * we expect only GL.
	     * 
	     * If we see a GR character while cont==0, we set
	     * cont=2 immediately.
	     */
	    if ((cont == 2 && !(input_chr & 0x80)) ||
		(cont != 0 && cont != 2 && (input_chr & 0x80))) {
		/*
		 * Clear the previous character; it was prematurely
		 * terminated by this error.
		 */
		state->s1 &= ~0x0F000000;
		state->s0 &= 0xFF000000;
		emit(emitctx, ERROR);
		/*
		 * If we were in the SS2 or SS3 container, we
		 * automatically exit it.
		 */
		if (state->s1 & 0x60000000)
		    state->s1 &= 0x9FFFFFFF;
	    }

	    if (cont == 0 && (input_chr & 0x80)) {
		state->s1 |= 0x20000000;
	    }
	}

	/* The current character and its length. */
	chr = ((state->s0 & 0x00FFFFFF) << 8) | (input_chr & 0x7F);
	chrlen = ((state->s1 >> 24) & 0xF) + 1;
	/* The current sub-charset. */
	cont = (state->s1 >> 28) & 7;
	if (cont > 1) cont >>= 1;
	subcharset = (state->s1 >> (6*cont)) & 0x3F;
	/* The number of bytes-per-character in that sub-charset. */
	bytes = iso->nbytes[subcharset];

	/*
	 * If this character is now complete, we convert and emit
	 * it. Otherwise, we simply update the state and return.
	 */
	if (chrlen >= bytes) {
	    emit(emitctx, iso->to_ucs(subcharset, chr));
	    chr = chrlen = 0;
	    /*
	     * If we were in the SS2 or SS3 container, we
	     * automatically exit it.
	     */
	    if (state->s1 & 0x60000000)
		state->s1 &= 0x9FFFFFFF;
	}
	state->s0 = (state->s0 & 0xFF000000) | chr;
	state->s1 = (state->s1 & 0xF0FFFFFF) | (chrlen << 24);
    }
}

static int write_iso2022s(charset_spec const *charset, long int input_chr,
			  charset_state *state,
			  void (*emit)(void *ctx, long int output),
			  void *emitctx)
{
    struct iso2022 const *iso = (struct iso2022 *)charset->data;
    int subcharset, len, i, j, cont, topbit = 0;
    unsigned long bytes;

    /*
     * For output, our s1 state variable contains most of the same
     * stuff as it did for input - initial-state indicator bit,
     * current container, and current subcharset selected in each
     * container.
     */

    /*
     * Analyse the character and find out what subcharset it needs
     * to go in.
     */
    if (input_chr >= 0 && !iso->from_ucs(input_chr, &subcharset, &bytes))
	return FALSE;

    if (!(state->s1 & 0x80000000)) {
	state->s1 = iso->s1;
	if (iso->initial_sequence)
	    for (i = 0; iso->initial_sequence[i]; i++)
		emit(emitctx, iso->initial_sequence[i]);
    }

    if (input_chr == -1) {
	unsigned long oldstate;
	int k;

	/*
	 * Special case: reset encoding state.
	 */
	for (i = 0; iso->reset[i]; i++) {
	    j = iso->reset[i] - 1;
	    oldstate = state->s1;
	    state->s1 &= iso->escapes[j].andbits;
	    state->s1 ^= iso->escapes[j].xorbits;
	    if (state->s1 != oldstate) {
		/* We must actually emit this sequence. */
		for (k = 0; iso->escapes[j].sequence[k]; k++)
		    emit(emitctx, iso->escapes[j].sequence[k]);
	    }
	}

	return TRUE;
    }

    /*
     * Now begins the fun. We now know what subcharset we want. So
     * we must find out which container we should select it into,
     * select it into it if necessary, select that _container_ if
     * necessary, and then output the given bytes.
     */
    for (i = 0; i < iso->nescapes; i++)
	if (iso->escapes[i].subcharset == subcharset &&
	    !(iso->escapes[i].container & RO))
	    break;
    assert(i < iso->nescapes);

    /*
     * We've found the escape sequence which would select this
     * subcharset into a container. However, that subcharset might
     * already _be_ selected in that container! Check before we go
     * to the effort of emitting the sequence.
     */
    cont = iso->escapes[i].container &~ RO;
    if (((state->s1 >> (6*cont)) & 0x3F) != (unsigned)subcharset) {
	for (j = 0; iso->escapes[i].sequence[j]; j++)
	    emit(emitctx, iso->escapes[i].sequence[j]);
	state->s1 &= iso->escapes[i].andbits;
	state->s1 ^= iso->escapes[i].xorbits;
    }

    /*
     * Now we know what container our subcharset is in, so we want
     * to select that container.
     */
    if (cont > 1) {
	/* SS2 or SS3; just output the sequence and be done. */
	emit(emitctx, ESC);
	emit(emitctx, 'L' + cont);     /* comes out to 'N' or 'O' */
    } else {
	/*
	 * Emit SI or SO, but only if the current container isn't already
	 * the right one.
	 * 
	 * Also, in an 8-bit subset, we need not do this; we'll
	 * just use 8-bit characters to output SO-container
	 * characters.
	 */
	if (iso->eightbit && cont == 1 && ((state->s1 >> 28) & 7) == 0) {
	    topbit = 0x80;
	} else if (((state->s1 >> 28) & 7) != (unsigned)cont) {
	    emit(emitctx, cont ? SO : SI);
	    state->s1 = (state->s1 & 0x8FFFFFFF) | (cont << 28);
	}
    }

    /*
     * We're done. Subcharset is selected in container, container
     * is selected. All we need now is to write out the bytes.
     */
    len = iso->nbytes[subcharset];
    while (len--)
	emit(emitctx, ((bytes >> (8*len)) & 0xFF) | topbit);

    return TRUE;
}

/*
 * ISO-2022-JP, defined in RFC 1468.
 */
static long int iso2022jp_to_ucs(int subcharset, unsigned long bytes)
{
    switch (subcharset) {
      case 1:			       /* JIS X 0201 bottom half */
	if (bytes == 0x5C)
	    return 0xA5;
	else if (bytes == 0x7E)
	    return 0x203E;
	/* else fall through to ASCII */
      case 0: return bytes;	       /* one-byte ASCII */
	/* (no break needed since all control paths have returned) */
      case 2: return jisx0208_to_unicode(((bytes >> 8) & 0xFF) - 0x21,
					 ((bytes     ) & 0xFF) - 0x21);
      default: return ERROR;
    }
}
static int iso2022jp_from_ucs(long int ucs, int *subcharset,
			      unsigned long *bytes)
{
    int r, c;
    if (ucs < 0x80) {
	*subcharset = 0;
	*bytes = ucs;
	return 1;
    } else if (ucs == 0xA5 || ucs == 0x203E) {
	*subcharset = 1;
	*bytes = (ucs == 0xA5 ? 0x5C : 0x7E);
	return 1;
    } else if (unicode_to_jisx0208(ucs, &r, &c)) {
	*subcharset = 2;
	*bytes = ((r+0x21) << 8) | (c+0x21);
	return 1;
    } else {
	return 0;
    }
}
static const struct iso2022_escape iso2022jp_escapes[] = {
    {"\033$@", 0xFFFFFFC0, 0x00000002, -1, -1},   /* we ignore this one */
    {"\033$B", 0xFFFFFFC0, 0x00000002, 0, 2},
    {"\033(B", 0xFFFFFFC0, 0x00000000, 0, 0},
    {"\033(J", 0xFFFFFFC0, 0x00000001, 0, 1},
};
static const struct iso2022 iso2022jp = {
    iso2022jp_escapes, lenof(iso2022jp_escapes),
    "\1\1\2", "\3", 0x80000000, NULL, FALSE,
    iso2022jp_to_ucs, iso2022jp_from_ucs
};
const charset_spec charset_CS_ISO2022_JP = {
    CS_ISO2022_JP, read_iso2022s, write_iso2022s, &iso2022jp
};

/*
 * ISO-2022-KR, defined in RFC 1557.
 */
static long int iso2022kr_to_ucs(int subcharset, unsigned long bytes)
{
    switch (subcharset) {
      case 0: return bytes;	       /* one-byte ASCII */
      case 1: return ksx1001_to_unicode(((bytes >> 8) & 0xFF) - 0x21,
					((bytes     ) & 0xFF) - 0x21);
      default: return ERROR;
    }
}
static int iso2022kr_from_ucs(long int ucs, int *subcharset,
			      unsigned long *bytes)
{
    int r, c;
    if (ucs < 0x80) {
	*subcharset = 0;
	*bytes = ucs;
	return 1;
    } else if (unicode_to_ksx1001(ucs, &r, &c)) {
	*subcharset = 1;
	*bytes = ((r+0x21) << 8) | (c+0x21);
	return 1;
    } else {
	return 0;
    }
}
static const struct iso2022_escape iso2022kr_escapes[] = {
    {"\016", 0x8FFFFFFF, 0x10000000, -1, -1},
    {"\017", 0x8FFFFFFF, 0x00000000, 0, 0},
    {"\033$)C", 0xFFFFF03F, 0x00000040, 1, 1},   /* bits[11:6] <- 1 */
};
static const struct iso2022 iso2022kr = {
    iso2022kr_escapes, lenof(iso2022kr_escapes),
    "\1\2", "\2", 0x80000040, "\033$)C", FALSE,
    iso2022kr_to_ucs, iso2022kr_from_ucs
};
const charset_spec charset_CS_ISO2022_KR = {
    CS_ISO2022_KR, read_iso2022s, write_iso2022s, &iso2022kr
};

#else /* ENUM_CHARSETS */

ENUM_CHARSET(CS_ISO2022_JP)
ENUM_CHARSET(CS_ISO2022_KR)

#endif /* ENUM_CHARSETS */
