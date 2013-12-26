OBJ = ld_file.o lock.o

.c.o:
	$(CC) $(CFLAGS) $(WARN) $(CPPFLAGS) -I../include -c $<

.cc.o:
	$(CXX) $(CXXFLAGS) $(WARN) $(CPPFLAGS) -I../include -c $<

all: libmakestate.a

libmakestate.a: $(OBJ)
	$(AR) -rv $@ $(OBJ)
	$(RANLIB) $@

install: all

clean:
	rm -f libmakestate.a $(OBJ) core log *~

mrproper: clean

ld_file.o: ld_file.c
lock.o: lock.c
