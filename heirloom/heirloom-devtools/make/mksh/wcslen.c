/*
 * Sccsid @(#)wcslen.c	1.3 (gritter) 01/23/07
 */
#if defined (__FreeBSD__) || defined (__APPLE__) || defined (__hpux) || defined (_AIX)
#include <wchar.h>
#include <stdlib.h>
wchar_t *
wcsdup(const wchar_t *s)
{
	wchar_t	*n;

	if ((n = malloc((wcslen(s) + 1) * sizeof *n)) == NULL)
		return NULL;
	wcscpy(n, s);
	return n;
}
#endif
