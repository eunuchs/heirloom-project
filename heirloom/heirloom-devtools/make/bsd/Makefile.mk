OBJ = bsd.o

.c.o:
	$(CC) $(CFLAGS) $(WARN) $(CPPFLAGS) -I../include -c $<

.cc.o:
	$(CXX) $(CXXFLAGS) $(WARN) $(CPPFLAGS) -I../include -c $<

all: libbsd.a

libbsd.a: $(OBJ)
	$(AR) -rv $@ $(OBJ)
	$(RANLIB) $@

install: all

clean:
	rm -f libbsd.a $(OBJ) core log *~

mrproper: clean

bsd.o: bsd.cc ../include/bsd/bsd.h
