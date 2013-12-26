INC = -I../../hdrs -I../../libgendb -I../../libpkg -I../../libinst \
	-I../../libspmizones -I../../libadm

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INC) $(PATHS) $(WARN) $<

PKGLIBS = -L../../libinst -linst -L../../libpkg -lpkg -L../../libgendb -lgendb \
	-L../../libadm -ladm \

BIN = pkginfo
OBJ = pkginfo.o ../../version/version.o

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(PKGLIBS) $(LIBS) -o $@

install: all
	mkdir -p $(ROOT)$(BINDIR)
	$(INSTALL) -c -m 755 pkginfo $(ROOT)$(BINDIR)/pkginfo
	$(STRIP) $(ROOT)$(BINDIR)/pkginfo
	mkdir -p $(ROOT)$(VSADMDIR)/pkg

clean:
	rm -f $(BIN) $(OBJ) core log

mrproper: clean

$(BIN): ../../libadm/libadm.a ../../libgendb/libgendb.a \
	../../libinst/libinst.a ../../libpkg/libpkg.a

pkginfo.o: pkginfo.c ../../hdrs/pkginfo.h ../../hdrs/pkgstrct.h \
  ../../hdrs/pkglocs.h ../../libpkg/pkglib.h ../../hdrs/pkgdev.h \
  ../../libpkg/pkgerr.h ../../libpkg/keystore.h ../../libpkg/cfext.h \
  ../../libpkg/dbtables.h ../../hdrs/libadm.h ../../hdrs/sys/vtoc.h \
  ../../hdrs/sys/dklabel.h ../../hdrs/valtools.h ../../hdrs/install.h \
  ../../hdrs/libinst.h ../../libpkg/cfext.h \
  ../../libspmizones/spmizones_api.h
