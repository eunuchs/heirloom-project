.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(WARN) $<

all: version.o

install: all

clean:
	rm -f version.o core log

mrproper: clean
