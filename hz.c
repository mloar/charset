/*
 * hz.c - HZ textual encoding of ASCII and GB2312, as defined in RFC 1843.
 */

#ifndef ENUM_CHARSETS

#include <assert.h>

#include "charset.h"
#include "internal.h"

static void read_hz(charset_spec const *charset, long int input_chr,
		    charset_state *state,
		    void (*emit)(void *ctx, long int output), void *emitctx)
{
    /*
     * When reading, our state variables are:
     * 
     *  - s0 is 0 in ASCII mode, 1 in GB2312 mode.
     * 
     * 	- s1 stores a character we have just seen but not fully
     * 	  processed. So in ASCII mode, this can only ever be zero
     * 	  (no character) or 0x7E (~); in GB2312 mode it can be
     * 	  anything from 0x21-0x7E.
     */

    UNUSEDARG(charset);

    if (state->s0 == 0) {
	/*
	 * ASCII mode.
	 */

	if (state->s1) {
	    assert(state->s1 == '~');
	    state->s1 = 0;
	    /* Process the character after a tilde. */
	    switch (input_chr) {
	      case '~':
		emit(emitctx, input_chr);
		return;
	      case '\n':
		return;		       /* ~\n is ignored */
	      case '{':
		state->s0 = 1;	       /* switch to GB2312 mode */
		return;
	    }
	} else if (input_chr == '~') {
	    state->s1 = '~';
	    return;
	} else {
	    /* In ASCII mode, any non-tildes go straight  */
	    emit(emitctx, input_chr);
	    return;
	}
    } else {
	/*
	 * GB2312 mode. As I understand it, we expect never to see
	 * anything in this mode that isn't 0x21-0x7E. So if we do,
	 * we'll simply throw an error and return to ASCII mode.
	 */
	if (input_chr < 0x21 || input_chr > 0x7E) {
	    emit(emitctx, ERROR);
	    state->s0 = state->s1 = 0;
	    return;
	}

	/*
	 * So if we don't have a character stored already, store
	 * this one...
	 */
	if (!state->s1) {
	    state->s1 = input_chr;
	    return;
	}

	/*
	 * ... otherwise, combine the stored char with this one.
	 * This will give either `~}', the escape sequence to
	 * return to ASCII mode, or something which we translate
	 * through GB2312.
	 */
	if (state->s1 == '~' && input_chr == '}') {
	    state->s1 = state->s0 = 0;
	    return;
	}

	emit(emitctx, gb2312_to_unicode(state->s1 - 0x21, input_chr - 0x21));
	state->s1 = 0;
    }
}

static int write_hz(charset_spec const *charset, long int input_chr,
		    charset_state *state,
		    void (*emit)(void *ctx, long int output), void *emitctx)
{
    int desired_state, r, c;

    UNUSEDARG(charset);

    /*
     * Analyse the input char.
     */
    if (input_chr < 0x80) {
	desired_state = 0;
	c = input_chr;
    } else if (unicode_to_gb2312(input_chr, &r, &c)) {
	desired_state = 1;
    } else {
	return FALSE;
    }

    if (state->s0 != (unsigned)desired_state) {
	emit(emitctx, '~');
	emit(emitctx, desired_state ? '{' : '}');
	state->s0 = desired_state;
    }

    if (input_chr < 0)
	return TRUE;		       /* special case: just reset state */

    if (state->s0) {
	/*
	 * GB mode.
	 */
	emit(emitctx, 0x21 + r);
	emit(emitctx, 0x21 + c);
    } else {
	emit(emitctx, c);
    }
    return TRUE;
}

const charset_spec charset_CS_HZ = {
    CS_HZ, read_hz, write_hz, NULL
};

#else /* ENUM_CHARSETS */

ENUM_CHARSET(CS_HZ)

#endif /* ENUM_CHARSETS */
