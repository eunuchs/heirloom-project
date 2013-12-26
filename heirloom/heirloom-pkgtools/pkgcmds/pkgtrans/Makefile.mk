INC = -I../../hdrs -I../../libgendb -I../../libpkg -I../../libinst \
	-I../../libspmizones -I../../libadm

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INC) $(PATHS) $(WARN) $<

PKGLIBS = -L../../libinst -linst -L../../libpkg -lpkg -L../../libgendb -lgendb \
	-L../../libadm -ladm \
	-L$(CCSDIR)/lib -ll

BIN = pkgtrans
OBJ = main.o ../../version/version.o

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(PKGLIBS) $(LIBS) -o $@

install: all
	mkdir -p $(ROOT)$(BINDIR)
	$(INSTALL) -c -m 755 pkgtrans $(ROOT)$(BINDIR)/pkgtrans
	$(STRIP) $(ROOT)$(BINDIR)/pkgtrans

clean:
	rm -f $(BIN) $(OBJ) core log

mrproper: clean

$(BIN): ../../libadm/libadm.a ../../libgendb/libgendb.a \
	../../libinst/libinst.a ../../libpkg/libpkg.a

main.o: main.c ../../hdrs/pkgtrans.h ../../libpkg/pkglib.h \
  ../../hdrs/pkgdev.h ../../hdrs/pkgstrct.h ../../libpkg/pkgerr.h \
  ../../libpkg/keystore.h ../../libpkg/cfext.h ../../libpkg/pkglibmsgs.h \
  ../../hdrs/pkglocs.h ../../hdrs/libadm.h ../../hdrs/sys/vtoc.h \
  ../../hdrs/sys/dklabel.h ../../hdrs/pkginfo.h ../../hdrs/valtools.h \
  ../../hdrs/install.h ../../hdrs/libinst.h ../../libpkg/cfext.h
