INC = -I../../hdrs -I../../libgendb -I../../libpkg -I../../libinst \
	-I../../libspmizones -I../../libadm

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INC) $(PATHS) $(WARN) $<

PKGLIBS = -L../../libinst -linst -L../../libpkg -lpkg -L../../libgendb -lgendb \
	-L../../libadm -ladm \
	-L$(CCSDIR)/lib -ll

BIN = pkgchk
OBJ = checkmap.o ckentry.o main.o ../../version/version.o

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(PKGLIBS) $(LIBS) -o $@

install: all
	mkdir -p $(ROOT)$(SBINDIR)
	$(INSTALL) -c -m 755 pkgchk $(ROOT)$(SBINDIR)/pkgchk
	$(STRIP) $(ROOT)$(SBINDIR)/pkgchk

clean:
	rm -f $(BIN) $(OBJ) core log

mrproper: clean

$(BIN): ../../libadm/libadm.a ../../libgendb/libgendb.a \
	../../libinst/libinst.a ../../libpkg/libpkg.a

checkmap.o: checkmap.c ../../hdrs/pkgstrct.h ../../hdrs/pkglocs.h \
  ../../libpkg/pkglib.h ../../hdrs/pkgdev.h ../../libpkg/pkgerr.h \
  ../../libpkg/keystore.h ../../libpkg/cfext.h ../../hdrs/libadm.h \
  ../../hdrs/sys/vtoc.h ../../hdrs/sys/dklabel.h ../../hdrs/pkginfo.h \
  ../../hdrs/valtools.h ../../hdrs/install.h ../../hdrs/libinst.h \
  ../../libpkg/cfext.h
ckentry.o: ckentry.c ../../hdrs/pkgstrct.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgdev.h ../../libpkg/pkgerr.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h ../../hdrs/install.h ../../hdrs/libadm.h \
  ../../hdrs/sys/vtoc.h ../../hdrs/sys/dklabel.h ../../hdrs/pkginfo.h \
  ../../hdrs/valtools.h ../../hdrs/install.h ../../hdrs/libinst.h \
  ../../libpkg/pkglib.h ../../libpkg/cfext.h
main.o: main.c ../../hdrs/pkginfo.h ../../hdrs/pkglocs.h \
  ../../hdrs/pkgstrct.h ../../hdrs/pkgtrans.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgdev.h ../../libpkg/pkgerr.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h ../../hdrs/libadm.h ../../hdrs/sys/vtoc.h \
  ../../hdrs/sys/dklabel.h ../../hdrs/valtools.h ../../hdrs/install.h \
  ../../hdrs/libinst.h ../../libpkg/cfext.h
