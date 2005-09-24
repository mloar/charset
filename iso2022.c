/*
 * iso2022.c - support for ISO/IEC 2022 (alias ECMA-35).
 *
 * This isn't a complete implementation of ISO/IEC 2022, but it's
 * close.  It only handles decoding, because a fully general encoder
 * isn't really useful.  It can decode 8-bit and 7-bit versions, with
 * support for single-byte and multi-byte character sets, all four
 * containers (G0, G1, G2, and G3), using both single-shift and
 * locking-shift sequences.
 *
 * The general principle is that any valid ISO/IEC 2022 sequence
 * should either be correctly decoded or should emit an ERROR.  The
 * only exception to this is that the C0 and C1 sets are fixed as
 * those of ISO/IEC 6429.  Escape sequences for designating control
 * sets are passed through, so a post-processor could fix them up if
 * necessary.
 *
 * DOCS to UTF-8 works.  Other DOCS sequences are ignored, which will
 * produce surprising results.
 */

#ifndef ENUM_CHARSETS

#include <assert.h>

#include "charset.h"
#include "internal.h"
#include "sbcsdat.h"

#define LS1 (0x0E)
#define LS0 (0x0F)
#define ESC (0x1B)
#define SS2 (0x8E)
#define SS3 (0x8F)

enum {S4, S6, M4, M6};

static long int emacs_big5_1_to_unicode(int, int);
static long int emacs_big5_2_to_unicode(int, int);
static long int cns11643_1_to_unicode(int, int);
static long int cns11643_2_to_unicode(int, int);
static long int cns11643_3_to_unicode(int, int);
static long int cns11643_4_to_unicode(int, int);
static long int cns11643_5_to_unicode(int, int);
static long int cns11643_6_to_unicode(int, int);
static long int cns11643_7_to_unicode(int, int);
static long int null_dbcs_to_unicode(int, int);

const struct iso2022_subcharset {
    char type, i, f;
    int offset;
    const sbcs_data *sbcs_base;
    long int (*dbcs_fn)(int, int);
} iso2022_subcharsets[] = {
    { S4, 0, '0', 0x00, &sbcsdata_CS_DEC_GRAPHICS },
    { S4, 0, '<', 0x80, &sbcsdata_CS_DEC_MCS },
    { S4, 0, 'A', 0x00, &sbcsdata_CS_BS4730 },
    { S4, 0, 'B', 0x00, &sbcsdata_CS_ASCII },
    { S4, 0, 'I', 0x80, &sbcsdata_CS_JISX0201 },
    { S4, 0, 'J', 0x00, &sbcsdata_CS_JISX0201 },
    { S4, 0, '~' },
    { S6, 0, 'A', 0x80, &sbcsdata_CS_ISO8859_1 },
    { S6, 0, 'B', 0x80, &sbcsdata_CS_ISO8859_2 },
    { S6, 0, 'C', 0x80, &sbcsdata_CS_ISO8859_3 },
    { S6, 0, 'D', 0x80, &sbcsdata_CS_ISO8859_4 },
    { S6, 0, 'F', 0x80, &sbcsdata_CS_ISO8859_7 },
    { S6, 0, 'G', 0x80, &sbcsdata_CS_ISO8859_6 },
    { S6, 0, 'H', 0x80, &sbcsdata_CS_ISO8859_8 },
    { S6, 0, 'L', 0x80, &sbcsdata_CS_ISO8859_5 },
    { S6, 0, 'M', 0x80, &sbcsdata_CS_ISO8859_9 },
    { S6, 0, 'T', 0x80, &sbcsdata_CS_ISO8859_11 },
    { S6, 0, 'V', 0x80, &sbcsdata_CS_ISO8859_10 },
    { S6, 0, 'Y', 0x80, &sbcsdata_CS_ISO8859_13 },
    { S6, 0, '_', 0x80, &sbcsdata_CS_ISO8859_14 },
    { S6, 0, 'b', 0x80, &sbcsdata_CS_ISO8859_15 },
    { S6, 0, 'f', 0x80, &sbcsdata_CS_ISO8859_16 },
    { S6, 0, '~' }, /* empty 96-set */
#if 0
    { M4, 0, '@' }, /* JIS C 6226-1978 */
#endif
    { M4, 0, '0', -0x21, 0, &emacs_big5_1_to_unicode },
    { M4, 0, '1', -0x21, 0, &emacs_big5_2_to_unicode },
    { M4, 0, 'A', -0x21, 0, &gb2312_to_unicode },
    { M4, 0, 'B', -0x21, 0, &jisx0208_to_unicode },
    { M4, 0, 'C', -0x21, 0, &ksx1001_to_unicode },
    { M4, 0, 'D', -0x21, 0, &jisx0212_to_unicode },
    { M4, 0, 'G', -0x21, 0, &cns11643_1_to_unicode },
    { M4, 0, 'H', -0x21, 0, &cns11643_2_to_unicode },
    { M4, 0, 'I', -0x21, 0, &cns11643_3_to_unicode },
    { M4, 0, 'J', -0x21, 0, &cns11643_4_to_unicode },
    { M4, 0, 'K', -0x21, 0, &cns11643_5_to_unicode },
    { M4, 0, 'L', -0x21, 0, &cns11643_6_to_unicode },
    { M4, 0, 'M', -0x21, 0, &cns11643_7_to_unicode },
    { M4, 0, '~', 0, 0, &null_dbcs_to_unicode }, /* empty 94^n-set */
    { M6, 0, '~', 0, 0, &null_dbcs_to_unicode }, /* empty 96^n-set */
};

static long int null_dbcs_to_unicode(int r, int c)
{
    return ERROR;
}

/*
 * Emacs encodes Big5 in COMPOUND_TEXT as two 94x94 character sets.
 * We treat Big5 as a 94x191 character set with a bunch of undefined
 * columns in the middle, so we have to mess around a bit to make
 * things fit.
 */

static long int emacs_big5_1_to_unicode(int r, int c)
{
    unsigned long s;
    s = r * 94 + c;
    r = s / 157;
    c = s % 157;
    if (c >= 64) c += 34; /* Skip over the gap */
    return big5_to_unicode(r, c);
}

static long int emacs_big5_2_to_unicode(int r, int c)
{
    unsigned long s;
    s = r * 94 + c;
    r = s / 157 + 40;
    c = s % 157;
    if (c >= 64) c += 34; /* Skip over the gap */
    return big5_to_unicode(r, c);
}

/* Wrappers for cns11643_to_unicode() */
static long int cns11643_1_to_unicode(int r, int c)
{
    return cns11643_to_unicode(0, r, c);
}
static long int cns11643_2_to_unicode(int r, int c)
{
    return cns11643_to_unicode(1, r, c);
}
static long int cns11643_3_to_unicode(int r, int c)
{
    return cns11643_to_unicode(2, r, c);
}
static long int cns11643_4_to_unicode(int r, int c)
{
    return cns11643_to_unicode(3, r, c);
}
static long int cns11643_5_to_unicode(int r, int c)
{
    return cns11643_to_unicode(4, r, c);
}
static long int cns11643_6_to_unicode(int r, int c)
{
    return cns11643_to_unicode(5, r, c);
}
static long int cns11643_7_to_unicode(int r, int c)
{
    return cns11643_to_unicode(6, r, c);
}

/* States, or "what we're currently accumulating". */
enum {
    IDLE,	/* None of the below */
    SS2CHAR,	/* Accumulating a character after SS2 */
    SS3CHAR,	/* Accumulating a character after SS3 */
    ESCSEQ,	/* Accumulating an escape sequence */
    ESCDROP,	/* Discarding an escape sequence */
    ESCPASS,	/* Passing through an escape sequence */
    DOCSUTF8,	/* DOCSed into UTF-8 */
    DOCSCTEXT	/* DOCSed into a COMPOUND_TEXT extended segment */
};

#if 0
#include <stdio.h>
static void dump_state(charset_state *s)
{
    unsigned s0 = s->s0, s1 = s->s1;
    char const * const modes[] = { "IDLE", "SS2CHAR", "SS3CHAR",
				   "ESCSEQ", "ESCDROP", "ESCPASS",
				   "DOCSUTF8" };

    fprintf(stderr, "s0: %s", modes[s0 >> 29]);
    fprintf(stderr, " %02x %02x %02x   ", (s0 >> 16) & 0xff, (s0 >> 8) & 0xff,
	    s0 & 0xff);
    fprintf(stderr, "s1: LS%d LS%dR", (s1 >> 30) & 3, (s1 >> 28) & 3);
    fprintf(stderr, " %d %d %d %d\n", s1 & 0x7f, (s1 >> 7) & 0x7f,
	    (s1 >> 14) & 0x7f, (s1 >> 21) & 0x7f);
}
#endif

static void designate(charset_state *state, int container,
		      int type, int ibyte, int fbyte)
{
    unsigned long i;

    assert(container >= 0 && container <= 3);
    assert(type == S4 || type == S6 || type == M4 || type == M6);

    for (i = 0; i <= lenof(iso2022_subcharsets); i++) {
	if (iso2022_subcharsets[i].type == type &&
	    iso2022_subcharsets[i].i == ibyte &&
	    iso2022_subcharsets[i].f == fbyte) {
	    state->s1 &= ~(0x7fL << (container * 7));
	    state->s1 |= (i << (container * 7));
	    return;
	}
    }
    /*
     * If we don't find the charset, invoke the empty one, so we
     * output ERROR rather than garbage.
     */
    designate(state, container, type, 0, '~');
}

static void do_utf8(long int input_chr,
		    charset_state *state,
		    void (*emit)(void *ctx, long int output),
		    void *emitctx)
{
    charset_state ustate;

    ustate.s1 = 0;
    ustate.s0 = state->s0 & 0x03ffffffL;
    read_utf8(NULL, input_chr, &ustate, emit, emitctx);
    state->s0 = (state->s0 & ~0x03ffffffL) | (ustate.s0 & 0x03ffffffL);
}

static void docs_utf8(long int input_chr,
		      charset_state *state,
		      void (*emit)(void *ctx, long int output),
		      void *emitctx)
{
    int retstate;

    /*
     * Bits [25:0] of s0 are reserved for read_utf8().
     * Bits [27:26] are a tiny state machine to recognise ESC % @.
     */
    retstate = (state->s0 & 0x0c000000L) >> 26;
    if (retstate == 1 && input_chr == '%')
	retstate = 2;
    else if (retstate == 2 && input_chr == '@') {
	/* If we've got a partial UTF-8 sequence, complain. */
	if (state->s0 & 0x03ffffffL)
	    emit(emitctx, ERROR);
	state->s0 = 0;
	return;
    } else {
	if (retstate >= 1) do_utf8(ESC, state, emit, emitctx);
	if (retstate >= 2) do_utf8('%', state, emit, emitctx);
	retstate = 0;
	if (input_chr == ESC)
	    retstate = 1;
	else {
	    do_utf8(input_chr, state, emit, emitctx);
	}
    }
    state->s0 = (state->s0 & ~0x0c000000L) | (retstate << 26);
}

struct ctext_encoding {
    char const *name;
    charset_spec const *subcs;
};

/*
 * In theory, this list is in <http://ftp.x.org/pub/docs/registry>,
 * but XLib appears to have its own ideas, and encodes these three
 * (as of X11R6.8.2)
 */

extern charset_spec const charset_CS_ISO8859_14;
extern charset_spec const charset_CS_ISO8859_15;
extern charset_spec const charset_CS_BIG5;

static struct ctext_encoding const ctext_encodings[] = {
    { "big5-0\2", &charset_CS_BIG5 },
    { "iso8859-14\2", &charset_CS_ISO8859_14 },
    { "iso8859-15\2", &charset_CS_ISO8859_15 }
};

static void docs_ctext(long int input_chr,
		       charset_state *state,
		       void (*emit)(void *ctx, long int output),
		       void *emitctx)
{
    /*
     * s0[27:26] = first entry in ctext_encodings that matches
     * s0[25:22] = number of characters successfully matched, 0xf if all
     * s0[21:8] count the number of octets left in the segment
     * s0[7:0] are for sub-charset use
     */
    int n = (state->s0 >> 22) & 0xf, i = (state->s0 >> 26) & 3, oi = i, j;
    int length = (state->s0 >> 8) & 0x3fff;

    if (!length) {
	/* Haven't read length yet */
	if ((state->s0 & 0xff) == 0)
	    /* ... or even the first byte */
	    state->s0 |= input_chr;
	else {
	    length = (state->s0 & 0x7f) * 0x80 + (input_chr & 0x7f);
	    if (length == 0)
		state->s0 = 0;
	    else
		state->s0 = (state->s0 & 0xf0000000) | (length << 8);
	}
	return;
    }

    j = i;
    if (n == 0xe) {
	/* Skipping unknown encoding.  Look out for STX. */
	if (input_chr == 2)
	    state->s0 = (state->s0 & 0xf0000000) | (i << 26) | (0xf << 22);
    } else if (n != 0xf) {
	while (j < lenof(ctext_encodings) &&
	       !memcmp(ctext_encodings[j].name,
		       ctext_encodings[oi].name, n)) {
	    if (ctext_encodings[j].name[n] < input_chr)
		i = ++j;
	    else
		break;
	}
	if (i >= lenof(ctext_encodings) ||
	    memcmp(ctext_encodings[i].name,
		   ctext_encodings[oi].name, n) ||
	    ctext_encodings[i].name[n] != input_chr) {
	    /* Doom!  We haven't heard of this encoding */
	    i = lenof(ctext_encodings);
	    n = 0xe;
	} else {
	    /*
	     * Otherwise, we have found an additional character in our
	     * encoding name. See if we have reached the _end_ of our
	     * name.
	     */
	    n++;
	    if (!ctext_encodings[i].name[n])
		n = 0xf;
	}
	/*
	 * Failing _that_, we simply update our encoding-name-
	 * tracking state.
	 */
	assert(i < 4 && n < 16);
	state->s0 = (state->s0 & 0xf0000000) | (i << 26) | (n << 22);
    } else {
	if (i >= lenof(ctext_encodings))
	    emit(emitctx, ERROR);
	else {
	    charset_state substate;
	    charset_spec const *subcs = ctext_encodings[i].subcs;
	    substate.s1 = 0;
	    substate.s0 = state->s0 & 0xff;
	    subcs->read(subcs, input_chr, &substate, emit, emitctx);
	    state->s0 = (state->s0 & ~0xff) | (substate.s0 & 0xff);
	}
    }
    if (!--length)
	state->s0 = 0;
    else
	state->s0 = (state->s0 &~0x003fff00) | (length << 8);
}

static void read_iso2022(charset_spec const *charset, long int input_chr,
			  charset_state *state,
			  void (*emit)(void *ctx, long int output),
			  void *emitctx)
{

    /* dump_state(state); */
    /*
     * We have to make fairly efficient use of the 64 bits of state
     * available to us.  Long-term state goes in s1, and consists of
     * the identities of the character sets designated as G0/G1/G2/G3
     * and the locking-shift states for GL and GR.  Short-term state
     * goes in s0: The bottom half of s0 accumulates characters for an
     * escape sequence or a multi-byte character, while the top three
     * bits indicate what they're being accumulated for.  After DOCS,
     * the bottom 29 bits of state are available for the DOCS function
     * to use -- the UTF-8 one uses the bottom 26 for UTF-8 decoding
     * and the top two to recognised ESC % @.
     *
     * s0[31:29] = state enum
     * s0[24:0] = accumulated bytes
     * s1[31:30] = GL locking-shift state
     * s1[29:28] = GR locking-shift state
     * s1[27:21] = G3 charset
     * s1[20:14] = G2 charset
     * s1[13:7] = G1 charset
     * s1[6:0] = G0 charset
     */

#define LEFT 30
#define RIGHT 28
#define LOCKING_SHIFT(n,side) \
	(state->s1 = (state->s1 & ~(3L<<(side))) | ((n ## L)<<(side)))
#define MODE ((state->s0 & 0xe0000000L) >> 29)
#define ENTER_MODE(m) (state->s0 = (state->s0 & ~0xe0000000L) | ((m)<<29))
#define SINGLE_SHIFT(n) ENTER_MODE(SS2CHAR - 2 + (n))
#define ASSERT_IDLE do {						\
	if (state->s0 != 0) emit(emitctx, ERROR);			\
	state->s0 = 0;							\
} while (0)

    if (state->s1 == 0) {
	/*
	 * Since there's no LS0R, this means we must just have started.
	 * Set up a sane initial state (LS0, LS1R, ASCII in G0/G1/G2/G3).
	 */
	LOCKING_SHIFT(0, LEFT);
	LOCKING_SHIFT(1, RIGHT);
	designate(state, 0, S4, 0, 'B');
	designate(state, 1, S4, 0, 'B');
	designate(state, 2, S4, 0, 'B');
	designate(state, 3, S4, 0, 'B');
    }

    if (MODE == DOCSUTF8) {
	docs_utf8(input_chr, state, emit, emitctx);
	return;
    }
    if (MODE == DOCSCTEXT) {
	docs_ctext(input_chr, state, emit, emitctx);
	return;
    }

    if ((input_chr & 0x60) == 0x00) {
	/* C0 or C1 control */
	ASSERT_IDLE;
	switch (input_chr) {
	  case ESC:
	    ENTER_MODE(ESCSEQ);
	    break;
	  case LS0:
	    LOCKING_SHIFT(0, LEFT);
	    break;
	  case LS1:
	    LOCKING_SHIFT(1, LEFT);
	    break;
	  case SS2:
	    SINGLE_SHIFT(2);
	    break;
	  case SS3:
	    SINGLE_SHIFT(3);
	    break;
	  default:
	    emit(emitctx, input_chr);
	    break;
	}
    } else if ((input_chr & 0x80) || MODE < ESCSEQ) {
	int is_gl = 0;
	struct iso2022_subcharset const *subcs;
	unsigned container;
	long input_7bit;
	/*
	 * Actual data.
	 * Force idle state if we're in mid escape sequence, or in a
	 * multi-byte character with a different top bit.
	 */
	if (MODE >= ESCSEQ ||
	    ((state->s0 & 0x00ff0000L) != 0 && 
	     (((state->s0 >> 16) ^ input_chr) & 0x80)))
	    ASSERT_IDLE;
	if (MODE == SS2CHAR || MODE == SS3CHAR) /* Single-shift */
	    container = MODE - SS2CHAR + 2;
	else if (input_chr >= 0x80) /* GR */
	    container = (state->s1 >> 28) & 3;
	else { /* GL */
	    container = state->s1 >> 30;
	    is_gl = 1;
	}
	input_7bit = input_chr & ~0x80;
	subcs = &iso2022_subcharsets[(state->s1 >> (container * 7)) & 0x7f];
	if ((subcs->type == S4 || subcs->type == M4) &&
	    (input_7bit == 0x20 || input_7bit == 0x7f)) {
	    /* characters not in 94-char set */
	    if (is_gl) emit(emitctx, input_7bit);
	    else emit(emitctx, ERROR);
	} else if (subcs->type == M4 || subcs->type == M6) {
	    if ((state->s0 & 0x00ff0000L) == 0) {
		state->s0 |= input_chr << 16;
		return;
	    } else {
		emit(emitctx,
		     subcs->dbcs_fn(((state->s0 >> 16) & 0x7f) + subcs->offset,
				    input_7bit + subcs->offset));
	    }
	} else {
	    if ((state->s0 & 0x00ff0000L) != 0)
		emit(emitctx, ERROR);
	    emit(emitctx, subcs->sbcs_base ?
		 sbcs_to_unicode(subcs->sbcs_base, input_7bit + subcs->offset):
		 ERROR);
	}
	state->s0 = 0;
    } else {
	unsigned i1, i2;
	if (MODE == ESCPASS) {
	    emit(emitctx, input_chr);
	    if ((input_chr & 0xf0) != 0x20)
		ENTER_MODE(IDLE);
	    return;
	}

	/*
	 * Intermediate bytes shall be any of the 16 positions of
	 * column 02 of the code table; they are denoted by the symbol
	 * I.
	 */
	if ((input_chr & 0xf0) == 0x20) {
	    if (((state->s0 >> 16) & 0xff) == 0)
		state->s0 |= input_chr << 16;
	    else if (((state->s0 >> 8) & 0xff) == 0)
		state->s0 |= input_chr << 8;
	    else {
		/* Long escape sequence.  Switch to ESCPASS or ESCDROP. */
		i1 = (state->s0 >> 16) & 0xff;
		i2 = (state->s0 >> 8) & 0xff;
		switch (i1) {
		  case '(': case ')': case '*': case '+':
		  case '-': case '.': case '/':
		  case '$':
		    ENTER_MODE(ESCDROP);
		    break;
		  default:
		    emit(emitctx, ESC);
		    emit(emitctx, i1);
		    emit(emitctx, i2);
		    emit(emitctx, input_chr);
		    state->s0 = 0;
		    ENTER_MODE(ESCPASS);
		    break;
		}
	    }
	    return;
	}

	/*
	 * Final bytes shall be any of the 79 positions of columns 03
	 * to 07 of the code table excluding position 07/15; they are
	 * denoted by the symbol F.
	 */
	i1 = (state->s0 >> 16) & 0xff;
	i2 = (state->s0 >> 8) & 0xff;
	if (MODE == ESCDROP)
	    input_chr = 0; /* Make sure it won't match. */
	state->s0 = 0;
	switch (i1) {
	  case 0: /* No intermediate bytes */
	    switch (input_chr) {
	      case 'N': /* SS2 */
		SINGLE_SHIFT(2);
		break;
	      case 'O': /* SS3 */
		SINGLE_SHIFT(3);
		break;
	      case 'n': /* LS2 */
		LOCKING_SHIFT(2, LEFT);
		break;
	      case 'o': /* LS3 */
		LOCKING_SHIFT(3, LEFT);
		break;
	      case '|': /* LS3R */
		LOCKING_SHIFT(3, RIGHT);
		break;
	      case '}': /* LS2R */
		LOCKING_SHIFT(2, RIGHT);
		break;
	      case '~': /* LS1R */
		LOCKING_SHIFT(1, RIGHT);
		break;
	      default:
		/* Unsupported escape sequence.  Spit it back out. */
		emit(emitctx, ESC);
		emit(emitctx, input_chr);
	    }
	    break;
	  case ' ': /* ACS */
	    /*
	     * Various coding structure facilities specify that designating
	     * a code element also invokes it.  As far as I can see, invoking
	     * it now will have the same practical effect, since those
	     * facilities also ban the use of locking shifts.
	     */
	    switch (input_chr) {
	      case 'A': /* G0 element used and invoked into GL */
		LOCKING_SHIFT(0, LEFT);
		break;
	      case 'C': /* G0 in GL, G1 in GR */
	      case 'D': /* Ditto, at least for 8-bit codes */
	      case 'L': /* ISO 4873 (ECMA-43) level 1 */
	      case 'M': /* ISO 4873 (ECMA-43) level 2 */
		LOCKING_SHIFT(0, LEFT);
		LOCKING_SHIFT(1, RIGHT);
		break;
	    }
	    break;
	  case '&': /* IRR */
	    /*
	     * IRR (Identify Revised Registration) is ignored here,
	     * since any revised registration must be
	     * upward-compatible with the old one, so either we'll
	     * support the new one or we'll emit ERROR when we run
	     * into a new character.  In either case, there's nothing
	     * to be done here.
	     */
	    break;
	  case '(': /* GZD4 */  case ')': /* G1D4 */
	  case '*': /* G2D4 */  case '+': /* G3D4 */
	    designate(state, i1 - '(', S4, i2, input_chr);
	    break;
	  case '-': /* G1D6 */  case '.': /* G2D6 */  case '/': /* G3D6 */
	    designate(state, i1 - ',', S6, i2, input_chr);
	    break;
	  case '$': /* G?DM? */
	    switch (i2) {
	      case 0: /* Obsolete version of GZDM4 */
		i2 = '(';
	      case '(': /* GZDM4 */  case ')': /* G1DM4 */
	      case '*': /* G2DM4 */  case '+': /* G3DM4 */
		designate(state, i2 - '(', M4, 0, input_chr);
		break;
	      case '-': /* G1DM6 */
	      case '.': /* G2DM6 */  case '/': /* G3DM6 */
		designate(state, i2 - ',', M6, 0, input_chr);
		break;
	      default:
		emit(emitctx, ERROR);
		break;
	    }
	  case '%': /* DOCS */
	    /* XXX What's a reasonable way to handle an unrecognised DOCS? */
	    switch (i2) {
	      case 0:
		switch (input_chr) {
		  case 'G':
		    ENTER_MODE(DOCSUTF8);
		    break;
		}
		break;
	      case '/':
		switch (input_chr) {
		  case '1': case '2':
		    ENTER_MODE(DOCSCTEXT);
		    break;
		}
		break;
	    }
	    break;
	  default:
	    /* Unsupported nF escape sequence.  Re-emit it. */
	    emit(emitctx, ESC);
	    emit(emitctx, i1);
	    if (i2) emit(emitctx, i2);
	    emit(emitctx, input_chr);
	    break;
	}
    }
}

static int write_iso2022(charset_spec const *charset, long int input_chr,
			 charset_state *state,
			 void (*emit)(void *ctx, long int output),
			 void *emitctx)
{
    return FALSE;
}

const charset_spec charset_CS_ISO2022 = {
    CS_ISO2022, read_iso2022, write_iso2022, NULL
};

#ifdef TESTMODE

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

int total_errs = 0;

void iso2022_emit(void *ctx, long output)
{
    wchar_t **p = (wchar_t **)ctx;
    *(*p)++ = output;
}

void iso2022_read_test(int line, char *input, int inlen, ...)
{
    va_list ap;
    wchar_t *p, str[512];
    int i;
    charset_state state;
    unsigned long l;

    state.s0 = state.s1 = 0;
    p = str;

    for (i = 0; i < inlen; i++)
	read_iso2022(NULL, input[i] & 0xFF, &state, iso2022_emit, &p);

    va_start(ap, inlen);
    l = 0;
    for (i = 0; i < p - str; i++) {
	l = va_arg(ap, long int);
	if (l == -1) {
	    printf("%d: correct string shorter than output\n", line);
	    total_errs++;
	    break;
	}
	if (l != str[i]) {
	    printf("%d: char %d came out as %08x, should be %08lx\n",
		    line, i, str[i], l);
	    total_errs++;
	}
    }
    if (l != -1) {
	l = va_arg(ap, long int);
	if (l != -1) {
	    printf("%d: correct string longer than output\n", line);
	    total_errs++;
	}
    }
    va_end(ap);
}

/* Macro to concoct the first three parameters of iso2022_read_test. */
#define TESTSTR(x) __LINE__, x, lenof(x)

int main(void)
{
    printf("read tests beginning\n");
    /* Simple test (Emacs sample text for Japanese, in ISO-2022-JP) */
    iso2022_read_test(TESTSTR("Japanese (\x1b$BF|K\\8l\x1b(B)\t"
			      "\x1b$B$3$s$K$A$O\x1b(B, "
			      "\x1b$B%3%s%K%A%O\x1b(B\n"),
		      'J','a','p','a','n','e','s','e',' ','(',
		      0x65E5, 0x672C, 0x8A9E, ')', '\t',
		      0x3053, 0x3093, 0x306b, 0x3061, 0x306f, ',', ' ',
		      0x30b3, 0x30f3, 0x30cb, 0x30c1, 0x30cf, '\n', 0, -1);
    /* Same thing in EUC-JP (with designations, and half-width katakana) */
    iso2022_read_test(TESTSTR("\x1b$)B\x1b*I\x1b$+D"
			      "Japanese (\xc6\xfc\xcb\xdc\xb8\xec)\t"
			      "\xa4\xb3\xa4\xf3\xa4\xcb\xa4\xc1\xa4\xcf, "
			      "\x8e\xba\x8e\xdd\x8e\xc6\x8e\xc1\x8e\xca\n"),
		      'J','a','p','a','n','e','s','e',' ','(',
		      0x65E5, 0x672C, 0x8A9E, ')', '\t',
		      0x3053, 0x3093, 0x306b, 0x3061, 0x306f, ',', ' ',
		      0xff7a, 0xff9d, 0xff86, 0xff81, 0xff8a, '\n', 0, -1);
    /* Multibyte single-shift */
    iso2022_read_test(TESTSTR("\x1b$)B\x1b*I\x1b$+D\x8f\"/!"),
		      0x02D8, '!', 0, -1);
    /* Non-existent SBCS */
    iso2022_read_test(TESTSTR("\x1b(!Zfnord\n"),
		      ERROR, ERROR, ERROR, ERROR, ERROR, '\n', 0, -1);
    /* Pass-through of ordinary escape sequences, including a long one */
    iso2022_read_test(TESTSTR("\x1b""b\x1b#5\x1b#!!!5"),
		      0x1B, 'b', 0x1B, '#', '5',
		      0x1B, '#', '!', '!', '!', '5', 0, -1);
    /* Non-existent DBCS (also 5-byte escape sequence) */
    iso2022_read_test(TESTSTR("\x1b$(!Bfnord!"),
		      ERROR, ERROR, ERROR, 0, -1);
    /* Incomplete DB characters */
    iso2022_read_test(TESTSTR("\x1b$B(,(\x1b(BHi\x1b$B(,(\n"),
		      0x2501, ERROR, 'H', 'i', 0x2501, ERROR, '\n', 0, -1);
    iso2022_read_test(TESTSTR("\x1b$)B\x1b*I\x1b$+D\xa4""B"),
		      ERROR, 'B', 0, -1);
    iso2022_read_test(TESTSTR("\x1b$)B\x1b*I\x1b$+D\x0e\x1b|$\xa2\xaf"),
		      ERROR, 0x02D8, 0, -1);
    /* Incomplete escape sequence */
    iso2022_read_test(TESTSTR("\x1b\n"), ERROR, '\n', 0, -1);
    iso2022_read_test(TESTSTR("\x1b-A\x1b~\x1b\xa1"), ERROR, 0xa1, 0, -1);
    /* Incomplete single-shift */
    iso2022_read_test(TESTSTR("\x8e\n"), ERROR, '\n', 0, -1);
    iso2022_read_test(TESTSTR("\x1b$*B\x8e(\n"), ERROR, '\n', 0, -1);
    /* Corner cases (02/00 and 07/15) */
    iso2022_read_test(TESTSTR("\x1b(B\x20\x7f"), 0x20, 0x7f, 0, -1);
    iso2022_read_test(TESTSTR("\x1b(I\x20\x7f"), 0x20, 0x7f, 0, -1);
    iso2022_read_test(TESTSTR("\x1b$B\x20\x7f"), 0x20, 0x7f, 0, -1);
    iso2022_read_test(TESTSTR("\x1b-A\x0e\x20\x7f"), 0xa0, 0xff, 0, -1);
    iso2022_read_test(TESTSTR("\x1b$-~\x0e\x20\x7f"), ERROR, 0, -1);
    iso2022_read_test(TESTSTR("\x1b)B\xa0\xff"), ERROR, ERROR, 0, -1);
    iso2022_read_test(TESTSTR("\x1b)I\xa0\xff"), ERROR, ERROR, 0, -1);
    iso2022_read_test(TESTSTR("\x1b$)B\xa0\xff"), ERROR, ERROR, 0, -1);
    iso2022_read_test(TESTSTR("\x1b-A\x1b~\xa0\xff"), 0xa0, 0xff, 0, -1);
    iso2022_read_test(TESTSTR("\x1b$-~\x1b~\xa0\xff"), ERROR, 0, -1);
    /* Designate control sets */
    iso2022_read_test(TESTSTR("\x1b!@"), 0x1b, '!', '@', 0, -1);
    /* Designate other coding system (UTF-8) */
    iso2022_read_test(TESTSTR("\x1b%G"
			      "\xCE\xBA\xE1\xBD\xB9\xCF\x83\xCE\xBC\xCE\xB5"),
		      0x03BA, 0x1F79, 0x03C3, 0x03BC, 0x03B5, 0, -1);
    iso2022_read_test(TESTSTR("\x1b-A\x1b%G\xCE\xBA\x1b%@\xa0"),
		      0x03BA, 0xA0, 0, -1);
    iso2022_read_test(TESTSTR("\x1b%G\xCE\x1b%@"), ERROR, 0, -1);
    iso2022_read_test(TESTSTR("\x1b%G\xCE\xBA\x1b%\x1b%@"),
		      0x03BA, 0x1B, '%', 0, -1);
    /* DOCS (COMPOUND_TEXT extended segment) */
    iso2022_read_test(TESTSTR("\x1b%/1\x80\x80"), 0, -1);
    iso2022_read_test(TESTSTR("\x1b%/1\x80\x8fiso-8859-15\2xyz\x1b(B"),
		      ERROR, ERROR, ERROR, 0, -1);
    iso2022_read_test(TESTSTR("\x1b%/1\x80\x8eiso8859-15\2xyz\x1b(B"),
		      'x', 'y', 'z', 0, -1);
    iso2022_read_test(TESTSTR("\x1b-A\x1b%/2\x80\x89"
			      "big5-0\2\xa1\x40\xa1\x40"),
		      0x3000, 0xa1, 0x40, 0, -1);
    /* Emacs Big5-in-ISO-2022 mapping */
    iso2022_read_test(TESTSTR("\x1b$(0&x86\x1b(B  \x1b$(0DeBv"),
		      0x5143, 0x6c23, ' ', ' ', 0x958b, 0x767c, 0, -1);
    /* Test from RFC 1922 (ISO-2022-CN) */
    iso2022_read_test(TESTSTR("\x1b$)A\x0e=;;;\x1b$)GG(_P\x0f"),
		      0x4EA4, 0x6362, 0x4EA4, 0x63db, 0, -1);
    
    printf("read tests completed\n");
    printf("total: %d errors\n", total_errs);
    return (total_errs != 0);
}

#endif /* TESTMODE */

#else /* ENUM_CHARSETS */

ENUM_CHARSET(CS_ISO2022)

#endif
