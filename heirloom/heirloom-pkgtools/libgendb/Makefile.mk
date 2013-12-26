.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -I../libpkgdb $(PATHS) $(WARN) $<


OBJ = genericdb.o

all: libgendb.a

libgendb.a: $(OBJ)
	$(AR) -rv $@ $(OBJ)
	$(RANLIB) $@

install: all

clean:
	rm -f libgendb.a $(OBJ) core log *~

mrproper: clean

genericdb.o: genericdb.c genericdb.h genericdb_private.h
