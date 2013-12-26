INC = -I../../hdrs -I../../libgendb -I../../libpkg -I../../libinst \
	-I../../libspmizones -I../../libadm

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INC) $(PATHS) $(WARN) $<

PKGLIBS = -L../../libinst -linst -L../../libpkg -lpkg -L../../libgendb -lgendb \
	-L../../libadm -ladm \
	-L$(CCSDIR)/lib -ll

BIN = pkgadm
OBJ = addcert.o certs.o listcert.o lock.o main.o pkgadm_contents.o \
	pkgadm_special.o removecert.o ../../version/version.o

all:

install:

_all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(PKGLIBS) $(LIBS) -o $@

_install: all
	mkdir -p $(ROOT)$(BINDIR)
	$(INSTALL) -c -m 755 pkgadm $(ROOT)$(BINDIR)/pkgadm
	$(STRIP) $(ROOT)$(BINDIR)/pkgadm

clean:
	rm -f $(BIN) $(OBJ) core log

mrproper: clean

$(BIN): ../../libadm/libadm.a ../../libgendb/libgendb.a \
	../../libinst/libinst.a ../../libpkg/libpkg.a

addcert.o: addcert.c pkgadm.h ../../libpkg/pkgerr.h \
  ../../libpkg/keystore.h ../../libpkg/pkgerr.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgdev.h ../../hdrs/pkgstrct.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h ../../libgendb/genericdb.h ../../hdrs/libinst.h \
  ../../hdrs/pkginfo.h ../../libpkg/pkglib.h ../../libpkg/cfext.h \
  ../../hdrs/install.h pkgadm_msgs.h
certs.o: certs.c ../../hdrs/pkglocs.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgdev.h ../../hdrs/pkgstrct.h ../../libpkg/pkgerr.h \
  ../../libpkg/keystore.h ../../libpkg/cfext.h ../../libpkg/p12lib.h \
  pkgadm.h ../../libpkg/pkgerr.h ../../libpkg/keystore.h \
  ../../libgendb/genericdb.h ../../hdrs/libinst.h ../../hdrs/pkginfo.h \
  ../../libpkg/pkglib.h ../../libpkg/cfext.h ../../hdrs/install.h \
  pkgadm_msgs.h ../../hdrs/install.h ../../hdrs/libadm.h \
  ../../hdrs/sys/vtoc.h ../../hdrs/sys/dklabel.h ../../hdrs/valtools.h
listcert.o: listcert.c pkgadm.h ../../libpkg/pkgerr.h \
  ../../libpkg/keystore.h ../../libpkg/pkgerr.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgdev.h ../../hdrs/pkgstrct.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h ../../libgendb/genericdb.h ../../hdrs/libinst.h \
  ../../hdrs/pkginfo.h ../../libpkg/pkglib.h ../../libpkg/cfext.h \
  ../../hdrs/install.h pkgadm_msgs.h
lock.o: lock.c ../../libspmizones/spmizones_api.h pkgadm.h \
  ../../libpkg/pkgerr.h ../../libpkg/keystore.h ../../libpkg/pkgerr.h \
  ../../libpkg/pkglib.h ../../hdrs/pkgdev.h ../../hdrs/pkgstrct.h \
  ../../libpkg/keystore.h ../../libpkg/cfext.h ../../libgendb/genericdb.h \
  ../../hdrs/libinst.h ../../hdrs/pkginfo.h ../../libpkg/pkglib.h \
  ../../libpkg/cfext.h ../../hdrs/install.h pkgadm_msgs.h
main.o: main.c pkgadm.h ../../libpkg/pkgerr.h ../../libpkg/keystore.h \
  ../../libpkg/pkgerr.h ../../libpkg/pkglib.h ../../hdrs/pkgdev.h \
  ../../hdrs/pkgstrct.h ../../libpkg/keystore.h ../../libpkg/cfext.h \
  ../../libgendb/genericdb.h ../../hdrs/libinst.h ../../hdrs/pkginfo.h \
  ../../libpkg/pkglib.h ../../libpkg/cfext.h ../../hdrs/install.h \
  pkgadm_msgs.h
pkgadm_contents.o: pkgadm_contents.c ../../hdrs/pkgstrct.h \
  ../../libpkg/pkglib.h ../../hdrs/pkgdev.h ../../libpkg/pkgerr.h \
  ../../libpkg/keystore.h ../../libpkg/cfext.h pkgadm.h \
  ../../libpkg/pkgerr.h ../../libpkg/keystore.h \
  ../../libgendb/genericdb.h ../../hdrs/libinst.h ../../hdrs/pkginfo.h \
  ../../libpkg/pkglib.h ../../libpkg/cfext.h ../../hdrs/install.h \
  pkgadm_msgs.h ../../libpkg/dbsql.h ../../libgendb/genericdb.h
pkgadm_special.o: pkgadm_special.c ../../hdrs/pkgstrct.h pkgadm.h \
  ../../libpkg/pkgerr.h ../../libpkg/keystore.h ../../libpkg/pkgerr.h \
  ../../libpkg/pkglib.h ../../hdrs/pkgdev.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h ../../libgendb/genericdb.h ../../hdrs/libinst.h \
  ../../hdrs/pkginfo.h ../../libpkg/pkglib.h ../../libpkg/cfext.h \
  ../../hdrs/install.h pkgadm_msgs.h ../../libpkg/dbsql.h \
  ../../libgendb/genericdb.h
removecert.o: removecert.c pkgadm.h ../../libpkg/pkgerr.h \
  ../../libpkg/keystore.h ../../libpkg/pkgerr.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgdev.h ../../hdrs/pkgstrct.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h ../../libgendb/genericdb.h ../../hdrs/libinst.h \
  ../../hdrs/pkginfo.h ../../libpkg/pkglib.h ../../libpkg/cfext.h \
  ../../hdrs/install.h pkgadm_msgs.h
