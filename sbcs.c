/*
 * sbcs.c - routines to handle single-byte character sets.
 */

#include "charset.h"
#include "internal.h"

/*
 * The charset_spec for any single-byte character set should
 * provide read_sbcs() as its read function, and its `data' field
 * should be a wchar_t string constant containing the 256 entries
 * of the translation table.
 */

long int sbcs_to_unicode(const struct sbcs_data *sd, long int input_chr)
{
    return sd->sbcs2ucs[input_chr];
}

void read_sbcs(charset_spec const *charset, long int input_chr,
	       charset_state *state,
	       void (*emit)(void *ctx, long int output), void *emitctx)
{
    const struct sbcs_data *sd = charset->data;

    UNUSEDARG(state);

    emit(emitctx, sbcs_to_unicode(sd, input_chr));
}

long int sbcs_from_unicode(const struct sbcs_data *sd, long int input_chr)
{
    int i, j, k, c;

    /*
     * Binary-search in the ucs2sbcs table.
     */
    i = -1;
    j = sd->nvalid;
    while (i+1 < j) {
	k = (i+j)/2;
	c = sd->ucs2sbcs[k];
	if (input_chr < (long int)sd->sbcs2ucs[c])
	    j = k;
	else if (input_chr > (long int)sd->sbcs2ucs[c])
	    i = k;
	else {
	    return c;
	}
    }
    return ERROR;
}

int write_sbcs(charset_spec const *charset, long int input_chr,
	       charset_state *state,
	       void (*emit)(void *ctx, long int output), void *emitctx)
{
    const struct sbcs_data *sd = charset->data;
    long int ret;

    UNUSEDARG(state);

    if (input_chr == -1)
	return TRUE;		       /* stateless; no cleanup required */

    ret = sbcs_from_unicode(sd, input_chr);
    if (ret == ERROR)
	return FALSE;

    emit(emitctx, ret);
    return TRUE;
}
