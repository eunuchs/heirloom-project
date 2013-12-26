OBJ = access.o args.o chdir.o chmod.o chown.o chroot.o creat.o \
	execve.o lock.o lstat.o mkdir.o mount.o open.o readlink.o \
	report.o rmdir.o setenv.o stat.o statfs.o truncate.o \
	unlink.o unmount.o utimes.o vroot.o

.c.o:
	$(CC) $(CFLAGS) $(WARN) $(CPPFLAGS) -I../include -c $<

.cc.o:
	$(CXX) $(CXXFLAGS) $(WARN) $(CPPFLAGS) -I../include -c $<

all: libvroot.a

libvroot.a: $(OBJ)
	$(AR) -rv $@ $(OBJ)
	$(RANLIB) $@

install: all

clean:
	rm -f libvroot.a $(OBJ) core log *~

mrproper: clean

access.o: access.cc ../include/vroot/vroot.h ../include/vroot/args.h
args.o: args.cc ../include/vroot/vroot.h ../include/vroot/args.h
chdir.o: chdir.cc ../include/vroot/vroot.h ../include/vroot/args.h
chmod.o: chmod.cc ../include/vroot/vroot.h ../include/vroot/args.h
chown.o: chown.cc ../include/vroot/vroot.h ../include/vroot/args.h
chroot.o: chroot.cc ../include/vroot/vroot.h ../include/vroot/args.h
creat.o: creat.cc ../include/vroot/vroot.h ../include/vroot/args.h
execve.o: execve.cc ../include/vroot/vroot.h ../include/vroot/args.h
lock.o: lock.cc ../include/avo/intl.h ../include/vroot/vroot.h \
  ../include/mksdmsi18n/mksdmsi18n.h
lstat.o: lstat.cc ../include/vroot/vroot.h ../include/vroot/args.h
mkdir.o: mkdir.cc ../include/vroot/vroot.h ../include/vroot/args.h
mount.o: mount.cc ../include/vroot/vroot.h ../include/vroot/args.h
open.o: open.cc ../include/vroot/vroot.h ../include/vroot/args.h
readlink.o: readlink.cc ../include/vroot/vroot.h ../include/vroot/args.h
report.o: report.cc ../include/vroot/report.h ../include/vroot/vroot.h \
  ../include/mksdmsi18n/mksdmsi18n.h ../include/avo/intl.h \
  ../include/mk/defs.h ../include/mksh/defs.h
rmdir.o: rmdir.cc ../include/vroot/vroot.h ../include/vroot/args.h
setenv.o: setenv.cc
stat.o: stat.cc ../include/vroot/vroot.h ../include/vroot/args.h
statfs.o: statfs.cc ../include/vroot/vroot.h ../include/vroot/args.h
truncate.o: truncate.cc ../include/vroot/vroot.h ../include/vroot/args.h
unlink.o: unlink.cc ../include/vroot/vroot.h ../include/vroot/args.h
unmount.o: unmount.cc ../include/vroot/vroot.h ../include/vroot/args.h
utimes.o: utimes.cc ../include/vroot/vroot.h ../include/vroot/args.h
vroot.o: vroot.cc ../include/vroot/vroot.h ../include/vroot/args.h \
  ../include/avo/intl.h
