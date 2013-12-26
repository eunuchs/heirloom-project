INC = -I../../hdrs -I../../libgendb -I../../libpkg -I../../libinst \
	-I../../libspmizones -I../../libadm

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INC) $(PATHS) $(WARN) $<

PKGLIBS = -L../../libinst -linst -L../../libpkg -lpkg -L../../libgendb -lgendb \
	-L../../libadm -ladm \
	-L$(CCSDIR)/lib -ll

BIN = pkgcond
OBJ = main.o ../../version/version.o

all:

install:

_all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(PKGLIBS) $(LIBS) -o $@

_install: all
	mkdir -p $(ROOT)$(BINDIR)
	$(INSTALL) -c -m 755 pkgcond $(ROOT)$(BINDIR)/pkgcond
	$(STRIP) $(ROOT)$(BINDIR)/pkgcond

clean:
	rm -f $(BIN) $(OBJ) core log

mrproper: clean

$(BIN): ../../libadm/libadm.a ../../libgendb/libgendb.a \
	../../libinst/libinst.a ../../libpkg/libpkg.a

main.o: main.c ../../hdrs/sys/mnttab.h ../../hdrs/sys/mntent.h \
  ../../libspmizones/spmizones_api.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgdev.h ../../hdrs/pkgstrct.h ../../libpkg/pkgerr.h \
  ../../libpkg/keystore.h ../../libpkg/cfext.h ../../hdrs/install.h \
  ../../hdrs/libinst.h ../../hdrs/pkginfo.h ../../libpkg/cfext.h \
  ../../hdrs/install.h ../../hdrs/libadm.h ../../hdrs/sys/vtoc.h \
  ../../hdrs/sys/dklabel.h ../../hdrs/valtools.h ../../hdrs/messages.h \
  pkgcond.h pkgcond_msgs.h
