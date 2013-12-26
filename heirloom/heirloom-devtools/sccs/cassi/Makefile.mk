.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -I../hdr $(WARN) $<


OBJ = cmrcheck.o deltack.o error.o filehand.o gf.o

all: libcassi.a

libcassi.a: $(OBJ)
	$(AR) -rv $@ $(OBJ)
	$(RANLIB) $@

install: all

clean:
	rm -f libcassi.a $(OBJ) core log *~

mrproper: clean

cmrcheck.o: cmrcheck.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/filehand.h ../hdr/i18n.h
deltack.o: deltack.c ../hdr/had.h ../hdr/defines.h ../hdr/macros.h \
  ../hdr/fatal.h ../hdr/fatal.h ../hdr/filehand.h ../hdr/i18n.h \
  ../hdr/ccstypes.h
error.o: error.c
filehand.o: filehand.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/filehand.h
gf.o: gf.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h ../hdr/fatal.h \
  ../hdr/filehand.h ../hdr/i18n.h
