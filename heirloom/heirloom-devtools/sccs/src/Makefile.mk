PATHS = -DBINDIR='"$(BINDIR)"' -DSUSBIN='"$(SUSBIN)"' -DLIBDIR='"$(LIBDIR)"'

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -I../hdr $(PATHS) $(WARN) $<

SCCSLIBS = -L../cassi -lcassi -L../comobj -lcomobj -L../mpwlib -lmpw

all: admin comb delta get help prs prt rmchg sccs sccsdiff unget val vc what

admin: admin.o version.o
	$(CC) $(LDFLAGS) admin.o version.o $(SCCSLIBS) $(LIBS) -o $@

comb: comb.o version.o
	$(CC) $(LDFLAGS) comb.o version.o $(SCCSLIBS) $(LIBS) -o $@

delta: delta.o version.o
	$(CC) $(LDFLAGS) delta.o version.o $(SCCSLIBS) $(LIBS) -o $@

get: get.o version.o
	$(CC) $(LDFLAGS) get.o version.o $(SCCSLIBS) $(LIBS) -o $@

help: help.o version.o
	$(CC) $(LDFLAGS) help.o version.o $(SCCSLIBS) $(LIBS) -o $@

prs: prs.o version.o
	$(CC) $(LDFLAGS) prs.o version.o $(SCCSLIBS) $(LIBS) -o $@

prt: prt.o version.o
	$(CC) $(LDFLAGS) prt.o version.o $(SCCSLIBS) $(LIBS) -o $@

rmchg: rmchg.o version.o
	$(CC) $(LDFLAGS) rmchg.o version.o $(SCCSLIBS) $(LIBS) -o $@

sccs: sccs.o version.o
	$(CC) $(LDFLAGS) sccs.o version.o $(SCCSLIBS) $(LIBS) -o $@

unget: unget.o version.o
	$(CC) $(LDFLAGS) unget.o version.o $(SCCSLIBS) $(LIBS) -o $@

val: val.o version.o
	$(CC) $(LDFLAGS) val.o version.o $(SCCSLIBS) $(LIBS) -o $@

vc: vc.o version.o
	$(CC) $(LDFLAGS) vc.o version.o $(SCCSLIBS) $(LIBS) -o $@

what: what.o version.o
	$(CC) $(LDFLAGS) what.o version.o $(SCCSLIBS) $(LIBS) -o $@

install: all
	mkdir -p $(ROOT)$(BINDIR)
	$(INSTALL) -c -m 755 admin $(ROOT)$(BINDIR)/admin
	$(STRIP) $(ROOT)$(BINDIR)/admin
	$(INSTALL) -c -m 755 comb $(ROOT)$(BINDIR)/comb
	$(STRIP) $(ROOT)$(BINDIR)/comb
	$(INSTALL) -c -m 755 delta $(ROOT)$(BINDIR)/delta
	$(STRIP) $(ROOT)$(BINDIR)/delta
	$(INSTALL) -c -m 755 get $(ROOT)$(BINDIR)/get
	$(STRIP) $(ROOT)$(BINDIR)/get
	$(INSTALL) -c -m 755 help $(ROOT)$(BINDIR)/help
	$(STRIP) $(ROOT)$(BINDIR)/help
	$(INSTALL) -c -m 755 prs $(ROOT)$(BINDIR)/prs
	$(STRIP) $(ROOT)$(BINDIR)/prs
	$(INSTALL) -c -m 755 prt $(ROOT)$(BINDIR)/prt
	$(STRIP) $(ROOT)$(BINDIR)/prt
	$(INSTALL) -c -m 755 rmchg $(ROOT)$(BINDIR)/rmdel
	$(STRIP) $(ROOT)$(BINDIR)/rmdel
	rm -f $(ROOT)$(BINDIR)/cdc
	cd $(ROOT)$(BINDIR) && $(LNS) rmdel cdc
	$(INSTALL) -c -m 755 sccs $(ROOT)$(BINDIR)/sccs
	$(STRIP) $(ROOT)$(BINDIR)/sccs
	$(INSTALL) -c -m 755 sccsdiff $(ROOT)$(BINDIR)/sccsdiff
	$(INSTALL) -c -m 755 unget $(ROOT)$(BINDIR)/unget
	$(STRIP) $(ROOT)$(BINDIR)/unget
	rm -f $(ROOT)$(BINDIR)/sact
	cd $(ROOT)$(BINDIR) && ln -s unget sact
	$(INSTALL) -c -m 755 val $(ROOT)$(BINDIR)/val
	$(STRIP) $(ROOT)$(BINDIR)/val
	$(INSTALL) -c -m 755 vc $(ROOT)$(BINDIR)/vc
	$(STRIP) $(ROOT)$(BINDIR)/vc
	$(INSTALL) -c -m 755 what $(ROOT)$(BINDIR)/what
	$(STRIP) $(ROOT)$(BINDIR)/what

clean:
	rm -f admin comb delta get help prs prt rmchg sccs sccsdiff \
		unget val vc what admin.o comb.o delta.o get.o help.o \
		prs.o prt.o rmchg.o sccs.o unget.o val.o vc.o what.o \
		version.o core log *~

mrproper: clean

admin.o: admin.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/had.h ../hdr/i18n.h
comb.o: comb.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/had.h ../hdr/i18n.h
delta.o: delta.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/had.h ../hdr/i18n.h ../hdr/ccstypes.h
get.o: get.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/had.h ../hdr/i18n.h ../hdr/ccstypes.h
help.o: help.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/i18n.h ../hdr/ccstypes.h
prs.o: prs.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/had.h ../hdr/i18n.h
prt.o: prt.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/had.h ../hdr/i18n.h
rmchg.o: rmchg.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/had.h ../hdr/filehand.h ../hdr/i18n.h
sccs.o: sccs.c ../hdr/i18n.h ../hdr/defines.h ../hdr/macros.h \
  ../hdr/fatal.h ../hdr/fatal.h
unget.o: unget.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/had.h ../hdr/i18n.h ../hdr/ccstypes.h
val.o: val.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/had.h ../hdr/i18n.h ../hdr/ccstypes.h
vc.o: vc.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h ../hdr/fatal.h
what.o: what.c ../hdr/defines.h ../hdr/macros.h ../hdr/fatal.h \
  ../hdr/fatal.h ../hdr/i18n.h
