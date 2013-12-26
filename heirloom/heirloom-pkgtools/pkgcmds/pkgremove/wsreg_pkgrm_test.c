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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "wsreg_pkgrm_test.c	1.2	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)wsreg_pkgrm_test.c	1.2 (gritter) 2/24/07
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <wsreg.h>
#include <errno.h>

#include <sys/types.h>
#include <string.h>
#include <sys/fcntl.h>

#include "wsreg_pkgrm.h"

static void 
fail(const char *pc) 
{
	printf("fail: %s\n", pc);
	exit(-1);
}

static void
unreg(Wsreg_component *pws)
{
	Wsreg_component **ppws = NULL;
	
	if (pws == NULL)
		return;
	ppws = wsreg_get_dependent_components(pws);
	if (ppws == NULL) {
		if (wsreg_unregister(pws) == 0)
			printf("warning: unreg %s failed - probably already "
			    "unregistered\n",
			    pws->id);
	} else {
		int i;
		for (i = 0; ppws[i] != NULL; i++) {
			printf("unregister %s which depends on %s\n",
			    ppws[i]->id, pws->id);
			unreg(ppws[i]);
		}
		printf("unregister %s, now that "
		    "all comps depending on it gone:\n",
		    pws->id);
		if (wsreg_unregister(pws) == 0)
			printf("===failed\n");
	}
}

static Wsreg_component *
create_component(const char *pcID, const char *pcLocation,
    const char *pcPKGS, const char *pcVersion, const char *pcDispName,
    Wsreg_component_type wtype)
{
	Wsreg_component *pws = wsreg_create_component(pcID);
	if (pws == NULL) 
		fail("wsreg_create_component");

	if (wsreg_set_unique_name(pws, pcID) == 0)
		fail("wsreg_set_unique_name");

	if (wsreg_set_version(pws, pcVersion) == 0)
		fail("wsreg_set_version");
	
	if (wsreg_add_display_name(pws, "en", pcDispName) == 0)
		fail("wsreg_add_display_name");

	if (wsreg_set_type(pws, wtype) == 0)
		fail("wsreg_set_type");

	if (wsreg_set_location(pws, pcLocation) == 0)
		fail("wsreg_set_location");

	if (wsreg_set_data(pws, "pkgs", pcPKGS) == 0)
		fail("wsreg_set_data");

	if (wsreg_register(pws) == 0)
		fail("wsreg_register");

	return (pws);
}

char *pcVersion[] = {
	"1.0",
	"2.0",
	"2.3",
	"2.5",
	"3.1",
	"5.4",
	"6.1",
	"9.9",
	NULL};

char *pcID[] = {
	"wsreg_pkgtest-1111-1111",
	"wsreg_pkgtest-2222-2222",
	"wsreg_pkgtest-3333-3333",
	"wsreg_pkgtest-4444-4444",
	"wsreg_pkgtest-5555-5555",
	"wsreg_pkgtest-6666-6666",
	"wsreg_pkgtest-7777-7777",
	"wsreg_pkgtest-8888-8888",
	NULL};

char *pcName[] = {
	"wsreg_test_name 1",
	"wsreg_test_name 2",
	"wsreg_test_name 3",
	"wsreg_test_name 4",
	"wsreg_test_name 5",
	"wsreg_test_name 6",
	"wsreg_test_name 7",
	"wsreg_test_name 8",
	NULL};

char *pcPkgs_c[] = {
	"",
	"SUNWwsregtest1",
	"SUNWwsregtest2 SUNWwsregtest3 SUNWwsregtest4",
	"",
	"SUNWwsregtest5",
	"SUNWwsregtest6",
	"",
	"SUNWwsregtest6",
	NULL};

Wsreg_component_type w[] = {
	WSREG_COMPONENT, WSREG_COMPONENT, WSREG_COMPONENT, WSREG_COMPONENT,
	WSREG_FEATURE, WSREG_FEATURE, WSREG_FEATURE, WSREG_PRODUCT };

static void
show(char **a, char **b) 
{
	int i;
	for (i = 0; a[i] != NULL && b[i] != NULL; i++) {
		printf("%s\t%s\n", a[i], b[i]);
	}
}

int main()
{
	char buf[256], **id, **name;
	Wsreg_component *pws, *ppws[9];
	int i, r;
	char *pcroot = "/";
	if (getenv("PKG_INSTALL_ROOT"))
		pcroot = getenv("PKG_INSTALL_ROOT");

	ppws[8] = NULL;

	/* 00000000000000000000000000000000000000000000000000000000000
	   Set up
	   00000000000000000000000000000000000000000000000000000000000 */

	if (wsreg_initialize(WSREG_INIT_NORMAL, pcroot) != WSREG_SUCCESS)
	    fail("wsreg_initialize not WSREG_SUCCESS");

	if (wsreg_can_access_registry(O_RDWR) == 0)
	    fail("wsreg_can_access_registry returned 0");

	/*
	 * Create entries to obtain the following cases:
	 *  1. can remove - no pkgs
	 *  2. cannot remove - one pkg
	 *  3. cannot remove - in list of pkgs (try beginning, middle &end)
	 *  4. cannot remove - one pkg, dependents (should show up)
	 *  5. more than one component has the same pkg.
	 */
	
	for (i = 0; pcID[i] != NULL; i++) {

		ppws[i] = create_component(pcID[i], "/tmp/foo", pcPkgs_c[i],
		    pcVersion[i], pcName[i], w[i]);
		if (ppws[i] == NULL || wsreg_register(ppws[i]) == 0)
			fail("crate and register components");
		if (wsreg_set_instance(ppws[i], 1) == 0)
			fail("wsreg_set_instance");
	}

	/* 4 needs 3, so 3 has 4 as dependent, 4 has 3 as required */
	if (wsreg_add_dependent_component(ppws[3],ppws[4]) == 0)
		fail("wsreg_add_dependent");
	if (wsreg_add_required_component(ppws[4],ppws[3]) == 0)
		fail("wsreg_add_required");
	if (wsreg_register(ppws[3]) == 0 ||
	    wsreg_register(ppws[4]) == 0)
		fail("wsreg_register");

	/* 6 needs 0, 0 has 6 as dependent, 6 has 0 as required */
	if (wsreg_add_dependent_component(ppws[0], ppws[6]) == 0)
		fail("wsreg_add_dependent");
	if (wsreg_add_required_component(ppws[6], ppws[0]) == 0)
		fail("wsreg_add_required");
	if (wsreg_register(ppws[0]) == 0 ||
	    wsreg_register(ppws[6]) == 0)
		fail("wsreg_register");

	/* 7 needs 6, 6 has 7 as dependent, 7 has 6 as required */
	if (wsreg_add_dependent_component(ppws[6], ppws[7]) == 0)
		fail("wsreg_add_dependent");
	if (wsreg_add_required_component(ppws[7], ppws[6]) == 0)
		fail("wsreg_add_required");
	if (wsreg_register(ppws[6]) == 0 ||
	    wsreg_register(ppws[7]) == 0)
		fail("wsreg_register");


	/* verify all items were registered */
	for (i = 0; pcID[i] != NULL; i++) {
		Wsreg_query *pq = wsreg_query_create();
		wsreg_query_set_id(pq, pcID[i]);
		wsreg_query_set_instance(pq, 1);
		pws = wsreg_get(pq);
		if (pws == NULL) {
			printf("unreg.%d %s failed to register\n",
			    i, pcID[i]);
		} else {
			wsreg_free_component(pws);
		}
	}

	/* 00000000000000000000000000000000000000000000000000000000000
	   Perform tests
	   00000000000000000000000000000000000000000000000000000000000 */

	/* 1 Query: SUNWwsregtest0  Expected Result: OK */
	id = NULL;
	name = NULL;
	r = wsreg_pkgrm_check("/", "SUNWwsregtest0", &id, &name);
	if (r == 1 || id != NULL || name != NULL)
		printf("test.1 shows a conflict where there should be none\n");
	else if (r < 0)
		printf("test.1 error [%d] = %s\n", errno, strerror(errno));

	/* 2 Query: SUNWwsregtest1  Expected Result: NOT OK {2} */

	id = NULL;
	name = NULL;
	r = wsreg_pkgrm_check("/", "SUNWwsregtest1", &id, &name);
	if (r == 0 || id == NULL || name == NULL)
		printf("test.2 shows no conflict where there should be one\n");
	else if (r < 0)
		printf("test.2 error [%d] = %s\n", errno, strerror(errno));
	else {
		if (id[1] != NULL || name[1] != NULL ||
		    id[0] == NULL || strcmp(pcID[1], id[0]) != 0 ||
		    name[0] == NULL || strcmp(pcName[1], name[0]) != 0) {
			printf("test.2 bad results\n");
			printf("expected:\n%s\t%s\n",
			    pcID[1], pcName[1]);
			printf("got:\n");
			show(id, name);
		}
	}

	/* 3 Query: SUNWwsregtest2  Expected Result: NOT OK {3} */

	id = NULL;
	name = NULL;
	r = wsreg_pkgrm_check("/", "SUNWwsregtest2", &id, &name);
	if (r == 0 || id == NULL || name == NULL)
		printf("test.3 shows no conflict where there should be one\n");
	else if (r < 0)
		printf("test.3 error [%d] = %s\n", errno, strerror(errno));
	else {
		if (id[1] != NULL || name[1] != NULL ||
		    id[0] == NULL ||  strcmp(pcID[2], id[0]) != 0 ||
		    name[0] == NULL || strcmp(pcName[2], name[0]) != 0) {
			printf("test.3 bad results\n");
			printf("expected:\n%s\t%s\n",
			    pcID[2], pcName[2]);
			printf("got:\n");
			show(id, name);
		}
	}

	/* 4 Query: SUNWwsregtest3  Expected Result: NOT OK {3} */
	id = NULL;
	name = NULL;
	r = wsreg_pkgrm_check("/", "SUNWwsregtest3", &id, &name);
	if (r == 0 || id == NULL || name == NULL)
		printf("test.4 shows no conflict where there should be one\n");
	else if (r < 0)
		printf("test.4 error [%d] = %s\n", errno, strerror(errno));
	else {
		if (id[1] != NULL || name[1] != NULL ||
		    id[0] == NULL || strcmp(pcID[2], id[0]) != 0 ||
		    name[0] == NULL || strcmp(pcName[2], name[0]) != 0) {
			printf("test.4 bad results\n");
			printf("expected:\n%s\t%s\n",
			    pcID[2], pcName[2]);
			printf("got:\n");
			show(id, name);
		}

	}


	/* 5 Query: SUNWwsregtest4  Expected Result: NOT OK {3} */
	id = NULL;
	name = NULL;
	r = wsreg_pkgrm_check("/", "SUNWwsregtest4", &id, &name);
	if (r == 0 || id == NULL || name == NULL)
		printf("test.5 shows no conflict where there should be one\n");
	else if (r < 0)
		printf("test.5 error [%d] = %s\n", errno, strerror(errno));
	else {
		if (id[1] != NULL || name[1] != NULL ||
		    id[0] == NULL || strcmp(pcID[2], id[0]) != 0 ||
		    name[0] == NULL || strcmp(pcName[2], name[0]) != 0) {
			printf("test.5 bad results\n");
			printf("expected:\n%s\t%s\n",
			    pcID[2], pcName[2]);
			printf("got:\n");
			show(id, name);
		}

	}

	/* 6 Query: SUNWwsregtest5  Expected Result: NOT OK {5,4} */

	id = NULL;
	name = NULL;
	r = wsreg_pkgrm_check("/", "SUNWwsregtest5", &id, &name);
	if (r == 0 || id == NULL || name == NULL)
		printf("test.6 shows no conflict where there should be one\n");
	else if (r < 0)
		printf("test.6 error [%d] = %s\n", errno, strerror(errno));
	else {
		if (id[2] != NULL || name[2] != NULL ||
		    id[0] == NULL || strcmp(pcID[4], id[0]) != 0 ||
		    name[0] == NULL || strcmp(pcName[4], name[0]) != 0 ||
		    id[1] == NULL || strcmp(pcID[3], id[1]) != 0 ||
		    name[1] == NULL || strcmp(pcName[3], name[1]) != 0) {
			printf("test.6 bad results\n");
			printf("expected:\n%s\t%s\n%s\t%s\n",
			    pcID[4], pcName[4], pcID[3], pcName[3]);
			printf("got:\n");
			show(id, name);
		}
	}

	/* 7 Query: SUNWwsregtest6  Expected Result: NOT OK {8,6,7,1} */

	id = NULL;
	name = NULL;
	r = wsreg_pkgrm_check("/", "SUNWwsregtest6", &id, &name);
	if (r == 0 || id == NULL || name == NULL)
		printf("test.7 shows no conflict where there should be one\n");
	else if (r < 0)
		printf("test.7 error [%d] = %s\n", errno, strerror(errno));
	else {
		if (id[4] != NULL || name[4] != NULL || /* too many? */
		    id[0] == NULL || strcmp(pcID[5], id[0]) != 0 ||
		    name[0] == NULL || strcmp(pcName[5], name[0]) != 0 ||
		    id[1] == NULL || strcmp(pcID[7], id[1]) != 0 ||
		    name[1] == NULL || strcmp(pcName[7], name[1]) != 0 ||
		    id[2] == NULL || strcmp(pcID[6], id[2]) != 0 ||
		    name[2] == NULL || strcmp(pcName[6], name[2]) != 0 ||
		    id[3] == NULL || strcmp(pcID[0], id[3]) != 0 ||
		    name[3] == NULL || strcmp(pcName[0], name[3]) != 0) {
			printf("test.7 bad results\n");
			printf("expected:\n%s\t%s\n%s\t%s\n%s\t%s\n%s\t%s\n",
			    pcID[5], pcName[5], pcID[7], pcName[7],
			    pcID[6], pcName[6], pcID[0], pcName[0]);
			printf("got:\n");
			show(id, name);
		}
	}

	/* 00000000000000000000000000000000000000000000000000000000000
	   Clean up
	   00000000000000000000000000000000000000000000000000000000000 */

	for (i = 0; ppws[i] != NULL; i++)
		unreg(ppws[i]);

	/* verify all items were unregistered */
	for (i = 0; i <= 7; i++) {
		Wsreg_query *pq = wsreg_query_create();
		wsreg_query_set_id(pq, pcID[i]);
		wsreg_query_set_instance(pq, 1);
		pws = wsreg_get(pq);
		if (pws != NULL) {
			printf("unreg.%d %s failed to unregister\n",
			    i, pcID[i]);
			wsreg_free_component(pws);
		}
	}
}
