INC = -I../../hdrs -I../../libgendb -I../../libpkg -I../../libinst \
	-I../../libspmizones -I../../libadm

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INC) $(PATHS) $(WARN) $<

PKGLIBS = -L../../libinst -linst -L../../libpkg -lpkg -L../../libgendb -lgendb \
	-L../../libadm -ladm \
	-L$(CCSDIR)/lib -ll

BIN = pkgremove
OBJ = check.o delmap.o main.o predepend.o quit.o special.o wsreg_pkgrm.o ../../version/version.o

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(PKGLIBS) $(LIBS) -o $@

install: all
	mkdir -p $(ROOT)$(SADMDIR)/install/bin
	$(INSTALL) -c -m 755 pkgremove $(ROOT)$(SADMDIR)/install/bin/pkgremove
	$(STRIP) $(ROOT)$(SADMDIR)/install/bin/pkgremove

clean:
	rm -f $(BIN) $(OBJ) core log

mrproper: clean

$(BIN): ../../libadm/libadm.a ../../libgendb/libgendb.a \
	../../libinst/libinst.a ../../libpkg/libpkg.a

check.o: check.c ../../hdrs/pkgstrct.h ../../hdrs/install.h \
  ../../libpkg/pkglib.h ../../hdrs/pkgdev.h ../../libpkg/pkgerr.h \
  ../../libpkg/keystore.h ../../libpkg/cfext.h ../../hdrs/libadm.h \
  ../../hdrs/sys/vtoc.h ../../hdrs/sys/dklabel.h ../../hdrs/pkginfo.h \
  ../../hdrs/valtools.h ../../hdrs/install.h ../../hdrs/libinst.h \
  ../../libpkg/cfext.h wsreg_pkgrm.h ../../hdrs/messages.h
delmap.o: delmap.c ../../hdrs/pkgstrct.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgdev.h ../../libpkg/pkgerr.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h ../../hdrs/libadm.h ../../hdrs/sys/vtoc.h \
  ../../hdrs/sys/dklabel.h ../../hdrs/pkginfo.h ../../hdrs/valtools.h \
  ../../hdrs/install.h ../../hdrs/libinst.h ../../libpkg/cfext.h
main.o: main.c ../../hdrs/pkgstrct.h ../../hdrs/pkginfo.h \
  ../../hdrs/pkglocs.h ../../libpkg/cfext.h \
  ../../libspmizones/spmizones_api.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgdev.h ../../libpkg/pkgerr.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h ../../hdrs/install.h ../../hdrs/libinst.h \
  ../../hdrs/install.h ../../hdrs/libadm.h ../../hdrs/sys/vtoc.h \
  ../../hdrs/sys/dklabel.h ../../hdrs/valtools.h ../../hdrs/messages.h
predepend.o: predepend.c ../../hdrs/pkglocs.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgdev.h ../../hdrs/pkgstrct.h ../../libpkg/pkgerr.h \
  ../../libpkg/keystore.h ../../libpkg/cfext.h ../../hdrs/libinst.h \
  ../../hdrs/pkginfo.h ../../libpkg/pkglib.h ../../libpkg/cfext.h \
  ../../hdrs/install.h ../../hdrs/libadm.h ../../hdrs/sys/vtoc.h \
  ../../hdrs/sys/dklabel.h ../../hdrs/valtools.h
quit.o: quit.c ../../libpkg/pkglib.h ../../hdrs/pkgdev.h \
  ../../hdrs/pkgstrct.h ../../libpkg/pkgerr.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h ../../hdrs/install.h ../../hdrs/libadm.h \
  ../../hdrs/sys/vtoc.h ../../hdrs/sys/dklabel.h ../../hdrs/pkginfo.h \
  ../../hdrs/valtools.h ../../hdrs/install.h ../../hdrs/libinst.h \
  ../../libpkg/cfext.h ../../hdrs/messages.h
special.o: special.c ../../hdrs/pkgstrct.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgdev.h ../../libpkg/pkgerr.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h
wsreg_pkgrm.o: wsreg_pkgrm.c wsreg_pkgrm.h
