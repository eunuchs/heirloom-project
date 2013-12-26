/*
 * Sccsid @(#)getvfsent.c	1.3 (gritter) 2/25/07
 */

#ifndef	__sun
#include <stdio.h>
#include "sys/vfstab.h"

int
getvfsent(FILE *fp, struct vfstab *vp)
{
	return -1;
}

#endif
