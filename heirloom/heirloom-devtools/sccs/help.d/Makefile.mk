DATA = ad bd bu cb cm cmds co de default ge he prs rc un ut vc

all:

install: all
	mkdir -p $(ROOT)$(LIBDIR)/help
	for i in $(DATA); \
	do \
		$(INSTALL) -c -m 644 $$i $(ROOT)$(LIBDIR)/help/$$i || exit; \
	done

clean:

mrproper: clean
