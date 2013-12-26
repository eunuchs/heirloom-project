INC = -I../../hdrs -I../../libgendb -I../../libpkg -I../../libinst \
	-I../../libspmizones -I../../libadm

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INC) $(PATHS) $(WARN) $<

PKGLIBS = -L../../libinst -linst -L../../libpkg -lpkg -L../../libgendb -lgendb \
	-L../../libadm -ladm \
	-L$(CCSDIR)/lib -ll

BIN = pkgrm
OBJ = check.o main.o presvr4.o quit.o ../../version/version.o

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(PKGLIBS) $(LIBS) -o $@

install: all
	mkdir -p $(ROOT)$(SBINDIR)
	$(INSTALL) -c -m 755 pkgrm $(ROOT)$(SBINDIR)/pkgrm
	$(STRIP) $(ROOT)$(SBINDIR)/pkgrm

clean:
	rm -f $(BIN) $(OBJ) core log

mrproper: clean

$(BIN): ../../libadm/libadm.a ../../libgendb/libgendb.a \
	../../libinst/libinst.a ../../libpkg/libpkg.a

check.o: check.c ../../hdrs/pkgstrct.h ../../hdrs/pkglocs.h \
  ../../libpkg/pkglib.h ../../hdrs/pkgdev.h ../../libpkg/pkgerr.h \
  ../../libpkg/keystore.h ../../libpkg/cfext.h ../../hdrs/install.h \
  ../../hdrs/libinst.h ../../hdrs/pkginfo.h ../../libpkg/cfext.h \
  ../../hdrs/install.h ../../hdrs/libadm.h ../../hdrs/sys/vtoc.h \
  ../../hdrs/sys/dklabel.h ../../hdrs/valtools.h ../../hdrs/messages.h \
  ../../libspmizones/spmizones_api.h
main.o: main.c ../../hdrs/pkgstrct.h ../../hdrs/pkgdev.h \
  ../../hdrs/pkginfo.h ../../hdrs/pkglocs.h ../../libpkg/pkglib.h \
  ../../libpkg/pkgerr.h ../../libpkg/keystore.h ../../libpkg/cfext.h \
  ../../libspmizones/spmizones_api.h ../../hdrs/install.h \
  ../../hdrs/libinst.h ../../libpkg/cfext.h ../../hdrs/install.h \
  ../../hdrs/libadm.h ../../hdrs/sys/vtoc.h ../../hdrs/sys/dklabel.h \
  ../../hdrs/valtools.h ../../hdrs/messages.h quit.h
presvr4.o: presvr4.c ../../hdrs/valtools.h ../../hdrs/pkgdev.h \
  ../../hdrs/pkglocs.h ../../hdrs/install.h ../../hdrs/pkgstrct.h \
  ../../libpkg/pkglib.h ../../libpkg/pkgerr.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h ../../hdrs/libadm.h ../../hdrs/sys/vtoc.h \
  ../../hdrs/sys/dklabel.h ../../hdrs/pkginfo.h ../../hdrs/install.h \
  ../../hdrs/libinst.h ../../libpkg/cfext.h quit.h
quit.o: quit.c ../../hdrs/pkgdev.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgstrct.h ../../libpkg/pkgerr.h ../../libpkg/keystore.h \
  ../../libpkg/cfext.h ../../libspmizones/spmizones_api.h \
  ../../hdrs/libadm.h ../../hdrs/sys/vtoc.h ../../hdrs/sys/dklabel.h \
  ../../hdrs/pkginfo.h ../../hdrs/valtools.h ../../hdrs/install.h \
  ../../hdrs/libinst.h ../../libpkg/cfext.h ../../hdrs/messages.h quit.h
