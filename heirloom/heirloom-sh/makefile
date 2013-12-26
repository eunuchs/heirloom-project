#
# Root directory. Mainly useful for package building; leave empty for
# normal installation.
#
ROOT=

#
# The destination directory for the "sh" and "jsh" binaries.
#
SV3BIN=/usr/5bin

#
# Location for manual pages (with man1 below).
#
MANDIR=/usr/share/man/5man

#
# Enable this definition if spell checking should be done for the
# "cd" special command.
#
SPELL=-DSPELL

#
# A BSD-compatible install command.
#
UCBINST=/usr/ucb/install

#
# The strip command that is used at installation time.
#
STRIP=strip -s -R .comment -R .note

#
# A command to create the link from "jsh" to "sh".
#
LNS=ln -s

#
# Uncomment the following line to compile with diet libc. -Ifakewchar might
# be usable for other environments without wide character support too.
#
#CC=diet gcc -Ifakewchar

#
# Compiler flags.
#
CFLAGS=-O

#
# Flags for the C preprocessor.
#
CPPFLAGS=-D_GNU_SOURCE

#
# A define for large file support, if necessary.
#
LARGEF=-D_FILE_OFFSET_BITS=64L

#
# The compiler warning options.
#
#WARN=

#
# End of adjustable settings.
#

OBJ = args.o blok.o bltin.o cmd.o ctype.o defs.o echo.o error.o \
	expand.o fault.o func.o hash.o hashserv.o io.o jobs.o \
	macro.o main.o msg.o name.o print.o pwd.o service.o \
	setbrk.o stak.o string.o test.o ulimit.o word.o xec.o \
	gmatch.o getopt.o strsig.o version.o mapmalloc.o umask.o

.c.o: ; $(CC) -c $(CFLAGS) $(CPPFLAGS) $(LARGEF) $(SPELL) $(WARN) $<

all: sh jsh sh.1.out

sh: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o sh

jsh: sh
	rm -f jsh
	$(LNS) sh jsh

sh.1.out: sh.1
	test "x$(SPELL)" != x && cat sh.1 >$@ || \
		sed '/BEGIN SPELL/,/END SPELL/d' <sh.1 >$@

install: all
	test -d $(ROOT)$(SV3BIN) || mkdir -p $(ROOT)$(SV3BIN)
	$(UCBINST) -c -m 755 sh $(ROOT)$(SV3BIN)/sh
	$(STRIP) $(ROOT)$(SV3BIN)/sh
	rm -f $(ROOT)$(SV3BIN)/jsh
	cd $(ROOT)$(SV3BIN) && $(LNS) sh jsh
	test -d $(ROOT)$(MANDIR)/man1 || mkdir -p $(ROOT)$(MANDIR)/man1
	$(UCBINST) -c -m 644 sh.1.out $(ROOT)$(MANDIR)/man1/sh.1
	rm -f $(ROOT)$(MANDIR)/man1/jsh.1
	cd $(ROOT)$(MANDIR)/man1 && $(LNS) sh.1 jsh.1

maninstall: sh.1.out
	$(UCBINST) -c -m 644 sh.1.out $(ROOT)$(MANDIR)/man1/sh.1

diet:
	$(MAKE) CC='diet gcc -Ifakewchar' CFLAGS='-Os -fomit-frame-pointer' \
		STRIP='strip -s -R .comment -R .note' WERROR=

dietinstall:
	ldd sh >/dev/null 2>&1 && { echo dynamic; exit 1; } || :
	$(MAKE) install SV3BIN=/sbin

world:
	$(MAKE) clean
	$(MAKE)
	sudo $(MAKE) install
	$(MAKE) clean
	$(MAKE) diet
	sudo $(MAKE) dietinstall
	$(MAKE) clean

clean:
	rm -f $(OBJ) sh jsh sh.1.out core log *~

mrproper: clean

PKGROOT = /var/tmp/heirloom-sh
PKGTEMP = /var/tmp
PKGPROTO = pkgproto

heirloom-sh.pkg: all
	rm -rf $(PKGROOT)
	mkdir -p $(PKGROOT)
	$(MAKE) ROOT=$(PKGROOT) install
	rm -f $(PKGPROTO)
	echo 'i pkginfo' >$(PKGPROTO)
	(cd $(PKGROOT) && find . -print | pkgproto) | >>$(PKGPROTO) sed 's:^\([df] [^ ]* [^ ]* [^ ]*\) .*:\1 root root:; s:^f\( [^ ]* etc/\):v \1:; s:^f\( [^ ]* var/\):v \1:; s:^\(s [^ ]* [^ ]*=\)\([^/]\):\1./\2:'
	rm -rf $(PKGTEMP)/$@
	pkgmk -a `uname -m` -d $(PKGTEMP) -r $(PKGROOT) -f $(PKGPROTO) $@
	pkgtrans -o -s $(PKGTEMP) `pwd`/$@ $@
	rm -rf $(PKGROOT) $(PKGPROTO) $(PKGTEMP)/$@

args.o: args.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
blok.o: blok.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
bltin.o: bltin.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h \
  sym.h hash.h
cmd.o: cmd.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h sym.h
ctype.o: ctype.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
defs.o: defs.c mode.h name.h
echo.o: echo.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
error.o: error.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
expand.o: expand.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
fault.o: fault.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
func.o: func.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
getopt.o: getopt.c
gmatch.o: gmatch.c mbtowi.h
hash.o: hash.c hash.h defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
hashserv.o: hashserv.c hash.h defs.h mac.h mode.h name.h stak.h brkincr.h \
  ctype.h
io.o: io.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h dup.h
jobs.o: jobs.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
macro.o: macro.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h \
  sym.h
main.o: main.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h sym.h \
  hash.h timeout.h dup.h
mapmalloc.o: mapmalloc.c
msg.o: msg.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h sym.h
name.o: name.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
print.o: print.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
pwd.o: pwd.c mac.h defs.h mode.h name.h stak.h brkincr.h ctype.h
service.o: service.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
setbrk.o: setbrk.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
stak.o: stak.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
string.o: string.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
strsig.o: strsig.c
test.o: test.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
ulimit.o: ulimit.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
umask.o: umask.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h
version.o: version.c
word.o: word.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h sym.h
xec.o: xec.c defs.h mac.h mode.h name.h stak.h brkincr.h ctype.h sym.h \
  hash.h
