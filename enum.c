/*
 * enum.c - enumerate all charsets defined by the library.
 * 
 * This file maintains a list of every other source file which
 * contains ENUM_CHARSET definitions. It #includes each one with
 * ENUM_CHARSETS defined, which causes those source files to do
 * nothing at all except call the ENUM_CHARSET macro on each
 * charset they define.
 * 
 * This file in turn is included from various other places, with
 * the ENUM_CHARSET macro defined to various different things. This
 * allows us to have multiple implementations of the master charset
 * lookup table (a static one and a dynamic one).
 */

#define ENUM_CHARSETS
#include "sbcsdat.c"
#include "utf8.c"
#include "utf7.c"
#include "utf16.c"
#include "euc.c"
#include "iso2022.c"
#include "iso2022s.c"
#include "big5enc.c"
#include "shiftjis.c"
#include "hz.c"
#include "cp949.c"
#undef ENUM_CHARSETS
