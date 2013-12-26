INC = -I. -I../hdrs -I../libgendb

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INC) $(PATHS) $(WARN) $<


OBJ = canonize.o ckparam.o ckvolseq.o cvtpath.o dbsql.o devtype.o \
	dstream.o fmkdir.o gpkglist.o gpkgmap.o isdir.o keystore.o \
	logerr.o mappath.o ncgrpw.o nhash.o pkgerr.o \
	pkgexecl.o pkgexecv.o pkgmount.o pkgstr.o pkgtrans.o pkgweb.o \
	pkgxpand.o ppkgmap.o progerr.o putcfile.o rrmdir.o runcmd.o \
	security.o srchcfile.o tputcfent.o verify.o vfpops.o

all: libpkg.a

libpkg.a: $(OBJ)
	$(AR) -rv $@ $(OBJ)
	$(RANLIB) $@

install: all

clean:
	rm -f libpkg.a $(OBJ) core log *~

mrproper: clean

canonize.o: canonize.c
ckparam.o: ckparam.c ./pkglib.h ../hdrs/pkgdev.h ../hdrs/pkgstrct.h \
  ./pkgerr.h ./keystore.h ./cfext.h ./pkglibmsgs.h pkglocale.h
ckvolseq.o: ckvolseq.c ../hdrs/pkgstrct.h ./pkglib.h ../hdrs/pkgdev.h \
  ./pkgerr.h ./keystore.h ./cfext.h ./pkglibmsgs.h pkglocale.h
cvtpath.o: cvtpath.c
dbsql.o: dbsql.c ./pkglib.h ../hdrs/pkgdev.h ../hdrs/pkgstrct.h \
  ./pkgerr.h ./keystore.h ./cfext.h pkglocale.h ../libgendb/genericdb.h \
  dbtables.h dbsql.h cfext.h pkglibmsgs.h ../hdrs/libadm.h
devtype.o: devtype.c ../hdrs/pkgdev.h ./pkglib.h ../hdrs/pkgstrct.h \
  ./pkgerr.h ./keystore.h ./cfext.h
dstream.o: dstream.c ./pkglib.h ../hdrs/pkgdev.h ../hdrs/pkgstrct.h \
  ./pkgerr.h ./keystore.h ./cfext.h ./pkglibmsgs.h pkglocale.h \
  ../hdrs/libadm.h
fmkdir.o: fmkdir.c pkglib.h
gpkglist.o: gpkglist.c ../hdrs/valtools.h ../hdrs/pkginfo.h ./pkglib.h \
  ../hdrs/pkgdev.h ../hdrs/pkgstrct.h ./pkgerr.h ./keystore.h ./cfext.h \
  ./pkglibmsgs.h pkglocale.h
gpkgmap.o: gpkgmap.c ../hdrs/pkgstrct.h pkglib.h ../hdrs/pkgdev.h \
  pkgerr.h keystore.h cfext.h pkglibmsgs.h pkglocale.h ../hdrs/libadm.h
isdir.o: isdir.c ../hdrs/archives.h pkglocale.h pkglibmsgs.h
keystore.o: keystore.c p12lib.h pkgerr.h keystore.h pkglib.h \
  ../hdrs/pkgdev.h ../hdrs/pkgstrct.h cfext.h pkglibmsgs.h
logerr.o: logerr.c pkglocale.h
mappath.o: mappath.c
ncgrpw.o: ncgrpw.c ./pkglib.h ../hdrs/pkgdev.h ../hdrs/pkgstrct.h \
  ./pkgerr.h ./keystore.h ./cfext.h pkglocale.h nhash.h
nhash.o: nhash.c ./pkglib.h ../hdrs/pkgdev.h ../hdrs/pkgstrct.h \
  ./pkgerr.h ./keystore.h ./cfext.h nhash.h pkglocale.h
p12lib.o: p12lib.c p12lib.h
pkgerr.o: pkgerr.c pkgerr.h
pkgexecl.o: pkgexecl.c pkglocale.h pkglibmsgs.h ./pkglib.h \
  ../hdrs/pkgdev.h ../hdrs/pkgstrct.h ./pkgerr.h ./keystore.h ./cfext.h
pkgexecv.o: pkgexecv.c ./pkglib.h ../hdrs/pkgdev.h ../hdrs/pkgstrct.h \
  ./pkgerr.h ./keystore.h ./cfext.h ./pkglibmsgs.h pkglocale.h ../hdrs/libadm.h
pkgmount.o: pkgmount.c ../hdrs/pkgdev.h ../hdrs/pkginfo.h \
  ../hdrs/devmgmt.h ./pkglib.h ../hdrs/pkgstrct.h ./pkgerr.h ./keystore.h \
  ./cfext.h ./pkglibmsgs.h pkglocale.h
pkgstr.o: pkgstr.c ./pkglib.h ../hdrs/pkgdev.h ../hdrs/pkgstrct.h \
  ./pkgerr.h ./keystore.h ./cfext.h pkglocale.h
pkgtrans.o: pkgtrans.c ../hdrs/pkginfo.h ../hdrs/pkgstrct.h \
  ../hdrs/pkgtrans.h ../hdrs/pkgdev.h ../hdrs/devmgmt.h ./pkglib.h \
  ./pkgerr.h ./keystore.h ./cfext.h ./pkglibmsgs.h ./keystore.h \
  pkglocale.h pkgerr.h ../hdrs/libadm.h
pkgweb.o: pkgweb.c ../hdrs/libadm.h
pkgxpand.o: pkgxpand.c ./pkglib.h ../hdrs/pkgdev.h ../hdrs/pkgstrct.h \
  ./pkgerr.h ./keystore.h ./cfext.h pkglocale.h
ppkgmap.o: ppkgmap.c ../hdrs/pkgstrct.h
progerr.o: progerr.c pkglocale.h pkgerr.h
putcfile.o: putcfile.c ../hdrs/pkgstrct.h pkglib.h ../hdrs/pkgdev.h \
  pkgerr.h keystore.h cfext.h ../hdrs/libadm.h
rrmdir.o: rrmdir.c pkglocale.h pkglib.h ../hdrs/pkgdev.h \
  ../hdrs/pkgstrct.h pkgerr.h keystore.h cfext.h
runcmd.o: runcmd.c ./pkglib.h ../hdrs/pkgdev.h ../hdrs/pkgstrct.h \
  ./pkgerr.h ./keystore.h ./cfext.h pkglocale.h pkglibmsgs.h ../hdrs/libadm.h
security.o: security.c ../hdrs/pkgstrct.h ../hdrs/pkginfo.h pkgerr.h \
  pkglib.h ../hdrs/pkgdev.h keystore.h cfext.h pkglibmsgs.h pkglocale.h \
  p12lib.h
srchcfile.o: srchcfile.c ./pkglib.h ../hdrs/pkgdev.h ../hdrs/pkgstrct.h \
  ./pkgerr.h ./keystore.h ./cfext.h pkglocale.h pkglibmsgs.h
tputcfent.o: tputcfent.c ../hdrs/pkgstrct.h pkglocale.h
verify.o: verify.c ../hdrs/pkgstrct.h ./pkglib.h ../hdrs/pkgdev.h \
  ./pkgerr.h ./keystore.h ./cfext.h ./pkglibmsgs.h pkglocale.h
vfpops.o: vfpops.c ./pkglib.h ../hdrs/pkgdev.h ../hdrs/pkgstrct.h \
  ./pkgerr.h ./keystore.h ./cfext.h pkglocale.h
