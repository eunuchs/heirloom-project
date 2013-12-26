.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -I../hdr $(WARN) $<


OBJ = abspath.o any.o cat.o dname.o fatal.o fdfopen.o fmalloc.o \
	had.o imatch.o index.o lockit.o patoi.o repl.o satoi.o \
	setsig.o sname.o strend.o trnslat.o userexit.o xcreat.o \
	xlink.o xmsg.o xopen.o xpipe.o xunlink.o zero.o getopt.o

all: libmpw.a

libmpw.a: $(OBJ)
	$(AR) -rv $@ $(OBJ)
	$(RANLIB) $@

install: all

clean:
	rm -f libmpw.a $(OBJ) core log *~

mrproper: clean

abspath.o: abspath.c
any.o: any.c
cat.o: cat.c
dname.o: dname.c ../hdr/macros.h
fatal.o: fatal.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/had.h
fdfopen.o: fdfopen.c ../hdr/macros.h
fmalloc.o: fmalloc.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
had.o: had.c ../hdr/had.h
imatch.o: imatch.c
index.o: index.c
lockit.o: lockit.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/i18n.h ../hdr/ccstypes.h
patoi.o: patoi.c
repl.o: repl.c ../hdr/defines.h
satoi.o: satoi.c ../hdr/macros.h
setsig.o: setsig.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/i18n.h
sname.o: sname.c
strend.o: strend.c
trnslat.o: trnslat.c
userexit.o: userexit.c
xcreat.o: xcreat.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/i18n.h ../hdr/ccstypes.h
xlink.o: xlink.c ../hdr/i18n.h ../hdr/defines.h ../hdr/macros.h \
  ../hdr/fatal.h ../hdr/fatal.h
xmsg.o: xmsg.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
xopen.o: xopen.c ../hdr/i18n.h ../hdr/ccstypes.h ../hdr/defines.h \
  ../hdr/macros.h ../hdr/fatal.h ../hdr/fatal.h
xpipe.o: xpipe.c ../hdr/i18n.h ../hdr/defines.h ../hdr/macros.h \
  ../hdr/fatal.h ../hdr/fatal.h
xunlink.o: xunlink.c ../hdr/i18n.h ../hdr/defines.h ../hdr/macros.h \
  ../hdr/fatal.h ../hdr/fatal.h
zero.o: zero.c
