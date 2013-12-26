/*
 * Sccsid @(#)posix.cc	1.1 (gritter) 01/13/07
 */
int POSIX = 1;
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define USED    __attribute__ ((used))
#elif defined __GNUC__
#define USED    __attribute__ ((unused))
#else
#define USED
#endif
static const char id[] USED = "@(#)make_sus.cc	1.1 (gritter) 01/13/07";
