/*
 * big5enc.c - multibyte encoding of Big5
 */

#ifndef ENUM_CHARSETS

#include "charset.h"
#include "internal.h"

/*
 * Big5 has no associated data, so `charset' may be ignored.
 */

static void read_big5(charset_spec const *charset, long int input_chr,
		      charset_state *state,
		      void (*emit)(void *ctx, long int output), void *emitctx)
{
    UNUSEDARG(charset);

    /*
     * For reading Big5, state->s0 simply contains the single
     * stored lead byte when we are half way through a double-byte
     * character, or 0 if we aren't.
     */

    if (state->s0 == 0) {
	if (input_chr >= 0xA1 && input_chr <= 0xFE) {
	    /*
	     * Lead byte. Just store it.
	     */
	    state->s0 = input_chr;
	} else {
	    /*
	     * Anything else we pass straight through unchanged.
	     */
	    emit(emitctx, input_chr);
	}
    } else {
	/*
	 * We have a stored lead byte. We expect a valid followup
	 * byte.
	 */
	if ((input_chr >= 0x40 && input_chr <= 0x7E) ||
	    (input_chr >= 0xA1 && input_chr <= 0xFE)) {
	    emit(emitctx, big5_to_unicode(state->s0 - 0xA1, input_chr - 0x40));
	} else {
	    emit(emitctx, ERROR);
	}
	state->s0 = 0;
    }
}

/*
 * Big5 is a stateless multi-byte encoding (in the sense that just
 * after any character has been completed, the state is always the
 * same); hence when writing it, there is no need to use the
 * charset_state.
 */

static int write_big5(charset_spec const *charset, long int input_chr,
		      charset_state *state,
		      void (*emit)(void *ctx, long int output), void *emitctx)
{
    UNUSEDARG(charset);
    UNUSEDARG(state);

    if (input_chr == -1)
	return TRUE;		       /* stateless; no cleanup required */

    if (input_chr < 0x80) {
	emit(emitctx, input_chr);
	return TRUE;
    } else {
	int r, c;
	if (unicode_to_big5(input_chr, &r, &c)) {
	    emit(emitctx, r + 0xA1);
	    emit(emitctx, c + 0x40);
	    return TRUE;
	} else {
	    return FALSE;
	}
    }
}

const charset_spec charset_CS_BIG5 = {
    CS_BIG5, read_big5, write_big5, NULL
};

#else /* ENUM_CHARSETS */

ENUM_CHARSET(CS_BIG5)

#endif /* ENUM_CHARSETS */
