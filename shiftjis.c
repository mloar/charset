/*
 * shiftjis.c - multibyte encoding of Shift-JIS
 */

#ifndef ENUM_CHARSETS

#include "charset.h"
#include "internal.h"

/*
 * Shift-JIS has no associated data, so `charset' may be ignored.
 */

static void read_sjis(charset_spec const *charset, long int input_chr,
		      charset_state *state,
		      void (*emit)(void *ctx, long int output), void *emitctx)
{
    UNUSEDARG(charset);

    /*
     * For reading Shift-JIS, state->s0 simply contains the single
     * stored lead byte when we are half way through a double-byte
     * character, or 0 if we aren't.
     */

    if (state->s0 == 0) {
	if ((input_chr >= 0x81 && input_chr <= 0x9F) ||
	    (input_chr >= 0xE0 && input_chr <= 0xEF)) {
	    /*
	     * Lead byte. Just store it.
	     */
	    state->s0 = input_chr;
	} else {
	    /*
	     * Anything else we translate through JIS X 0201.
	     */
	    if (input_chr == 0x5C)
		input_chr = 0xA5;
	    else if (input_chr == 0x7E)
		input_chr = 0x203E;
	    else if (input_chr >= 0xA1 && input_chr <= 0xDF)
		input_chr += 0xFF61 - 0xA1;
	    else if (input_chr < 0x80)
		/* do nothing */;
	    else
		input_chr = ERROR;
	    emit(emitctx, input_chr);
	}
    } else {
	/*
	 * We have a stored lead byte. We expect a valid followup
	 * byte.
	 */
	if (input_chr >= 0x40 && input_chr <= 0xFC && input_chr != 0x7F) {
	    int r, c;
	    r = state->s0;
	    if (r >= 0xE0) r -= (0xE0 - 0xA0);
	    r -= 0x81;
	    c = input_chr;
	    if (c > 0x7F) c--;
	    c -= 0x40;
	    r *= 2;
	    if (c >= 94)
		r++, c -= 94;
	    emit(emitctx, jisx0208_to_unicode(r, c));
	} else {
	    emit(emitctx, ERROR);
	}
	state->s0 = 0;
    }
}

/*
 * Shift-JIS is a stateless multi-byte encoding (in the sense that
 * just after any character has been completed, the state is always
 * the same); hence when writing it, there is no need to use the
 * charset_state.
 */

static int write_sjis(charset_spec const *charset, long int input_chr,
		      charset_state *state,
		      void (*emit)(void *ctx, long int output), void *emitctx)
{
    UNUSEDARG(charset);
    UNUSEDARG(state);

    if (input_chr == -1)
	return TRUE;		       /* stateless; no cleanup required */

    if (input_chr < 0x80 && input_chr != 0x5C && input_chr != 0x7E) {
	emit(emitctx, input_chr);
	return TRUE;
    } else if (input_chr == 0xA5) {
	emit(emitctx, 0x5C);
	return TRUE;
    } else if (input_chr == 0x203E) {
	emit(emitctx, 0x7E);
	return TRUE;
    } else if (input_chr >= 0xFF61 && input_chr <= 0xFF9F) {
	emit(emitctx, input_chr - (0xFF61 - 0xA1));
	return TRUE;
    } else {
	int r, c;
	if (unicode_to_jisx0208(input_chr, &r, &c)) {
	    c += 94 * (r % 2);
	    r /= 2;
	    r += 0x81;
	    if (r >= 0xA0) r += 0xE0 - 0xA0;
	    c += 0x40;
	    if (c >= 0x7F) c++;
	    emit(emitctx, r);
	    emit(emitctx, c);
	    return TRUE;
	} else {
	    return FALSE;
	}
    }
}

const charset_spec charset_CS_SHIFT_JIS = {
    CS_SHIFT_JIS, read_sjis, write_sjis, NULL
};

#else /* ENUM_CHARSETS */

ENUM_CHARSET(CS_SHIFT_JIS)

#endif /* ENUM_CHARSETS */
