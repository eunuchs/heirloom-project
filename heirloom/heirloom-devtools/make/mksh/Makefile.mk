OBJ = dosys.o globals.o i18n.o macro.o misc.o mksh.o read.o wcslen.o posix.o

.c.o:
	$(CC) $(CFLAGS) $(WARN) $(CPPFLAGS) -I../include -c $<

.cc.o:
	$(CXX) $(CXXFLAGS) $(WARN) $(CPPFLAGS) -I../include -c $<

all: libmksh.a

libmksh.a: $(OBJ)
	$(AR) -rv $@ $(OBJ)
	$(RANLIB) $@

install: all

clean:
	rm -f libmksh.a $(OBJ) core log *~

mrproper: clean

dosys.o: dosys.cc ../include/avo/avo_alloca.h ../include/mksh/dosys.h \
  ../include/mksh/defs.h ../include/avo/intl.h ../include/vroot/vroot.h \
  ../include/mksh/macro.h ../include/mksh/misc.h \
  ../include/mksdmsi18n/mksdmsi18n.h
globals.o: globals.cc ../include/mksh/globals.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h
i18n.o: i18n.cc ../include/mksh/i18n.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h ../include/mksh/misc.h
macro.o: macro.cc ../include/mksh/dosys.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h ../include/mksh/i18n.h \
  ../include/mksh/macro.h ../include/mksh/misc.h ../include/mksh/read.h \
  ../include/mksdmsi18n/mksdmsi18n.h
misc.o: misc.cc ../include/bsd/bsd.h ../include/mksh/i18n.h \
  ../include/mksh/defs.h ../include/avo/intl.h ../include/vroot/vroot.h \
  ../include/mksh/misc.h ../include/mksdmsi18n/mksdmsi18n.h
mksh.o: mksh.cc ../include/mksh/dosys.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h ../include/mksh/misc.h \
  ../include/mksh/mksh.h ../include/mksdmsi18n/mksdmsi18n.h
read.o: read.cc ../include/mksh/misc.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h ../include/mksh/read.h \
  ../include/mksdmsi18n/mksdmsi18n.h
