OBJ = libmksdmsi18n_init.o

.c.o:
	$(CC) $(CFLAGS) $(WARN) $(CPPFLAGS) -I../include -c $<

.cc.o:
	$(CXX) $(CXXFLAGS) $(WARN) $(CPPFLAGS) -I../include -c $<

all: libmksdmsi18n.a

libmksdmsi18n.a: $(OBJ)
	$(AR) -rv $@ $(OBJ)
	$(RANLIB) $@

install: all

clean:
	rm -f libmksdmsi18n.a $(OBJ) core log *~

mrproper: clean

libmksdmsi18n_init.o: libmksdmsi18n_init.cc ../include/avo/intl.h
