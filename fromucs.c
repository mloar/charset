/*
 * fromucs.c - convert Unicode to other character sets.
 */

#include "charset.h"
#include "internal.h"

struct charset_emit_param {
    char *output;
    int outlen;
    int stopped;
};

static void charset_emit(void *ctx, long int output)
{
    struct charset_emit_param *param = (struct charset_emit_param *)ctx;

    if (param->outlen > 0) {
	*param->output++ = output;
	param->outlen--;
    } else {
	param->stopped = 1;
    }
}

int charset_from_unicode(const wchar_t **input, int *inlen,
			 char *output, int outlen,
			 int charset, charset_state *state, int *error)
{
    charset_spec const *spec = charset_find_spec(charset);
    charset_state localstate = CHARSET_INIT_STATE;
    struct charset_emit_param param;
    int locallen;

    if (!input) {
	locallen = 1;
	inlen = &locallen;
    }

    param.output = output;
    param.outlen = outlen;
    param.stopped = 0;

    if (state)
	localstate = *state;	       /* structure copy */
    if (error)
	*error = FALSE;

    while (*inlen > 0) {
	int lenbefore = param.output - output;
	int ret;

	if (input)
	    ret = spec->write(spec, **input, &localstate,
			      charset_emit, &param);
	else
	    ret = spec->write(spec, -1, &localstate, charset_emit, &param);
	if (error && !ret) {
	    /*
	     * We have hit a difficult character, which the user
	     * wants to know about. Leave now.
	     */
	    *error = TRUE;
	    return lenbefore;
	}
	if (param.stopped) {
	    /*
	     * The emit function has _tried_ to output some
	     * characters, but ran up against the end of the
	     * buffer. Leave immediately, and return what happened
	     * _before_ attempting to process this character.
	     */
	    return lenbefore;
	}
	if (state)
	    *state = localstate;       /* structure copy */
	if (input)
	    (*input)++;
	(*inlen)--;
    }
    return param.output - output;
}
