/*
 * utf16.c - routines to handle UTF-16 (RFC 2781).
 */

#ifndef ENUM_CHARSETS

#include "charset.h"
#include "internal.h"

struct utf16 {
    int s0;			       /* initial value of state->s0 */
};

static void read_utf16(charset_spec const *charset, long int input_chr,
		       charset_state *state,
		       void (*emit)(void *ctx, long int output),
		       void *emitctx)
{
    struct utf16 const *utf = (struct utf16 *)charset->data;
    long int hw;

    /*
     * State variable s1 handles the combining of bytes into
     * transport-endianness halfwords. It contains:
     * 
     *  - 0 if we're between halfwords
     *  - 0x100 plus the first byte if we're in mid-halfword
     * 
     * State variable s0 handles everything from there upwards. It
     * contains:
     * 
     * 	- Bottom 16 bits are set to a surrogate value if we've just
     * 	  seen one.
     * 	- Next two bits (17:16) indicate possible endiannesses. Bit
     * 	  17 is set if we might be BE; bit 16 if we might be LE. If
     * 	  they're both zero, it has to be because this is right at
     * 	  the start, so the first thing we do is set them to the
     * 	  correct initial state.
     * 	- The bit after that (18) is 1 iff we have already seen at
     * 	  least one halfword (meaning we should pass any further
     * 	  BOMs straight through).
     */

    /* Set up s0 if this is the start. */
    if (state->s0 == 0)
	state->s0 = utf->s0;

    /* Accumulate a transport-endianness halfword. */
    if (state->s1 == 0) {
	state->s1 = 0x100 | input_chr;
	return;
    }
    hw = ((state->s1 & 0xFF) << 8) + input_chr;
    state->s1 = 0;

    /* Process BOM and determine byte order. */
    if (!(state->s0 & 0x40000)) {
	state->s0 |= 0x40000;
	if (hw == 0xFEFF && (state->s0 & 0x20000)) {
	    /*
	     * Text starts with a big-endian BOM, and big-
	     * endianness is a possibility. So clear the
	     * little-endian bit (the BOM confirms our endianness),
	     * and return without emitting the BOM in Unicode.
	     */
	    state->s0 &= ~0x10000;
	    return;
	} else if (hw == 0xFFFE && (state->s0 & 0x10000)) {
	    /*
	     * Text starts with a little-endian BOM, and little-
	     * endianness is a possibility. So clear the big-endian
	     * bit (the BOM confirms our endianness), and return
	     * without emitting the BOM in Unicode.
	     */
	    state->s0 &= ~0x20000;
	    return;
	} else {
	    /*
	     * Text does not begin with a BOM. RFC 2781 states that
	     * in this case we must assume big-endianness if we
	     * haven't been told otherwise by the content type.
	     */
	    if ((state->s0 & 0x30000) == 0x30000)
		state->s0 &= ~0x10000; /* clear LE bit */
	}
    }

    /*
     * Byte-swap transport-endianness halfword if necessary. We may
     * now test individual endianness bits, since we can be sure
     * exactly one is set.
     */
    if (state->s0 & 0x10000)
	hw = ((hw >> 8) | (hw << 8)) & 0xFFFF;

    /*
     * Now that the endianness issue has been dealt with, what
     * reaches this point should be a stream of halfwords in
     * sensible numeric form. So now we process surrogates.
     */
    if (state->s0 & 0xFFFF) {
	/*
	 * We have already seen a high surrogate, so we expect a
	 * low surrogate. Whinge if we didn't get it.
	 */
	if (hw < 0xDC00 || hw >= 0xE000) {
	    emit(emitctx, ERROR);
	} else {
	    hw &= 0x3FF;
	    hw |= (state->s0 & 0x3FF) << 10;
	    emit(emitctx, hw + 0x10000);
	}
	state->s0 &= 0xFFFF0000;
    } else {
	/*
	 * Any low surrogate is an error.
	 */
	if (hw >= 0xDC00 && hw < 0xE000) {
	    emit(emitctx, ERROR);
	    return;
	}

	/*
	 * Any high surrogate is simply stored until we see the
	 * next halfword.
	 */
	if (hw >= 0xD800 && hw < 0xDC00) {
	    state->s0 |= hw;
	    return;
	}

	/*
	 * Anything else we simply output.
	 */
	emit(emitctx, hw);
    }
}

/*
 * Repeated code in write_utf16 abstracted out for sanity.
 */
static void emithl(void (*emit)(void *ctx, long int output), void *emitctx,
		   unsigned long s0, long int hw)
{
    int h = (hw >> 8) & 0xFF, l = hw & 0xFF;

    if (s0 & 0x20000) {
	/* Big-endian takes priority over little, if both are allowed. */
	emit(emitctx, h);
	emit(emitctx, l);
    } else {
	emit(emitctx, l);
	emit(emitctx, h);
    }
}

static int write_utf16(charset_spec const *charset, long int input_chr,
		       charset_state *state,
		       void (*emit)(void *ctx, long int output),
		       void *emitctx)
{
    struct utf16 const *utf = (struct utf16 *)charset->data;

    /*
     * state->s0 == 0 means we have not output anything yet (and so
     * must output a BOM before we do anything else). state->s0 ==
     * 1 means we are off and running.
     */

    if (input_chr < 0)
	return TRUE;		       /* no cleanup required */

    if ((input_chr >= 0xD800 && input_chr < 0xE000) ||
	input_chr >= 0x110000) {
	/*
	 * We can't output surrogates, or anything above 0x10FFFF.
	 */
	return FALSE;
    }

    if (!state->s0) {
	state->s0 = 1;
	emithl(emit, emitctx, utf->s0, 0xFEFF);
    }

    if (input_chr < 0x10000) {
	emithl(emit, emitctx, utf->s0, input_chr);
    } else {
	input_chr -= 0x10000;
	/* now input_chr is between 0 and 0xFFFFF inclusive */
	emithl(emit, emitctx, utf->s0, 0xD800 | ((input_chr >> 10) & 0x3FF));
	emithl(emit, emitctx, utf->s0, 0xDC00 | (input_chr & 0x3FF));
    }
    return TRUE;
}

static const struct utf16 utf16_bigendian = { 0x20000 };
static const struct utf16 utf16_littleendian = { 0x10000 };
static const struct utf16 utf16_variable_endianness = { 0x30000 };

const charset_spec charset_CS_UTF16BE = {
    CS_UTF16BE, read_utf16, write_utf16, &utf16_bigendian
};
const charset_spec charset_CS_UTF16LE = {
    CS_UTF16LE, read_utf16, write_utf16, &utf16_littleendian
};
const charset_spec charset_CS_UTF16 = {
    CS_UTF16, read_utf16, write_utf16, &utf16_variable_endianness
};

#else /* ENUM_CHARSETS */

ENUM_CHARSET(CS_UTF16)
ENUM_CHARSET(CS_UTF16BE)
ENUM_CHARSET(CS_UTF16LE)

#endif /* ENUM_CHARSETS */
