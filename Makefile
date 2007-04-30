# -*- make -*-
#
# Makefile for libcharset.

# This Makefile should be sufficient to build libcharset and its
# demo application all on its own. However, it's also a valid
# Makefile _fragment_ which can be linked in to another program
# Makefile to allow libcharset to be built directly into its
# binary.

# To include this as part of another Makefile, you need to:
#
#  - Define $(LIBCHARSET_SRCDIR) to be a directory prefix (i.e.
#    probably ending in a slash) which allows access to the
#    libcharset source files.
#
#  - Define $(LIBCHARSET_OBJDIR) to be a directory prefix (i.e.
#    probably ending in a slash) which allows access to the
#    directory where the libcharset object files need to be put.
#
#  - Define $(LIBCHARSET_OBJPFX) to be a filename prefix to be
#    applied to the libcharset object files (in case, for example,
#    the file names clash with those of the main application, and
#    you need to call them cs-*.o to resolve the clash).
#
#  - Define $(LIBCHARSET_GENPFX) to be a prefix to be added to
#    targets such as `all' and `clean'. (Mostly the point of this
#    is to get those targets out of the way for the Makefile
#    fragment including us.)
#
#  - If you need your compiler to use the -MD flag, define $(MD) to
#    be `-MD'.
#
# This Makefile fragment will then define rules for building each
# object file, and will in turn define $(LIBCHARSET_OBJS) to be
# what you need to add to your link line.

$(LIBCHARSET_GENPFX)all: \
	$(LIBCHARSET_OBJDIR)libcharset.a \
	$(LIBCHARSET_OBJDIR)convcs \
	$(LIBCHARSET_OBJDIR)cstable

$(LIBCHARSET_OBJDIR)convcs: $(LIBCHARSET_SRCDIR)test.c \
	$(LIBCHARSET_OBJDIR)libcharset.a
	$(CC) $(CFLAGS) -o $(LIBCHARSET_OBJDIR)convcs \
		$(LIBCHARSET_SRCDIR)test.c \
		$(LIBCHARSET_OBJDIR)libcharset.a

$(LIBCHARSET_OBJDIR)cstable: $(LIBCHARSET_SRCDIR)cstable.c \
	$(LIBCHARSET_OBJDIR)libcharset.a
	$(CC) $(CFLAGS) -o $(LIBCHARSET_OBJDIR)cstable \
		$(LIBCHARSET_SRCDIR)cstable.c \
		$(LIBCHARSET_OBJDIR)libcharset.a

LIBCHARSET_OBJS = \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)big5enc.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)big5set.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)cns11643.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)cp949.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)emacsenc.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)euc.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)fromucs.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)gb2312.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)hz.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)iso2022.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)iso2022s.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)istate.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)jisx0208.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)jisx0212.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)ksx1001.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)locale.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)localenc.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)macenc.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)mimeenc.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)sbcs.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)sbcsdat.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)shiftjis.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)slookup.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)superset.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)toucs.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)utf16.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)utf7.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)utf8.o \
	$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)xenc.o \
	# end of list

$(LIBCHARSET_OBJDIR)libcharset.a: $(LIBCHARSET_OBJS)
	ar rcs $@ $(LIBCHARSET_OBJS)

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)big5enc.o: \
	$(LIBCHARSET_SRCDIR)big5enc.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)big5set.o: \
	$(LIBCHARSET_SRCDIR)big5set.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)cns11643.o: \
	$(LIBCHARSET_SRCDIR)cns11643.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)cp949.o: \
	$(LIBCHARSET_SRCDIR)cp949.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)emacsenc.o: \
	$(LIBCHARSET_SRCDIR)emacsenc.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)euc.o: \
	$(LIBCHARSET_SRCDIR)euc.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)fromucs.o: \
	$(LIBCHARSET_SRCDIR)fromucs.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)gb2312.o: \
	$(LIBCHARSET_SRCDIR)gb2312.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)hz.o: \
	$(LIBCHARSET_SRCDIR)hz.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)iso2022.o: \
	$(LIBCHARSET_SRCDIR)iso2022.c \
	$(LIBCHARSET_OBJDIR)sbcsdat.h
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)iso2022s.o: \
	$(LIBCHARSET_SRCDIR)iso2022s.c \
	$(LIBCHARSET_OBJDIR)sbcsdat.h
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)istate.o: \
	$(LIBCHARSET_SRCDIR)istate.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)jisx0208.o: \
	$(LIBCHARSET_SRCDIR)jisx0208.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)jisx0212.o: \
	$(LIBCHARSET_SRCDIR)jisx0212.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)ksx1001.o: \
	$(LIBCHARSET_SRCDIR)ksx1001.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)locale.o: \
	$(LIBCHARSET_SRCDIR)locale.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)localenc.o: \
	$(LIBCHARSET_SRCDIR)localenc.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)macenc.o: \
	$(LIBCHARSET_SRCDIR)macenc.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)mimeenc.o: \
	$(LIBCHARSET_SRCDIR)mimeenc.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)sbcs.o: \
	$(LIBCHARSET_SRCDIR)sbcs.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)shiftjis.o: \
	$(LIBCHARSET_SRCDIR)shiftjis.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)slookup.o: \
	$(LIBCHARSET_SRCDIR)slookup.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)superset.o: \
	$(LIBCHARSET_SRCDIR)superset.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)toucs.o: \
	$(LIBCHARSET_SRCDIR)toucs.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)utf16.o: \
	$(LIBCHARSET_SRCDIR)utf16.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)utf7.o: \
	$(LIBCHARSET_SRCDIR)utf7.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)utf8.o: \
	$(LIBCHARSET_SRCDIR)utf8.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)xenc.o: \
	$(LIBCHARSET_SRCDIR)xenc.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

# This object file is special, because its source file is itself
# generated - and therefore goes in the object directory.

$(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)sbcsdat.o: \
	$(LIBCHARSET_OBJDIR)sbcsdat.c
	$(CC) $(CFLAGS) $(MD) -c -o $@ $<

$(LIBCHARSET_OBJDIR)sbcsdat.c $(LIBCHARSET_OBJDIR)sbcsdat.h: \
	$(LIBCHARSET_SRCDIR)sbcs.dat \
	$(LIBCHARSET_SRCDIR)sbcsgen.pl
	perl $(LIBCHARSET_SRCDIR)sbcsgen.pl \
		$(LIBCHARSET_SRCDIR)sbcs.dat \
		$(LIBCHARSET_OBJDIR)sbcsdat.c \
		$(LIBCHARSET_OBJDIR)sbcsdat.h

$(LIBCHARSET_GENPFX)clean:
	rm -f $(LIBCHARSET_OBJDIR)$(LIBCHARSET_OBJPFX)*.o \
		$(LIBCHARSET_OBJDIR)libcharset.a \
		$(LIBCHARSET_OBJDIR)sbcsdat.c \
		$(LIBCHARSET_OBJDIR)sbcsdat.h \
		$(LIBCHARSET_OBJDIR)convcs
