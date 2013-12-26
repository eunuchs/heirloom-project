/*
 * Sccsid @(#)resolvepath.c	1.1 (gritter) 2/24/07
 */

#ifndef	__sun
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

int
resolvepath(const char *path, char *buf, size_t bufsiz)
{
	char	buf2[PATH_MAX+1];
	size_t	n;

	if (realpath(path, buf2) == NULL)
		return -1;
	n = strlen(buf2);
	if (n + 1 < bufsiz) {
		errno = ENAMETOOLONG;	/* not really correct */
		return -1;
	}
	strcpy(buf, buf2);
	return n;
}

#endif	/* !__sun */
