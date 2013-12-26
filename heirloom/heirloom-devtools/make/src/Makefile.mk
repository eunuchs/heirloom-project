HDRSDIR = $(PREFIX)/share/lib/make

OBJ = ar.o depvar.o dist.o dmake.o doname.o dosys.o files.o globals.o \
	implicit.o macro.o main.o make.o misc.o nse.o nse_printdep.o \
	parallel.o pmake.o read.o read2.o rep.o state.o getopt.o version.o

XFLAGS = -I../include -D_GNU_SOURCE -DSUNOS4_AND_AFTER \
	-DHDRSDIR='"$(HDRSDIR)"' \
	-DSHELL='"$(SHELL)"' -DPOSIX_SHELL='"$(POSIX_SHELL)"'

.c.o:
	$(CC) $(CFLAGS) $(WARN) $(CPPFLAGS) $(XFLAGS) -c $<

.cc.o:
	$(CXX) $(CXXFLAGS) $(WARN) $(CPPFLAGS) $(XFLAGS) -c $<


MAKELIBS = -L../bsd -lbsd -L../makestate -lmakestate \
	-L../mksh -lmksh -L../vroot -lvroot \
	-L../mksdmsi18n -lmksdmsi18n

MAKEARCHIVES = ../bsd/libbsd.a ../makestate/libmakestate.a \
	../mksdmsi18n/libmksdmsi18n.a ../mksh/libmksh.a ../vroot/libvroot.a

all: make make_sus

make: $(OBJ) $(MAKEARCHIVES)
	$(CXX) $(LDFLAGS) $(OBJ) $(MAKELIBS) $(LIBS) -o $@

make_sus: $(OBJ) posix.o $(MAKEARCHIVES)
	$(CXX) $(LDFLAGS) $(OBJ) posix.o $(MAKELIBS) $(LIBS) -o $@

install: all
	mkdir -p $(ROOT)$(BINDIR) $(ROOT)$(HDRSDIR)
	$(INSTALL) -c make $(ROOT)$(BINDIR)/make
	$(STRIP) $(ROOT)$(BINDIR)/make
	rm -f $(ROOT)$(LIBDIR)/svr4.make
	ln -s ../bin/make $(ROOT)$(LIBDIR)/svr4.make
	$(INSTALL) -c make_sus $(ROOT)$(SUSBIN)/make
	$(STRIP) $(ROOT)$(SUSBIN)/make
	$(INSTALL) -c -m 644 make.rules.file $(ROOT)$(HDRSDIR)/make.rules
	$(INSTALL) -c -m 644 svr4.make.rules.file $(ROOT)$(HDRSDIR)/svr4.make.rules
	test -d $(ROOT)$(MANDIR)/man1 || mkdir -p $(ROOT)$(MANDIR)/man1
	$(INSTALL) -c -m 644 make.1 $(ROOT)$(MANDIR)/man1/make.1

clean:
	rm -f make make_sus $(OBJ) posix.o core log *~

mrproper: clean

ar.o: ar.cc ../include/avo/avo_alloca.h ../include/mk/defs.h \
  ../include/mksh/defs.h ../include/avo/intl.h ../include/vroot/vroot.h \
  ../include/mksh/misc.h
depvar.o: depvar.cc ../include/mk/defs.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h ../include/mksh/misc.h
dist.o: dist.cc
dmake.o: dmake.cc
doname.o: doname.cc ../include/avo/avo_alloca.h ../include/mk/defs.h \
  ../include/mksh/defs.h ../include/avo/intl.h ../include/vroot/vroot.h \
  ../include/mksh/i18n.h ../include/mksh/macro.h ../include/mksh/misc.h
dosys.o: dosys.cc ../include/mk/defs.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h ../include/mksh/dosys.h \
  ../include/mksh/misc.h
files.o: files.cc ../include/mk/defs.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h ../include/mksh/macro.h \
  ../include/mksh/misc.h
globals.o: globals.cc ../include/mk/defs.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h
implicit.o: implicit.cc ../include/mk/defs.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h ../include/mksh/macro.h \
  ../include/mksh/misc.h
macro.o: macro.cc ../include/mk/defs.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h ../include/mksh/macro.h \
  ../include/mksh/misc.h
main.o: main.cc ../include/bsd/bsd.h ../include/mk/copyright.h \
  ../include/mk/defs.h ../include/mksh/defs.h ../include/avo/intl.h \
  ../include/vroot/vroot.h ../include/mksdmsi18n/mksdmsi18n.h \
  ../include/mksh/macro.h ../include/mksh/misc.h \
  ../include/vroot/report.h
make.o: make.cc
misc.o: misc.cc ../include/mk/defs.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h ../include/mksh/macro.h \
  ../include/mksh/misc.h ../include/vroot/report.h
nse.o: nse.cc
nse_printdep.o: nse_printdep.cc ../include/mk/defs.h \
  ../include/mksh/defs.h ../include/avo/intl.h ../include/vroot/vroot.h \
  ../include/mksh/misc.h
parallel.o: parallel.cc
pmake.o: pmake.cc
read.o: read.cc ../include/avo/avo_alloca.h ../include/mk/defs.h \
  ../include/mksh/defs.h ../include/avo/intl.h ../include/vroot/vroot.h \
  ../include/mksh/macro.h ../include/mksh/misc.h ../include/mksh/read.h
read2.o: read2.cc ../include/mk/defs.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h ../include/mksh/dosys.h \
  ../include/mksh/macro.h ../include/mksh/misc.h
rep.o: rep.cc ../include/mk/defs.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h ../include/mksh/misc.h \
  ../include/vroot/report.h
state.o: state.cc ../include/mk/defs.h ../include/mksh/defs.h \
  ../include/avo/intl.h ../include/vroot/vroot.h ../include/mksh/misc.h
