/*
 * utf7.c - routines to handle UTF-7.
 */

#ifndef ENUM_CHARSETS

#include "charset.h"
#include "internal.h"

/*
 * This array is generated by a piece of Perl:

perl -e 'for $i (0..32) { $a[$i] |= 2; } $a[32] |= 1;' \
     -e 'for $i ("a".."z","A".."Z","0".."9","'\''","(",' \
     -e '        ")",",","-",".","/",":","?") { $a[ord $i] |= 1; }' \
     -e 'for $i ("!","\"","#","\$","%","&","*",";","<","=",">","\@",' \
     -e '        "[","]","^","_","`","{","|","}") { $a[ord $i] |= 2; }' \
     -e 'for $i ("a".."z","A".."Z","0".."9","+","/") { $a[ord $i] |= 4; }' \
     -e 'for $i (0..127) { printf "%s%d,%s", $i%32?"":"    ", $a[$i],' \
     -e '                  ($i+1)%32?"":"\n"; }'

 */
static const unsigned char utf7_ascii_properties[128] = {
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,2,2,2,2,2,2,1,1,1,2,4,1,1,1,5,5,5,5,5,5,5,5,5,5,5,1,2,2,2,2,1,
    2,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,2,0,2,2,2,
    2,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,2,2,2,0,0,
};
#define SET_D(c) ((c) >= 0 && (c) < 0x80 && (utf7_ascii_properties[(c)] & 1))
#define SET_O(c) ((c) >= 0 && (c) < 0x80 && (utf7_ascii_properties[(c)] & 2))
#define SET_B(c) ((c) >= 0 && (c) < 0x80 && (utf7_ascii_properties[(c)] & 4))

#define base64_value(c) ( (c) >= 'A' && (c) <= 'Z' ? (c) - 'A' : \
			  (c) >= 'a' && (c) <= 'z' ? (c) - 'a' + 26 : \
			  (c) >= '0' && (c) <= '9' ? (c) - '0' + 52 : \
			  (c) == '+' ? 62 : 63 )

static const char *base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void read_utf7(charset_spec const *charset, long int input_chr,
		      charset_state *state,
		      void (*emit)(void *ctx, long int output), void *emitctx)
{
    long int hw;

    UNUSEDARG(charset);

    /*
     * state->s0 is used to handle the conversion of the UTF-7
     * transport format into a stream of halfwords. Its layout is:
     * 
     *  - In normal ASCII mode, it is zero.
     * 
     * 	- Otherwise, it holds a leading 1 followed by all the bits
     * 	  so far accumulated in base64 digits.
     * 
     * 	- Special case: when we have only just seen the initial `+'
     * 	  which enters base64 mode, it is set to 2 rather than 1
     * 	  (this is an otherwise unused value since base64 always
     * 	  accumulates an even number of bits at a time), so that
     * 	  the special sequence `+-' can be made to encode `+'
     * 	  easily.
     * 
     * state->s1 is used to handle the conversion of those
     * halfwords into Unicode values. It contains a high surrogate
     * value if we've just seen one, and 0 otherwise.
     */

    if (!state->s0) {
	if (input_chr == '+')
	    state->s0 = 2;
	else
	    emit(emitctx, input_chr);
	return;
    } else {
	if (!SET_B(input_chr)) {
	    /*
	     * base64 mode ends here. Emit the character we have,
	     * unless it's a minus in which case we should swallow
	     * it.
	     */
	    if (input_chr != '-')
		emit(emitctx, input_chr);
	    else if (state->s0 == 2)
		emit(emitctx, '+');    /* special case */
	    state->s0 = 0;
	    return;
	}

	/*
	 * Now we have a base64 character, so add it to our state,
	 * first correcting the special case value of s0.
	 */
	if (state->s0 == 2)
	    state->s0 = 1;
	state->s0 = (state->s0 << 6) | base64_value(input_chr);
    }

    /*
     * If we don't have a whole halfword at this point, bale out.
     */
    if (!(state->s0 & 0xFFFF0000))
	return;

    /*
     * Otherwise, extract the halfword. There are three
     * possibilities for where the top set bit might be.
     */
    if (state->s0 & 0x00100000) {
	hw = (state->s0 >> 4) & 0xFFFF;
	state->s0 = (state->s0 & 0xF) | 0x10;
    } else if (state->s0 & 0x00040000) {
	hw = (state->s0 >> 2) & 0xFFFF;
	state->s0 = (state->s0 & 3) | 4;
    } else {
	hw = state->s0 & 0xFFFF;
	state->s0 = 1;
    }

    /*
     * Now what reaches this point should be a stream of halfwords
     * in sensible numeric form. So now we process surrogates.
     */
    if (state->s1) {
	/*
	 * We have already seen a high surrogate, so we expect a
	 * low surrogate. Whinge if we didn't get it.
	 */
	if (hw < 0xDC00 || hw >= 0xE000) {
	    emit(emitctx, ERROR);
	} else {
	    hw &= 0x3FF;
	    hw |= (state->s1 & 0x3FF) << 10;
	    emit(emitctx, hw + 0x10000);
	}
	state->s1 = 0;
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
	    state->s1 = hw;
	    return;
	}

	/*
	 * Anything else we simply output.
	 */
	emit(emitctx, hw);
    }
}

/*
 * For writing UTF-7, we supply two charset definitions, one of
 * which will directly encode Set O characters and the other of
 * which will cautiously base64 them.
 */
static void write_utf7(charset_spec const *charset, long int input_chr,
		       charset_state *state,
		       void (*emit)(void *ctx, long int output),
		       void *emitctx)
{
    unsigned long hws[2];
    int nhws;
    int i;

    /*
     * For writing: state->s0 contains accumulated base64 data with
     * a 1 in front, and state->s1 indicates how many bits of it we
     * have.
     */

    if ((input_chr >= 0xD800 && input_chr < 0xE000) ||
	input_chr >= 0x110000) {
	/*
	 * We can't output surrogates, or anything above 0x10FFFF.
	 */
	emit(emitctx, ERROR);
	return;
    }

    if (input_chr == -1 || SET_D(input_chr) ||
	(charset->charset == CS_UTF7 && SET_O(input_chr))) {
	if (state->s0) {
	    /*
	     * These characters are output in ASCII mode, so flush any
	     * lingering base64 data.
	     */
	    state->s0 <<= 6 - state->s1;
	    emit(emitctx, base64_chars[state->s0 & 0x3F]);
	    /*
	     * I'm going to arbitrarily decide to always use the
	     * terminating minus sign. It's easier than figuring out
	     * whether to do so or not, and looks prettier besides.
	     */
	    emit(emitctx, '-');
	    state->s0 = state->s1 = 0;
	}

	/*
	 * Now output the character.
	 */
	if (input_chr != -1)	       /* special case: just reset state */
	    emit(emitctx, input_chr);
	return;
    }

    /*
     * Now we know we have a character that needs to be output as
     * either one base64-encoded halfword or two. So first figure
     * out how many...
     */
    if (input_chr < 0x10000) {
	nhws = 1;
	hws[0] = input_chr;
    } else {
	input_chr -= 0x10000;
	if (input_chr >= 0x100000) {
	    /* Anything above 0x10FFFF is outside UTF-7 range. */
	    emit(emitctx, ERROR);
	    return;
	}

	nhws = 2;
	hws[0] = 0xD800 | ((input_chr >> 10) & 0x3FF);
	hws[1] = 0xDC00 | (input_chr & 0x3FF);
    }

    /*
     * ... switch into base64 mode if required ...
     */
    if (!state->s0) {
	emit(emitctx, '+');
	state->s0 = 1;
	state->s1 = 0;
    }

    /*
     * ... and do the base64 output.
     */
    for (i = 0; i < nhws; i++) {
	state->s0 = (state->s0 << 16) | hws[i];
	state->s1 += 16;

	while (state->s1 >= 6) {
	    /*
	     * The top set bit must be in position 16, 18 or 20.
	     */
	    unsigned long out, topbit;
	    
	    out = (state->s0 >> (state->s1 - 6)) & 0x3F;
	    state->s1 -= 6;
	    topbit = 1 << state->s1;
	    state->s0 = (state->s0 & (topbit-1)) | topbit;

	    emit(emitctx, base64_chars[out]);
	}
    }
}

const charset_spec charset_CS_UTF7 = {
    CS_UTF7, read_utf7, write_utf7, NULL
};

const charset_spec charset_CS_UTF7_CONSERVATIVE = {
    CS_UTF7_CONSERVATIVE, read_utf7, write_utf7, NULL
};

#else /* ENUM_CHARSETS */

ENUM_CHARSET(CS_UTF7)
ENUM_CHARSET(CS_UTF7_CONSERVATIVE)

#endif /* ENUM_CHARSETS */
