SUBDIRS = \
	pkginfo pkgproto pkgmk pkgtrans \
	installf pkgadd pkgchk \
	pkgcond pkginstall pkgname \
	pkgparam pkgremove pkgrm pkgscripts

MAKEFILES = $(SUBDIRS:=/Makefile)

.SUFFIXES: .mk
.mk:
	cat ../mk.config $< >$@

dummy: $(MAKEFILES) all

makefiles: $(MAKEFILES)

.DEFAULT:
	+ for i in $(SUBDIRS); \
	do \
		(cd "$$i" && $(MAKE) $@) || exit; \
	done

mrproper: clean
	+ for i in $(SUBDIRS); \
	do \
		(cd "$$i" && $(MAKE) $@) || exit; \
	done
	rm -f $(MAKEFILES)
