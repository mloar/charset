/*
 * internal.h - internal header stuff for the charset library.
 */

#ifndef charset_internal_h
#define charset_internal_h

/* This invariably comes in handy */
#define lenof(x) ( sizeof((x)) / sizeof(*(x)) )

/* This is an invalid Unicode value used to indicate an error. */
#define ERROR 0xFFFFL		       /* Unicode value representing error */

#undef TRUE
#define TRUE 1
#undef FALSE
#define FALSE 0

typedef struct charset_spec charset_spec;
typedef struct sbcs_data sbcs_data;

struct charset_spec {
    int charset;		       /* numeric identifier */

    /*
     * A function to read the character set and output Unicode
     * characters. The `emit' function expects to get Unicode chars
     * passed to it; it should be sent ERROR for any encoding error
     * on the input.
     */
    void (*read)(charset_spec const *charset, long int input_chr,
		 charset_state *state,
		 void (*emit)(void *ctx, long int output), void *emitctx);
    /*
     * A function to read Unicode characters and output in this
     * character set. The `emit' function expects to get byte
     * values passed to it.
     * 
     * A non-representable input character should cause a FALSE
     * return, _before_ `emit' is called. Successful conversion
     * causes a TRUE return.
     * 
     * If `input_chr' is -1, this function must revert the encoding
     * state to any default required at the end of a piece of
     * encoded text.
     */
    int (*write)(charset_spec const *charset, long int input_chr,
		 charset_state *state,
		 void (*emit)(void *ctx, long int output), void *emitctx);
    void const *data;
};

/*
 * This is the format of `data' used by the SBCS read and write
 * functions; so it's the format used in all SBCS definitions.
 */
struct sbcs_data {
    /*
     * This is a simple mapping table converting each SBCS position
     * to a Unicode code point. Some positions may contain ERROR,
     * indicating that that byte value is not defined in the SBCS
     * in question and its occurrence in input is an error.
     */
    unsigned long sbcs2ucs[256];

    /*
     * This lookup table is used to convert Unicode back to the
     * SBCS. It consists of the valid byte values in the SBCS,
     * sorted in order of their Unicode translation. So given a
     * Unicode value U, you can do a binary search on this table
     * using the above table as a lookup: when testing the Xth
     * position in this table, you branch according to whether
     * sbcs2ucs[ucs2sbcs[X]] is less than, greater than, or equal
     * to U.
     * 
     * Note that since there may be fewer than 256 valid byte
     * values in a particular SBCS, we must supply the length of
     * this table as well as the contents.
     */
    unsigned char ucs2sbcs[256];
    int nvalid;
};

/*
 * Prototypes for internal library functions.
 */
charset_spec const *charset_find_spec(int charset);
void read_sbcs(charset_spec const *charset, long int input_chr,
	       charset_state *state,
	       void (*emit)(void *ctx, long int output), void *emitctx);
int write_sbcs(charset_spec const *charset, long int input_chr,
	       charset_state *state,
	       void (*emit)(void *ctx, long int output), void *emitctx);
long int sbcs_to_unicode(const struct sbcs_data *sd, long int input_chr);
long int sbcs_from_unicode(const struct sbcs_data *sd, long int input_chr);

void read_utf8(charset_spec const *charset, long int input_chr,
	       charset_state *state,
	       void (*emit)(void *ctx, long int output), void *emitctx);
int write_utf8(charset_spec const *charset, long int input_chr,
	       charset_state *state,
	       void (*emit)(void *ctx, long int output),
	       void *emitctx);

long int big5_to_unicode(int r, int c);
int unicode_to_big5(long int unicode, int *r, int *c);
long int cns11643_to_unicode(int p, int r, int c);
int unicode_to_cns11643(long int unicode, int *p, int *r, int *c);
long int cp949_to_unicode(int r, int c);
int unicode_to_cp949(long int unicode, int *r, int *c);
long int ksx1001_to_unicode(int r, int c);
int unicode_to_ksx1001(long int unicode, int *r, int *c);
long int gb2312_to_unicode(int r, int c);
int unicode_to_gb2312(long int unicode, int *r, int *c);
long int jisx0208_to_unicode(int r, int c);
int unicode_to_jisx0208(long int unicode, int *r, int *c);
long int jisx0212_to_unicode(int r, int c);
int unicode_to_jisx0212(long int unicode, int *r, int *c);

/*
 * Placate compiler warning about unused parameters, of which we
 * expect to have some in this library.
 */
#define UNUSEDARG(x) ( (x) = (x) )

#endif /* charset_internal_h */
