#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define USED    __attribute__ ((used))
#elif defined __GNUC__
#define USED    __attribute__ ((unused))
#else
#define USED
#endif
static const char id[] USED = "@(#)sccs.sl	1.30 (gritter) 4/23/07";
/* SLIST */
/*
admin.c: * Sccsid @(#)admin.c	1.5 (gritter) 2/26/07
comb.c: * Sccsid @(#)comb.c	1.4 (gritter) 01/21/07
delta.c: * Sccsid @(#)delta.c	1.8 (gritter) 3/17/07
get.c: * Sccsid @(#)get.c	1.8 (gritter) 2/13/07
help.c: * Sccsid @(#)help.c	1.5 (gritter) 12/21/06
prs.c: * Sccsid @(#)prs.c	1.6 (gritter) 01/21/07
prt.c: * Sccsid @(#)prt.c	1.4 (gritter) 01/21/07
rmchg.c: * Sccsid @(#)rmchg.c	1.6 (gritter) 2/26/07
sccs.c: * Sccsid @(#)sccs.c	1.14 (gritter) 2/25/07
unget.c: * Sccsid @(#)unget.c	1.6 (gritter) 01/21/07
val.c: * Sccsid @(#)val.c	1.5 (gritter) 01/21/07
vc.c: * Sccsid @(#)vc.c	1.4 (gritter) 3/25/07
what.c: * Sccsid @(#)what.c	1.4 (gritter) 4/14/07
../cassi/cmrcheck.c: * Sccsid @(#)cmrcheck.c	1.5 (gritter) 2/26/07
../cassi/deltack.c: * Sccsid @(#)deltack.c	1.5 (gritter) 2/26/07
../cassi/error.c: * Sccsid @(#)error.c	1.4 (gritter) 12/20/06
../cassi/filehand.c: * Sccsid @(#)filehand.c	1.6 (gritter) 2/26/07
../cassi/gf.c: * Sccsid @(#)gf.c	1.5 (gritter) 2/26/07
../comobj/auxf.c: * Sccsid @(#)auxf.c	1.4 (gritter) 12/20/06
../comobj/chkid.c: * Sccsid @(#)chkid.c	1.4 (gritter) 12/20/06
../comobj/chksid.c: * Sccsid @(#)chksid.c	1.4 (gritter) 12/20/06
../comobj/cmpdate.c: * Sccsid @(#)cmpdate.c	1.4 (gritter) 12/20/06
../comobj/date_ab.c: * Sccsid @(#)date_ab.c	1.5 (gritter) 01/12/07
../comobj/date_ba.c: * Sccsid @(#)date_ba.c	1.4 (gritter) 12/20/06
../comobj/del_ab.c: * Sccsid @(#)del_ab.c	1.4 (gritter) 12/20/06
../comobj/del_ba.c: * Sccsid @(#)del_ba.c	1.4 (gritter) 12/20/06
../comobj/dodelt.c: * Sccsid @(#)dodelt.c	1.5 (gritter) 12/25/06
../comobj/dofile.c: * Sccsid @(#)dofile.c	1.5 (gritter) 12/25/06
../comobj/dohist.c: * Sccsid @(#)dohist.c	1.4 (gritter) 12/20/06
../comobj/doie.c: * Sccsid @(#)doie.c	1.4 (gritter) 12/20/06
../comobj/dolist.c: * Sccsid @(#)dolist.c	1.5 (gritter) 12/25/06
../comobj/encode.c: * Sccsid @(#)encode.c	1.4 (gritter) 12/20/06
../comobj/eqsid.c: * Sccsid @(#)eqsid.c	1.4 (gritter) 12/20/06
../comobj/flushto.c: * Sccsid @(#)flushto.c	1.4 (gritter) 12/20/06
../comobj/fmterr.c: * Sccsid @(#)fmterr.c	1.5 (gritter) 01/03/07
../comobj/getline.c: * Sccsid @(#)getline.c	1.4 (gritter) 12/20/06
../comobj/getser.c: * Sccsid @(#)getser.c	1.4 (gritter) 12/20/06
../comobj/logname.c: * Sccsid @(#)logname.c	1.5 (gritter) 01/05/07
../comobj/newsid.c: * Sccsid @(#)newsid.c	1.4 (gritter) 12/20/06
../comobj/newstats.c: * Sccsid @(#)newstats.c	1.5 (gritter) 12/25/06
../comobj/permiss.c: * Sccsid @(#)permiss.c	1.5 (gritter) 01/05/07
../comobj/pf_ab.c: * Sccsid @(#)pf_ab.c	1.4 (gritter) 12/20/06
../comobj/putline.c: * Sccsid @(#)putline.c	1.5 (gritter) 12/20/06
../comobj/rdmod.c: * Sccsid @(#)rdmod.c	1.4 (gritter) 12/20/06
../comobj/setup.c: * Sccsid @(#)setup.c	1.4 (gritter) 12/20/06
../comobj/sid_ab.c: * Sccsid @(#)sid_ab.c	1.4 (gritter) 12/20/06
../comobj/sid_ba.c: * Sccsid @(#)sid_ba.c	1.4 (gritter) 12/20/06
../comobj/sidtoser.c: * Sccsid @(#)sidtoser.c	1.4 (gritter) 12/20/06
../comobj/sinit.c: * Sccsid @(#)sinit.c	1.5 (gritter) 01/03/07
../comobj/stats_ab.c: * Sccsid @(#)stats_ab.c	1.4 (gritter) 12/20/06
../comobj/strptim.c: * Sccsid @(#)strptim.c	1.4 (gritter) 12/20/06
../hdr/ccstypes.h: * Sccsid @(#)ccstypes.h	1.4 (gritter) 12/20/06
../hdr/defines.h: * Sccsid @(#)defines.h	1.5 (gritter) 12/25/06
../hdr/fatal.h: * Sccsid @(#)fatal.h	1.4 (gritter) 12/25/06
../hdr/filehand.h: * Sccsid @(#)filehand.h	1.3 (gritter) 12/20/06
../hdr/had.h: * Sccsid @(#)had.h	1.3 (gritter) 12/20/06
../hdr/i18n.h: * Sccsid @(#)i18n.h	1.3 (gritter) 12/20/06
../hdr/macros.h: * Sccsid @(#)macros.h	1.3 (gritter) 01/23/07
../hdr/macros.h: *	Note:	Sccsid[] strings are still supported but not the prefered
../hdr/macros.h:#define	SCCSID(arg)		static char Sccsid[] = #arg
../hdr/macros.h:#define	SCCSID(arg)		static char Sccsid[] = "arg"
../mpwlib/abspath.c: * Sccsid @(#)abspath.c	1.3 (gritter) 12/20/06
../mpwlib/any.c: * Sccsid @(#)any.c	1.3 (gritter) 12/20/06
../mpwlib/cat.c: * Sccsid @(#)cat.c	1.4 (gritter) 01/21/07
../mpwlib/dname.c: * Sccsid @(#)dname.c	1.3 (gritter) 12/20/06
../mpwlib/fatal.c: * Sccsid @(#)fatal.c	1.4 (gritter) 12/25/06
../mpwlib/fdfopen.c: * Sccsid @(#)fdfopen.c	1.3 (gritter) 12/20/06
../mpwlib/fmalloc.c: * Sccsid @(#)fmalloc.c	1.3 (gritter) 12/20/06
../mpwlib/getopt.c: * Sccsid @(#)getopt.c	1.9 (gritter) 4/2/07
../mpwlib/had.c: * Sccsid @(#)had.c	1.3 (gritter) 12/20/06
../mpwlib/imatch.c: * Sccsid @(#)imatch.c	1.3 (gritter) 12/20/06
../mpwlib/index.c: * Sccsid @(#)index.c	1.3 (gritter) 12/20/06
../mpwlib/lockit.c: * Sccsid @(#)lockit.c	1.5 (gritter) 12/25/06
../mpwlib/patoi.c: * Sccsid @(#)patoi.c	1.3 (gritter) 12/20/06
../mpwlib/repl.c: * Sccsid @(#)repl.c	1.3 (gritter) 12/20/06
../mpwlib/satoi.c: * Sccsid @(#)satoi.c	1.3 (gritter) 12/20/06
../mpwlib/setsig.c: * Sccsid @(#)setsig.c	1.4 (gritter) 12/25/06
../mpwlib/sname.c: * Sccsid @(#)sname.c	1.3 (gritter) 12/20/06
../mpwlib/strend.c: * Sccsid @(#)strend.c	1.3 (gritter) 12/20/06
../mpwlib/trnslat.c: * Sccsid @(#)trnslat.c	1.3 (gritter) 12/20/06
../mpwlib/userexit.c: * Sccsid @(#)userexit.c	1.3 (gritter) 12/20/06
../mpwlib/xcreat.c: * Sccsid @(#)xcreat.c	1.4 (gritter) 01/18/07
../mpwlib/xlink.c: * Sccsid @(#)xlink.c	1.3 (gritter) 12/20/06
../mpwlib/xmsg.c: * Sccsid @(#)xmsg.c	1.3 (gritter) 12/20/06
../mpwlib/xopen.c: * Sccsid @(#)xopen.c	1.3 (gritter) 12/20/06
../mpwlib/xpipe.c: * Sccsid @(#)xpipe.c	1.3 (gritter) 12/20/06
../mpwlib/xunlink.c: * Sccsid @(#)xunlink.c	1.3 (gritter) 12/20/06
../mpwlib/zero.c: * Sccsid @(#)zero.c	1.3 (gritter) 12/20/06
*/
