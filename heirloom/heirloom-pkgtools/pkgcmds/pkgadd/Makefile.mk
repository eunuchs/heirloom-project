INC = -I../../hdrs -I../../libgendb -I../../libpkg -I../../libinst \
	-I../../libspmizones -I../../libadm

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INC) $(PATHS) $(WARN) $<

PKGLIBS = -L../../libinst -linst -L../../libpkg -lpkg -L../../libgendb -lgendb \
	-L../../libadm -ladm \
	-L$(CCSDIR)/lib -ll

BIN = pkgadd
OBJ = check.o main.o presvr4.o quit.o ../../version/version.o

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(PKGLIBS) $(LIBS) -o $@

$(BIN): ../../libadm/libadm.a ../../libgendb/libgendb.a \
	../../libinst/libinst.a ../../libpkg/libpkg.a

install: all
	mkdir -p $(ROOT)$(SBINDIR)
	$(INSTALL) -c -m 755 pkgadd $(ROOT)$(SBINDIR)/pkgadd
	$(STRIP) $(ROOT)$(SBINDIR)/pkgadd
	rm -f $(ROOT)$(SBINDIR)/pkgask
	ln -s pkgadd $(ROOT)$(SBINDIR)/pkgask

clean:
	rm -f $(BIN) $(OBJ) core log

mrproper: clean

check.o: check.c ../../hdrs/pkgstrct.h ../../hdrs/pkglocs.h \
  ../../libspmizones/spmizones_api.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgdev.h ../../libpkg/pkgerr.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h ../../hdrs/install.h ../../hdrs/libinst.h \
  ../../hdrs/pkginfo.h ../../libpkg/cfext.h ../../hdrs/install.h \
  ../../hdrs/libadm.h ../../hdrs/sys/vtoc.h ../../hdrs/sys/dklabel.h \
  ../../hdrs/valtools.h ../../hdrs/messages.h
main.o: main.c ../../hdrs/pkgdev.h ../../hdrs/pkginfo.h \
  ../../hdrs/pkglocs.h ../../hdrs/pkgtrans.h \
  ../../libspmizones/spmizones_api.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgstrct.h ../../libpkg/pkgerr.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h ../../libpkg/pkgerr.h ../../libpkg/pkgweb.h \
  ../../hdrs/install.h ../../hdrs/libinst.h ../../libpkg/cfext.h \
  ../../hdrs/install.h ../../hdrs/libadm.h ../../hdrs/sys/vtoc.h \
  ../../hdrs/sys/dklabel.h ../../hdrs/valtools.h ../../hdrs/messages.h \
  quit.h
presvr4.o: presvr4.c ../../hdrs/pkginfo.h ../../hdrs/pkgstrct.h \
  ../../hdrs/pkgdev.h ../../hdrs/pkglocs.h ../../libpkg/pkglib.h \
  ../../libpkg/pkgerr.h ../../libpkg/keystore.h ../../libpkg/cfext.h \
  ../../hdrs/install.h ../../hdrs/libinst.h ../../libpkg/cfext.h \
  ../../hdrs/install.h ../../hdrs/libadm.h ../../hdrs/sys/vtoc.h \
  ../../hdrs/sys/dklabel.h ../../hdrs/valtools.h ../../hdrs/messages.h \
  quit.h ../../libspmizones/spmizones_api.h
quit.o: quit.c ../../hdrs/pkgdev.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgstrct.h ../../libpkg/pkgerr.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h ../../libpkg/pkgweb.h ../../hdrs/libadm.h \
  ../../hdrs/sys/vtoc.h ../../hdrs/sys/dklabel.h ../../hdrs/pkginfo.h \
  ../../hdrs/valtools.h ../../hdrs/install.h ../../hdrs/libinst.h \
  ../../libpkg/cfext.h ../../hdrs/messages.h quit.h \
  ../../libspmizones/spmizones_api.h
