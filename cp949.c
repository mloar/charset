/*
 * cp949.c - CP949 / KS_C_5601_1987 multibyte encoding
 */

#ifndef ENUM_CHARSETS

#include "charset.h"
#include "internal.h"

/*
 * CP949 has no associated data, so `charset' may be ignored.
 */

static void read_cp949(charset_spec const *charset, long int input_chr,
		       charset_state *state,
		       void (*emit)(void *ctx, long int output), void *emitctx)
{
    UNUSEDARG(charset);

    /*
     * For reading CP949, state->s0 simply contains the single
     * stored lead byte when we are half way through a double-byte
     * character, or 0 if we aren't.
     */

    if (state->s0 == 0) {
	if (input_chr >= 0x81 && input_chr <= 0xFE) {
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
	if ((input_chr >= 0x40 && input_chr <= 0xFF)) {
	    emit(emitctx, cp949_to_unicode(state->s0 - 0x80,
					   input_chr - 0x40));
	} else {
	    emit(emitctx, ERROR);
	}
	state->s0 = 0;
    }
}

/*
 * CP949 is a stateless multi-byte encoding (in the sense that just
 * after any character has been completed, the state is always the
 * same); hence when writing it, there is no need to use the
 * charset_state.
 */

static int write_cp949(charset_spec const *charset, long int input_chr,
		       charset_state *state,
		       void (*emit)(void *ctx, long int output),
		       void *emitctx)
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
	if (unicode_to_cp949(input_chr, &r, &c)) {
	    emit(emitctx, r + 0x80);
	    emit(emitctx, c + 0x40);
	    return TRUE;
	} else {
	    return FALSE;
	}
    }
}

const charset_spec charset_CS_CP949 = {
    CS_CP949, read_cp949, write_cp949, NULL
};

#else /* ENUM_CHARSETS */

ENUM_CHARSET(CS_CP949)

#endif /* ENUM_CHARSETS */
