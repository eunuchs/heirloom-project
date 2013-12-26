OBJ = m4.o m4ext.o m4macs.o m4y.o version.o
XOBJ = m4.o m4ext.o m4macs.o m4y_xpg4.o version_xpg4.o

.c.o: ; $(CC) -c $(CFLAGS) $(CPPFLAGS) $(WARN) $<

all: m4 m4_xpg4

m4: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o m4

m4_xpg4: $(XOBJ)
	$(CC) $(LDFLAGS) $(XOBJ) $(LIBS) -o m4_xpg4

version_xpg4.o: version.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -DXPG4 $(WARN) version.c -o $@

install: all
	test -d $(ROOT)$(BINDIR) || mkdir -p $(ROOT)$(BINDIR)
	$(INSTALL) -c -m 755 m4 $(ROOT)$(BINDIR)/m4
	$(STRIP) $(ROOT)$(BINDIR)/m4
	test -d $(ROOT)$(SUSBIN) || mkdir -p $(ROOT)$(SUSBIN)
	$(INSTALL) -c -m 755 m4_xpg4 $(ROOT)$(SUSBIN)/m4
	$(STRIP) $(ROOT)$(SUSBIN)/m4

clean:
	rm -f m4 $(OBJ) m4y.c m4_xpg4 $(XOBJ) m4y_xpg4.c core log *~

mrproper: clean

m4.o: m4.c m4.h
m4ext.o: m4ext.c m4.h
m4macs.o: m4macs.c m4.h
m4y.o: m4y.c m4.h
m4y_xpg4.o: m4y_xpg4.c m4.h
