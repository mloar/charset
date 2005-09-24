/*
 * euc.c - routines to handle the various EUC multibyte encodings.
 */

#ifndef ENUM_CHARSETS

#include "charset.h"
#include "internal.h"

struct euc {
    int nchars[3];		       /* GR, SS2+GR, SS3+GR */
    long int (*to_ucs)(unsigned long state);
    unsigned long (*from_ucs)(long int ucs);
};

static void read_euc(charset_spec const *charset, long int input_chr,
		     charset_state *state,
		     void (*emit)(void *ctx, long int output), void *emitctx)
{
    struct euc const *euc = (struct euc *)charset->data;

    /*
     * For EUC input, our state variable divides into three parts:
     * 
     * 	- Topmost nibble (bits 31:28) is nonzero if we're
     * 	  accumulating a multibyte character, and it indicates
     * 	  which section we're in: 1 for GR chars, 2 for things
     * 	  beginning with SS2, 3 for things beginning with SS3.
     * 
     * 	- Next nibble (bits 27:24) indicates how many bytes of the
     * 	  character we've accumulated so far.
     * 
     * 	- The rest (bits 23:0) are those bytes in full, accumulated
     * 	  as a large integer (so that seeing A1 A2 A3, in a
     * 	  hypothetical EUC whose GR encoding is three-byte, runs
     * 	  our state variable from 0 -> 0x110000A1 -> 0x1200A1A2 ->
     * 	  0x13A1A2A3, at which point it gets translated and output
     * 	  and resets to zero).
     */

    if (state->s0 != 0) {

	/*
	 * At this point, no matter whether we had an SS2 or SS3
	 * introducer or not, we _always_ expect a GR character.
	 * Anything else causes us to emit ERROR for an incomplete
	 * character, and then reset to state 0 to process the
	 * character in its own way.
	 */
	if (input_chr < 0xA1 || input_chr == 0xFF) {
	    emit(emitctx, ERROR);
	    state->s0 = 0;
	} else
	    state->s0 = (((state->s0 & 0xFF000000) + 0x01000000) |
			 ((state->s0 & 0x0000FFFF) << 8) | input_chr);

    }

    if (state->s0 == 0) {
	/*
	 * The input character determines which of the four
	 * possible charsets we're going to be in.
	 */
	if (input_chr < 0x80) {	       /* this is always ASCII */
	    emit(emitctx, input_chr);
	} else if (input_chr == 0x8E) {/* SS2 means charset 2 */
	    state->s0 = 0x20000000;
	} else if (input_chr == 0x8F) {/* SS3 means charset 3 */
	    state->s0 = 0x30000000;
	} else if (input_chr < 0xA1 || input_chr == 0xFF) {   /* errors */
	    emit(emitctx, ERROR);
	} else {		       /* A1-FE means charset 1 */
	    state->s0 = 0x11000000 | input_chr;
	}
    }

    /*
     * Finally, if we have accumulated a complete character, output
     * it.
     */
    if (state->s0 != 0 &&
	((state->s0 & 0x0F000000) >> 24) >=
	(unsigned)euc->nchars[(state->s0 >> 28)-1]) {
	emit(emitctx, euc->to_ucs(state->s0));
	state->s0 = 0;
    }
}

/*
 * All EUCs are stateless multi-byte encodings (in the sense that
 * just after any character has been completed, the state is always
 * the same); hence when writing them, there is no need to use the
 * charset_state.
 */

static int write_euc(charset_spec const *charset, long int input_chr,
		     charset_state *state,
		     void (*emit)(void *ctx, long int output), void *emitctx)
{
    struct euc const *euc = (struct euc *)charset->data;
    unsigned long c;
    int cset, len;

    UNUSEDARG(state);

    if (input_chr == -1)
	return TRUE;		       /* stateless; no cleanup required */

    /* ASCII is the easy bit, and is always the same. */
    if (input_chr < 0x80) {
	emit(emitctx, input_chr);
	return TRUE;
    }

    c = euc->from_ucs(input_chr);
    if (!c) {
	return FALSE;
    }

    cset = c >> 28;
    len = euc->nchars[cset-1];
    c &= 0xFFFFFF;

    if (cset > 1)
	emit(emitctx, 0x8C + cset);    /* SS2/SS3 */

    while (len--)
	emit(emitctx, (c >> (8*len)) & 0xFF);
    return TRUE;
}

/*
 * EUC-CN encodes GB2312 only.
 */
static long int euc_cn_to_ucs(unsigned long state)
{
    switch (state >> 28) {
      case 1: return gb2312_to_unicode(((state >> 8) & 0xFF) - 0xA1,
				       ((state     ) & 0xFF) - 0xA1);
      default: return ERROR;
    }
}
static unsigned long euc_cn_from_ucs(long int ucs)
{
    int r, c;
    if (unicode_to_gb2312(ucs, &r, &c))
	return 0x10000000 | ((r+0xA1) << 8) | (c+0xA1);
    else
	return 0;
}
static const struct euc euc_cn = {
    {2,0,0}, euc_cn_to_ucs, euc_cn_from_ucs
};
const charset_spec charset_CS_EUC_CN = {
    CS_EUC_CN, read_euc, write_euc, &euc_cn
};

/*
 * EUC-KR encodes KS X 1001 only.
 */
static long int euc_kr_to_ucs(unsigned long state)
{
    switch (state >> 28) {
      case 1: return ksx1001_to_unicode(((state >> 8) & 0xFF) - 0xA1,
				       ((state     ) & 0xFF) - 0xA1);
      default: return ERROR;
    }
}
static unsigned long euc_kr_from_ucs(long int ucs)
{
    int r, c;
    if (unicode_to_ksx1001(ucs, &r, &c))
	return 0x10000000 | ((r+0xA1) << 8) | (c+0xA1);
    else
	return 0;
}
static const struct euc euc_kr = {
    {2,0,0}, euc_kr_to_ucs, euc_kr_from_ucs
};
const charset_spec charset_CS_EUC_KR = {
    CS_EUC_KR, read_euc, write_euc, &euc_kr
};

/*
 * EUC-JP encodes several character sets.
 */
static long int euc_jp_to_ucs(unsigned long state)
{
    switch (state >> 28) {
      case 1: return jisx0208_to_unicode(((state >> 8) & 0xFF) - 0xA1,
					 ((state     ) & 0xFF) - 0xA1);
      case 2:
	/*
	 * This is the top half of JIS X 0201. That means A1-DF map
	 * to FF61-FF9F, and nothing else is valid.
	 */
	{
	    int c = state & 0xFF;
	    if (c >= 0xA1 && c <= 0xDF)
		return c + (0xFF61 - 0xA1);
	    else
		return ERROR;
	}
	/* (no break needed since all control paths have returned) */
      case 3: return jisx0212_to_unicode(((state >> 8) & 0xFF) - 0xA1,
					 ((state     ) & 0xFF) - 0xA1);
      default: return ERROR;	       /* placate optimisers */
    }
}
static unsigned long euc_jp_from_ucs(long int ucs)
{
    int r, c;
    if (ucs >= 0xFF61 && ucs <= 0xFF9F)
	return 0x20000000 | (ucs - (0xFF61 - 0xA1));
    else if (unicode_to_jisx0208(ucs, &r, &c))
	return 0x10000000 | ((r+0xA1) << 8) | (c+0xA1);
    else if (unicode_to_jisx0212(ucs, &r, &c))
	return 0x30000000 | ((r+0xA1) << 8) | (c+0xA1);
    else
	return 0;
}
static const struct euc euc_jp = {
    {2,1,2}, euc_jp_to_ucs, euc_jp_from_ucs
};
const charset_spec charset_CS_EUC_JP = {
    CS_EUC_JP, read_euc, write_euc, &euc_jp
};

/*
 * EUC-TW encodes CNS 11643 (all planes).
 */
static long int euc_tw_to_ucs(unsigned long state)
{
    int plane;
    switch (state >> 28) {
      case 1: return cns11643_to_unicode(0, ((state >> 8) & 0xFF) - 0xA1,
					    ((state     ) & 0xFF) - 0xA1);
      case 2:
	plane = ((state >> 8) & 0xFF) - 0xA1;
	if (plane >= 7) return ERROR;
	return cns11643_to_unicode(plane, ((state >> 8) & 0xFF) - 0xA1,
					  ((state     ) & 0xFF) - 0xA1);
      default: return ERROR;
    }
}
static unsigned long euc_tw_from_ucs(long int ucs)
{
    int p, r, c;
    if (unicode_to_cns11643(ucs, &p, &r, &c)) {
	if (p == 0)
	    return 0x10000000 | ((r+0xA1) << 8) | (c+0xA1);
	else
	    return 0x20000000 |
		((p + 0xA1) << 16) | ((r+0xA1) << 8) | (c+0xA1);
    } else
	return 0;
}
static const struct euc euc_tw = {
    {2,3,0}, euc_tw_to_ucs, euc_tw_from_ucs
};
const charset_spec charset_CS_EUC_TW = {
    CS_EUC_TW, read_euc, write_euc, &euc_tw
};

#else /* ENUM_CHARSETS */

ENUM_CHARSET(CS_EUC_CN)
ENUM_CHARSET(CS_EUC_KR)
ENUM_CHARSET(CS_EUC_JP)
ENUM_CHARSET(CS_EUC_TW)

#endif /* ENUM_CHARSETS */
