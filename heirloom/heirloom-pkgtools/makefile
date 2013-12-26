SUBDIRS = version libadm libgendb libpkg libinst \
	pkgcmds/pkginfo pkgcmds/pkgproto pkgcmds/pkgmk pkgcmds/pkgtrans \
	pkgcmds/installf pkgcmds/pkgadd pkgcmds/pkgchk \
	pkgcmds/pkginstall pkgcmds/pkgname \
	pkgcmds/pkgparam pkgcmds/pkgremove pkgcmds/pkgrm pkgcmds/pkgscripts \
	valtools man

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

PKGROOT = /var/tmp/heirloom-pkgtools
PKGTEMP = /var/tmp
PKGPROTO = pkgproto

heirloom-pkgtools.pkg: all
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
