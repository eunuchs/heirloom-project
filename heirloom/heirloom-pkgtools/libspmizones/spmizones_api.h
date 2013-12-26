/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SPMIZONES_API_H
#define	_SPMIZONES_API_H

/*	from OpenSolaris "spmizones_api.h	1.1	06/11/17 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)spmizones_api.h	1.6 (gritter) 2/24/07
 */

/*
 * Module:	spmizones_api.h
 * Group:	libspmizones
 * Description:	This module contains the libspmizones API data structures,
 *		constants, and function prototypes.
 */

/*
 * required includes
 */

/* System includes */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#ifdef __sun
#include <libzonecfg.h>
#endif

/*
 * C++ prefix
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * constants
 */

/*
 * macros
 */


/* function prototypes */

/* printf-type function definition */

typedef void (*_z_printf_fcn_t)(char *a_format, ...);

/* zone list structure */

typedef struct _zoneListElement_t *zoneList_t;

/* flag for zone locking functions */

typedef unsigned long ZLOCKS_T;

/* flags for zone locking */

#define	ZLOCKS_ZONE_ADMIN	((ZLOCKS_T)0x00000001)	/* zone admin */
#define	ZLOCKS_PKG_ADMIN	((ZLOCKS_T)0x00000002)	/* package admin */
#define	ZLOCKS_PATCH_ADMIN	((ZLOCKS_T)0x00000004)	/* patch admin */
#define	ZLOCKS_ALL		((ZLOCKS_T)0xFFFFFFFF)	/* all locks */
#define	ZLOCKS_NONE		((ZLOCKS_T)0x00000000)	/* no locks */

/*
 * external function definitions
 */

/* zones.c */

#ifdef __sun
extern boolean_t	z_zones_are_implemented(void);
extern void		z_set_zone_root(const char *zroot);
extern boolean_t	z_zlist_is_zone_runnable(zoneList_t a_zoneList,
				int a_zoneIndex);
extern boolean_t	z_zlist_restore_zone_state(zoneList_t a_zoneList,
				int a_zoneIndex);
extern boolean_t	z_zlist_change_zone_state(zoneList_t a_zoneList,
				int a_zoneIndex, zone_state_t a_newState);
extern char		*z_get_zonename(void);
extern zone_state_t	z_zlist_get_current_state(zoneList_t a_zoneList,
				int a_zoneIndex);
extern char 		**z_zlist_get_inherited_pkg_dirs(zoneList_t a_zoneList,
				int a_zoneIndex);
extern zone_state_t	z_zlist_get_original_state(zoneList_t a_zoneList,
				int a_zoneIndex);
extern int		z_zoneExecCmdArray(int *r_status, char **r_results,
				char *a_inputFile, char *a_path, char *a_argv[],
				const char *a_zoneName, int *a_fds);
extern int		z_zone_exec(const char *zonename, const char *path,
				char *argv[], char *a_stdoutPath,
				char *a_stderrPath, int *a_fds);
extern boolean_t	z_create_zone_admin_file(char *a_zoneAdminFilename,
				char *a_userAdminFilename);
extern void		z_free_zone_list(zoneList_t a_zoneList);
extern zoneList_t	z_get_nonglobal_zone_list(void);
extern boolean_t	z_lock_zones(zoneList_t a_zlst, ZLOCKS_T a_lflags);
extern boolean_t	z_non_global_zones_exist(void);
extern boolean_t	z_running_in_global_zone(void);
extern void		z_set_output_functions(_z_printf_fcn_t a_echo_fcn,
				_z_printf_fcn_t a_echo_debug_fcn,
				_z_printf_fcn_t a_progerr_fcn);
extern int		z_set_zone_spec(const char *zlist);
extern int		z_verify_zone_spec(void);
extern boolean_t	z_on_zone_spec(const char *zonename);
extern boolean_t	z_global_only(void);
extern boolean_t	z_unlock_zones(zoneList_t a_zlst, ZLOCKS_T a_lflags);
extern boolean_t	z_lock_this_zone(ZLOCKS_T a_lflags);
extern boolean_t	z_unlock_this_zone(ZLOCKS_T a_lflags);
extern char		*z_zlist_get_zonename(zoneList_t a_zoneList,
				int a_zoneId);
extern char		*z_zlist_get_zonepath(zoneList_t a_zoneList,
				int a_zoneId);
extern char		*z_zlist_get_scratch(zoneList_t a_zoneList,
				int a_zoneId);
extern boolean_t	z_umount_lz_mount(char *a_lzMountPoint);
extern boolean_t	z_mount_in_lz(char **r_lzMountPoint,
				char **r_lzRootPath,
				char *a_zoneName, char *a_gzPath,
				char *a_mountPointPrefix);
extern boolean_t	z_is_zone_branded(char *zoneName);
extern boolean_t	z_zones_are_implemented(void);

/* zones_exec.c */
extern int		z_ExecCmdArray(int *r_status, char **r_results,
				char *a_inputFile, char *a_cmd, char **a_args);
/*VARARGS*/
extern int		z_ExecCmdList(int *r_status, char **r_results,
				char *a_inputFile, char *a_cmd, ...);

/* zones_paths.c */
extern boolean_t	z_add_inherited_file_system(
				char *a_inheritedFileSystem);
extern boolean_t	z_path_is_inherited(char *a_path, char a_ftype,
				char *a_rootDir);
extern char **		z_get_inherited_file_systems(void);
extern char		*z_make_zone_root(char *);
extern void		z_path_canonize(char *file);
extern void		z_free_inherited_file_systems(void);

/* zones_lofs.c */
extern void z_destroyMountTable(void);
extern int z_createMountTable(void);
extern int z_isPathWritable(const char *);

/* zones_states.c */
extern int UmountAllZones(char *mntpnt);

#else	/* !__sun */

#define	z_zones_are_implemented()	0
#define	z_set_zone_root(a)
#define	z_zlist_is_zone_runnable(a, b)	0
#define	z_zlist_restore_zone_state(a, b)	0
#define	z_zlist_change_zone_state(a, b, c)	0
#define	z_get_zonename()	(NULL)
typedef	int	zone_state_t;
#define	z_zlist_get_current_state(a, b)	(-1)
#define	z_zlist_get_inherited_pkg_dirs(a, b)	(NULL)
#define	z_zlist_get_original_state(a, b)	(-1)
#define	z_zoneExecCmdArray(a, b, c, d, e, f, g)	(-1)
#define	z_zone_exec(a, b, c, d, e, f)	(-1)
#define	z_create_zone_admin_file(a, b)	0
#define	z_free_zone_list(a)
#define	z_get_nonglobal_zone_list()	(-1)
#define	z_lock_zones(a, b)	0
#define	z_non_global_zones_exist()	0
#define	z_running_in_global_zone()	1
#define	z_set_output_functions(a, b, c)
#define	z_set_zone_spec(a)	(-1)
#define	z_verify_zone_spec()	(-1)
#define	z_on_zone_spec(a)	0
#define	z_global_only()		1
#define	z_unlock_zones(a, b)	1
#define	z_lock_this_zone(a)	1
#define	z_unlock_this_zone(a)	1
#define	z_zlist_get_zonename(a, b)	(NULL)
#define	z_zlist_get_zonepath(a, b)	(NULL)
#define	z_zlist_get_scratch(a, b)	(NULL)
#define	z_umount_lz_mount(a)	0
#define	z_mount_in_lz(a, b, c, d, e)	0
#define	z_is_zone_branded(a)	0
#define	z_zones_are_implemented()	0

/* zones_exec.c */
#define	z_ExecCmdArray(a, b, c, d, e)	(-1)
/*VARARGS*/
#define	z_ExecCmdList(a, b, c, d, ...)	(-1)

/* zones_paths.c */
#define	z_add_inherited_file_system(a)	0
#define	z_path_is_inherited(a, b, c)	0
#define	z_get_inherited_file_systems()	(NULL)
#define	z_make_zone_root(a)	(NULL)
#define	z_path_canonize(a)
#define	z_free_inherited_file_systems()

/* zones_lofs.c */
#define	z_destroyMountTable()
#define	z_createMountTable()	(-1)
#define	z_isPathWritable(a)	0

/* zones_states.c */
#define	UmountAllZones(a)	(-1)

enum {
	ZONE_STATE_RUNNING,
	ZONE_STATE_MOUNTED
};

typedef	int	zoneid_t;

#define	getzoneid()	(0)

#endif	/* !__sun */

/*
 * C++ postfix
 */

#ifdef __cplusplus
}
#endif

#endif /* _SPMIZONES_API_H */
