all:

install: all
	test -d $(ROOT)$(MANDIR)/man1 || mkdir -p $(ROOT)$(MANDIR)/man1
	$(INSTALL) -c -m 644 admin.1 $(ROOT)$(MANDIR)/man1/admin.1
	$(INSTALL) -c -m 644 cdc.1 $(ROOT)$(MANDIR)/man1/cdc.1
	$(INSTALL) -c -m 644 comb.1 $(ROOT)$(MANDIR)/man1/comb.1
	$(INSTALL) -c -m 644 delta.1 $(ROOT)$(MANDIR)/man1/delta.1
	$(INSTALL) -c -m 644 get.1 $(ROOT)$(MANDIR)/man1/get.1
	$(INSTALL) -c -m 644 help.1 $(ROOT)$(MANDIR)/man1/help.1
	$(INSTALL) -c -m 644 prs.1 $(ROOT)$(MANDIR)/man1/prs.1
	$(INSTALL) -c -m 644 rmdel.1 $(ROOT)$(MANDIR)/man1/rmdel.1
	$(INSTALL) -c -m 644 sact.1 $(ROOT)$(MANDIR)/man1/sact.1
	$(INSTALL) -c -m 644 sccsdiff.1 $(ROOT)$(MANDIR)/man1/sccsdiff.1
	$(INSTALL) -c -m 644 unget.1 $(ROOT)$(MANDIR)/man1/unget.1
	$(INSTALL) -c -m 644 val.1 $(ROOT)$(MANDIR)/man1/val.1
	$(INSTALL) -c -m 644 vc.1 $(ROOT)$(MANDIR)/man1/vc.1
	$(INSTALL) -c -m 644 what.1 $(ROOT)$(MANDIR)/man1/what.1
	test -d $(ROOT)$(MANDIR)/man1b || mkdir -p $(ROOT)$(MANDIR)/man1b
	$(INSTALL) -c -m 644 prt.1b $(ROOT)$(MANDIR)/man1b/prt.1b
	$(INSTALL) -c -m 644 sccs.1b $(ROOT)$(MANDIR)/man1b/sccs.1b
	test -d $(ROOT)$(MANDIR)/man5 || mkdir -p $(ROOT)$(MANDIR)/man5
	$(INSTALL) -c -m 644 sccsfile.5 $(ROOT)$(MANDIR)/man5/sccsfile.5

clean:

mrproper: clean
