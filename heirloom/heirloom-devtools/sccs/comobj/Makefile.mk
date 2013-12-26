.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -I../hdr $(WARN) $<


OBJ = auxf.o chkid.o chksid.o cmpdate.o date_ab.o date_ba.o \
	del_ab.o del_ba.o dodelt.o dofile.o dohist.o doie.o \
	dolist.o encode.o eqsid.o flushto.o fmterr.o getline.o \
	getser.o logname.o newsid.o newstats.o permiss.o pf_ab.o \
	putline.o rdmod.o setup.o sid_ab.o sid_ba.o sidtoser.o \
	sinit.o stats_ab.o strptim.o

all: libcomobj.a

libcomobj.a: $(OBJ)
	$(AR) -rv $@ $(OBJ)
	$(RANLIB) $@

install: all

clean:
	rm -f libcomobj.a $(OBJ) core log *~

mrproper: clean

auxf.o: auxf.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
chkid.o: chkid.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
chksid.o: chksid.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
cmpdate.o: cmpdate.c
date_ab.o: date_ab.c ../hdr/macros.h ../hdr/defines.h
date_ba.o: date_ba.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
del_ab.o: del_ab.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
del_ba.o: del_ba.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
dodelt.o: dodelt.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
dofile.o: dofile.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
dohist.o: dohist.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/had.h ../hdr/i18n.h
doie.o: doie.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
dolist.o: dolist.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
encode.o: encode.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
eqsid.o: eqsid.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
flushto.o: flushto.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
fmterr.o: fmterr.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
getline.o: getline.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
getser.o: getser.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/had.h
logname.o: logname.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
newsid.o: newsid.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
newstats.o: newstats.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
permiss.o: permiss.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/i18n.h
pf_ab.o: pf_ab.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
putline.o: putline.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/i18n.h
rdmod.o: rdmod.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
setup.o: setup.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
sid_ab.o: sid_ab.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
sid_ba.o: sid_ba.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
sidtoser.o: sidtoser.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
sinit.o: sinit.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
stats_ab.o: stats_ab.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
strptim.o: strptim.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h
