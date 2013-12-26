SUBDIRS = yacc lex m4 \
	sccs/cassi sccs/comobj sccs/mpwlib sccs/src sccs/help.d sccs/man \
	make/bsd make/makestate make/mksdmsi18n make/mksh make/vroot make/src

MAKEFILES = $(SUBDIRS:=/Makefile)

.SUFFIXES: .mk
.mk:
	cat mk.config $< >$@

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

sun:
	/usr/xpg4/bin/make CXX=CC CFLAGS=-O CXXFLAGS=-O WARN=

PKGROOT = /var/tmp/heirloom-devtools
PKGTEMP = /var/tmp
PKGPROTO = pkgproto

heirloom-devtools.pkg: all
	rm -rf $(PKGROOT)
	mkdir -p $(PKGROOT)
	$(MAKE) ROOT=$(PKGROOT) install
	rm -f $(PKGPROTO)
	echo 'i pkginfo' >$(PKGPROTO)
	(cd $(PKGROOT) && find . -print | pkgproto) | >>$(PKGPROTO) sed 's:^\([df] [^ ]* [^ ]* [^ ]*\) .*:\1 root root:; s:^f\( [^ ]* etc/\):v \1:; s:^f\( [^ ]* var/\):v \1:; s:^\(s [^ ]* [^ ]*=\)\([^/]\):\1./\2:'
	rm -rf $(PKGTEMP)/$@
	pkgmk -a `uname -m` -d $(PKGTEMP) -r $(PKGROOT) -f $(PKGPROTO) $@
	pkgtrans -o -s $(PKGTEMP) `pwd`/$@ $@
	rm -rf $(PKGROOT) $(PKGPROTO) $(PKGTEMP)/$@
