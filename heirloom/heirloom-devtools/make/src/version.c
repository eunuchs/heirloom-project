#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define USED    __attribute__ ((used))
#elif defined __GNUC__
#define USED    __attribute__ ((unused))
#else
#define USED
#endif
static const char id[] USED = "@(#)make.sl	1.44 (gritter) 8/7/10";
/* SLIST */
/*
../bsd/bsd.cc: * Sccsid @(#)bsd.cc	1.6 (gritter) 01/22/07
../src/read.cc: * Sccsid @(#)read.cc	1.15 (gritter) 3/13/07
../src/dist.cc: * Sccsid @(#)dist.cc	1.5 (gritter) 01/20/07
../src/ar.cc: * Sccsid @(#)ar.cc	1.5 (gritter) 01/13/07
../src/dosys.cc: * Sccsid @(#)dosys.cc	1.4 (gritter) 01/13/07
../src/make.cc: * Sccsid @(#)make.cc	1.2 (gritter) 01/07/07
../src/depvar.cc: * Sccsid @(#)depvar.cc	1.3 (gritter) 01/13/07
../src/implicit.cc: * Sccsid @(#)implicit.cc	1.5 (gritter) 2/16/07
../src/files.cc: * Sccsid @(#)files.cc	1.7 (gritter) 01/23/07
../src/posix.cc: * Sccsid @(#)posix.cc	1.1 (gritter) 01/13/07
../src/parallel.cc: * Sccsid @(#)parallel.cc	1.7 (gritter) 3/7/07
../src/getopt.c: * Sccsid @(#)getopt.c	1.10 (gritter) 12/16/07
../src/nse.cc: * Sccsid @(#)nse.cc	1.2 (gritter) 01/07/07
../src/dmake.cc: * Sccsid @(#)dmake.cc	1.2 (gritter) 01/07/07
../src/globals.cc: * Sccsid @(#)globals.cc	1.8 (gritter) 3/6/07
../src/rep.cc: * Sccsid @(#)rep.cc	1.3 (gritter) 01/13/07
../src/state.cc: * Sccsid @(#)state.cc	1.4 (gritter) 2/26/07
../src/pmake.cc: * Sccsid @(#)pmake.cc	1.6 (gritter) 01/18/07
../src/nse_printdep.cc: * Sccsid @(#)nse_printdep.cc	1.4 (gritter) 01/13/07
../src/misc.cc: * Sccsid @(#)misc.cc	1.12 (gritter) 3/7/07
../src/main.cc: * Sccsid @(#)main.cc	1.26 (gritter) 8/7/10
../src/doname.cc: * Sccsid @(#)doname.cc	1.11 (gritter) 3/7/07
../src/read2.cc: * Sccsid @(#)read2.cc	1.10 (gritter) 3/8/07
../src/macro.cc: * Sccsid @(#)macro.cc	1.4 (gritter) 1/13/07
../mksh/i18n.cc: * Sccsid @(#)i18n.cc	1.3 (gritter) 01/13/07
../mksh/read.cc: * Sccsid @(#)read.cc	1.3 (gritter) 01/13/07
../mksh/dosys.cc: * Sccsid @(#)dosys.cc	1.10 (gritter) 3/7/07
../mksh/macro.cc: * Sccsid @(#)macro.cc	1.11 (gritter) 3/7/07
../mksh/posix.cc: * Sccsid @(#)posix.cc	1.1 (gritter) 01/13/07
../mksh/misc.cc: * Sccsid @(#)misc.cc	1.9 (gritter) 3/6/07
../mksh/mksh.cc: * Sccsid @(#)mksh.cc	1.4 (gritter) 01/13/07
../mksh/globals.cc: * Sccsid @(#)globals.cc	1.4 (gritter) 3/6/07
../mksh/wcslen.c: * Sccsid @(#)wcslen.c	1.3 (gritter) 01/23/07
../makestate/lock.c: * Sccsid @(#)lock.c	1.7 (gritter) 01/21/07
../makestate/ld_file.c: * Sccsid @(#)ld_file.c	1.8 (gritter) 01/23/07
../vroot/rmdir.cc: * Sccsid @(#)rmdir.cc	1.2 (gritter) 01/07/07
../vroot/unmount.cc: * Sccsid @(#)unmount.cc	1.2 (gritter) 01/07/07
../vroot/report.cc: * Sccsid @(#)report.cc	1.5 (gritter) 01/13/07
../vroot/setenv.cc: * Sccsid @(#)setenv.cc	1.3 (gritter) 01/13/07
../vroot/access.cc: * Sccsid @(#)access.cc	1.2 (gritter) 01/07/07
../vroot/chdir.cc: * Sccsid @(#)chdir.cc	1.2 (gritter) 01/07/07
../vroot/lock.cc: * Sccsid @(#)lock.cc	1.5 (gritter) 10/1/07
../vroot/vroot.cc: * Sccsid @(#)vroot.cc	1.3 (gritter) 01/13/07
../vroot/stat.cc: * Sccsid @(#)stat.cc	1.2 (gritter) 01/07/07
../vroot/chmod.cc: * Sccsid @(#)chmod.cc	1.2 (gritter) 01/07/07
../vroot/chown.cc: * Sccsid @(#)chown.cc	1.2 (gritter) 01/07/07
../vroot/execve.cc: * Sccsid @(#)execve.cc	1.2 (gritter) 01/07/07
../vroot/lstat.cc: * Sccsid @(#)lstat.cc	1.2 (gritter) 01/07/07
../vroot/mount.cc: * Sccsid @(#)mount.cc	1.4 (gritter) 01/23/07
../vroot/unlink.cc: * Sccsid @(#)unlink.cc	1.2 (gritter) 01/07/07
../vroot/mkdir.cc: * Sccsid @(#)mkdir.cc	1.2 (gritter) 01/07/07
../vroot/chroot.cc: * Sccsid @(#)chroot.cc	1.2 (gritter) 01/07/07
../vroot/truncate.cc: * Sccsid @(#)truncate.cc	1.2 (gritter) 01/07/07
../vroot/args.cc: * Sccsid @(#)args.cc	1.2 (gritter) 01/07/07
../vroot/statfs.cc: * Sccsid @(#)statfs.cc	1.3 (gritter) 01/13/07
../vroot/open.cc: * Sccsid @(#)open.cc	1.2 (gritter) 01/07/07
../vroot/creat.cc: * Sccsid @(#)creat.cc	1.2 (gritter) 01/07/07
../vroot/readlink.cc: * Sccsid @(#)readlink.cc	1.2 (gritter) 01/07/07
../vroot/utimes.cc: * Sccsid @(#)utimes.cc	1.2 (gritter) 01/07/07
../mksdmsi18n/libmksdmsi18n_init.cc: * Sccsid @(#)libmksdmsi18n_init.cc	1.5 (gritter) 01/13/07
../include/mk/copyright.h: * Sccsid @(#)copyright.h	1.2 (gritter) 01/07/07
../include/mk/defs.h: * Sccsid @(#)defs.h	1.10 (gritter) 8/7/10
../include/avo/avo_alloca.h: * Sccsid @(#)avo_alloca.h	1.4 (gritter) 01/21/07
../include/avo/intl.h: * Sccsid @(#)intl.h	1.3 (gritter) 01/13/07
../include/bsd/bsd.h: * Sccsid @(#)bsd.h	1.4 (gritter) 01/22/07
../include/mksh/dosys.h: * Sccsid @(#)dosys.h	1.2 (gritter) 01/07/07
../include/mksh/macro.h: * Sccsid @(#)macro.h	1.2 (gritter) 01/07/07
../include/mksh/i18n.h: * Sccsid @(#)i18n.h	1.2 (gritter) 01/07/07
../include/mksh/defs.h: * Sccsid @(#)defs.h	1.12 (gritter) 6/16/07
../include/mksh/misc.h: * Sccsid @(#)misc.h	1.2 (gritter) 01/07/07
../include/mksh/mksh.h: * Sccsid @(#)mksh.h	1.2 (gritter) 01/07/07
../include/mksh/read.h: * Sccsid @(#)read.h	1.2 (gritter) 01/07/07
../include/mksh/globals.h: * Sccsid @(#)globals.h	1.2 (gritter) 01/07/07
../include/mksh/libmksh_init.h: * Sccsid @(#)libmksh_init.h	1.2 (gritter) 01/07/07
../include/vroot/report.h: * Sccsid @(#)report.h	1.2 (gritter) 01/07/07
../include/vroot/args.h: * Sccsid @(#)args.h	1.4 (gritter) 01/23/07
../include/vroot/vroot.h: * Sccsid @(#)vroot.h	1.2 (gritter) 01/07/07
../include/mksdmsi18n/mksdmsi18n.h: * Sccsid @(#)mksdmsi18n.h	1.2 (gritter) 01/07/07
*/
