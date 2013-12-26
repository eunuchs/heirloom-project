/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2004 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)dist.cc 1.35 06/12/12
 */

/*	from OpenSolaris "dist.cc	1.25	96/03/12"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)dist.cc	1.5 (gritter) 01/20/07
 */

#ifdef DISTRIBUTED
/*
 *      dist.cc
 *
 *      Deal with the distributed processing
 */

#include <avo/err.h>
#include <avo/find_dir.h>
#include <avo/util.h>
#include <dm/Avo_AcknowledgeMsg.h>
#include <dm/Avo_DoJobMsg.h>
#include <dm/Avo_JobResultMsg.h>
#include <mk/defs.h>
#include <mksh/misc.h>		/* getmem() */
#include <rw/pstream.h>
#include <rw/queuecol.h>
#include <rw/xdrstrea.h>
#include <signal.h>
#ifdef linux
#include <sstream>
using namespace std;
#else
#include <strstream.h>
#endif
#include <sys/stat.h>		/* stat() */
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

/*
 * Defined macros
 */

#define AVO_BLOCK_INTERUPTS sigfillset(&newset) ; \
        sigprocmask(SIG_SETMASK, &newset, &oldset)
 
#define AVO_UNBLOCK_INTERUPTS \
        sigprocmask(SIG_SETMASK, &oldset, &newset)


/*
 * typedefs & structs
 */

/*
 * Static variables
 */
int		dmake_ifd; 
FILE*		dmake_ifp; 
XDR 		xdrs_in;

int		dmake_ofd;
FILE*		dmake_ofp;
XDR 		xdrs_out;

// These instances are required for the RWfactory.
static Avo_JobResultMsg dummyJobResultMsg;
static Avo_AcknowledgeMsg dummyAcknowledgeMsg;
static int firstAcknowledgeReceived = 0;
 
int		rxmPid = 0;

/*
 * File table of contents
 */
static void	set_dmake_env_vars(void);

/*
 * void
 * startup_rxm(void)
 *
 * When startup_rxm() is called, read_command_options() and
 * read_files_and_state() have already been read, so DMake
 * will now know what options to pass to rxm.
 *
 * rxm
 *     [ -k ] [ -n ]
 *     [ -c <dmake_rcfile> ]
 *     [ -g <dmake_group> ]
 *     [ -j <dmake_max_jobs> ]
 *     [ -m <dmake_mode> ]
 *     [ -o <dmake_odir> ]
 *     <read_fd> <write_fd>
 *
 * rxm will, among other things, read the rc file.
 *
 */
void
startup_rxm(void)
{
	Name		dmake_name;
	Name		dmake_value;
	Avo_err		*find_run_dir_err;
	int		pipe1[2], pipe2[2];
	Property	prop;
	char		*run_dir;
	char		rxm_command[MAXPATHLEN];
	int		rxm_debug = 0;

	int 		length;
	char *		env;

	firstAcknowledgeReceived = 0;
	/*
	 * Create two pipes, one for dmake->rxm, and one for rxm->dmake.
	 * pipe1 is dmake->rxm,
	 * pipe2 is rxm->dmake.
	 */
	if ((pipe(pipe1) < 0) || (pipe(pipe2) < 0)) {
		fatal("pipe() failed: %s", errmsg(errno));
	}

	set_dmake_env_vars();

	if ((rxmPid = fork()) < 0) {	/* error */
		fatal("fork() failed: %s", errmsg(errno));
	} else if (rxmPid > 0) {	/* parent, dmake */
		dmake_ofd = pipe1[1];	// write side of pipe
		if (!(dmake_ofp = fdopen(dmake_ofd, "a"))) {
			fatal("fdopen() failed: %s", errmsg(errno));
		}
		xdrstdio_create(&xdrs_out, dmake_ofp, XDR_ENCODE);

		dmake_ifd = pipe2[0];	// read side of pipe
		if (!(dmake_ifp = fdopen(dmake_ifd, "r"))) {
			fatal("fdopen() failed: %s", errmsg(errno));
		}
		xdrstdio_create(&xdrs_in, dmake_ifp, XDR_DECODE);

		close(pipe1[0]);	// read side
		close(pipe2[1]);	// write side
	} else {			/* child, rxm */
		close(pipe1[1]);	// write side
		close(pipe2[0]);	// read side

		/* Find the run directory of dmake, for rxm. */
		find_run_dir_err = avo_find_run_dir(&run_dir);
		if (find_run_dir_err) {
			delete find_run_dir_err;
			/* Use the path to find rxm. */
			sprintf(rxm_command, NOCATGETS("rxm"));
		} else {
			/* Use the run dir of dmake for rxm. */
			sprintf(rxm_command, NOCATGETS("%s/rxm"), run_dir);
		}

		if (continue_after_error) {
			strcat(rxm_command, NOCATGETS(" -k"));
		}
		if (do_not_exec_rule) {
			strcat(rxm_command, NOCATGETS(" -n"));
		}
		if (rxm_debug) {
			strcat(rxm_command, NOCATGETS(" -S"));
		}
		if (send_mtool_msgs) {
			sprintf(&rxm_command[strlen(rxm_command)],
			               NOCATGETS(" -O %d"),
			               mtool_msgs_fd);
		}
		MBSTOWCS(wcs_buffer, NOCATGETS("DMAKE_RCFILE"));
		dmake_name = GETNAME(wcs_buffer, FIND_LENGTH);
		if (((prop = get_prop(dmake_name->prop, macro_prop)) != NULL) &&
		    ((dmake_value = prop->body.macro.value) != NULL)) {
			sprintf(&rxm_command[strlen(rxm_command)],
			               NOCATGETS(" -c %s"),
			               dmake_value->string_mb);
		} else {
			length = 2 + strlen(NOCATGETS("DMAKE_RCFILE"));
			env = getmem(length);
			sprintf(env,
			               "%s=",
			               NOCATGETS("DMAKE_RCFILE"));
			putenv(env);
		}
		MBSTOWCS(wcs_buffer, NOCATGETS("DMAKE_GROUP"));
		dmake_name = GETNAME(wcs_buffer, FIND_LENGTH);
		if (((prop = get_prop(dmake_name->prop, macro_prop)) != NULL) &&
		    ((dmake_value = prop->body.macro.value) != NULL)) {
			sprintf(&rxm_command[strlen(rxm_command)],
			               NOCATGETS(" -g %s"),
			               dmake_value->string_mb);
		} else {
			length = 2 + strlen(NOCATGETS("DMAKE_GROUP"));
			env = getmem(length);
			sprintf(env,
			               "%s=",
			               NOCATGETS("DMAKE_GROUP"));
			putenv(env);
		}
		MBSTOWCS(wcs_buffer, NOCATGETS("DMAKE_MAX_JOBS"));
		dmake_name = GETNAME(wcs_buffer, FIND_LENGTH);
		if (((prop = get_prop(dmake_name->prop, macro_prop)) != NULL) &&
		    ((dmake_value = prop->body.macro.value) != NULL)) {
			sprintf(&rxm_command[strlen(rxm_command)],
			               NOCATGETS(" -j %s"),
			               dmake_value->string_mb);
		} else {
			length = 2 + strlen(NOCATGETS("DMAKE_MAX_JOBS"));
			env = getmem(length);
			sprintf(env,
			               "%s=",
			               NOCATGETS("DMAKE_MAX_JOBS"));
			putenv(env);
		}
		MBSTOWCS(wcs_buffer, NOCATGETS("DMAKE_MODE"));
		dmake_name = GETNAME(wcs_buffer, FIND_LENGTH);
		if (((prop = get_prop(dmake_name->prop, macro_prop)) != NULL) &&
		    ((dmake_value = prop->body.macro.value) != NULL)) {
			sprintf(&rxm_command[strlen(rxm_command)],
			               NOCATGETS(" -m %s"),
			               dmake_value->string_mb);
		} else {
			length = 2 + strlen(NOCATGETS("DMAKE_MODE"));
			env = getmem(length);
			sprintf(env,
			               "%s=",
			               NOCATGETS("DMAKE_MODE"));
			putenv(env);
		}
		MBSTOWCS(wcs_buffer, NOCATGETS("DMAKE_ODIR"));
		dmake_name = GETNAME(wcs_buffer, FIND_LENGTH);
		if (((prop = get_prop(dmake_name->prop, macro_prop)) != NULL) &&
		    ((dmake_value = prop->body.macro.value) != NULL)) {
			sprintf(&rxm_command[strlen(rxm_command)],
			               NOCATGETS(" -o %s"),
			               dmake_value->string_mb);
		} else {
			length = 2 + strlen(NOCATGETS("DMAKE_ODIR"));
			env = getmem(length);
			sprintf(env,
			               "%s=",
			               NOCATGETS("DMAKE_ODIR"));
			putenv(env);
		}

		sprintf(&rxm_command[strlen(rxm_command)],
		               NOCATGETS(" %d %d"),
		               pipe1[0], pipe2[1]);
		execl(bourne_shell,
		      NOCATGETS("sh"),
		      NOCATGETS("-c"),
		      rxm_command,
		      (char *)NULL);
		_exit(127);
	}
}

/*
 * static void
 * set_dmake_env_vars()
 *
 * Sets the DMAKE_* environment variables for rxm and rxs.
 *	DMAKE_PWD
 *	DMAKE_NPWD
 *	DMAKE_UMASK
 *	DMAKE_SHELL
 */
static void
set_dmake_env_vars()
{
	char		*current_netpath;
	char		*current_path;
	static char	*env;
	int		length;
	char		netpath[MAXPATHLEN];
	mode_t		um;
	char		um_buf[MAXPATHLEN];
	Name		dmake_name;
	Name		dmake_value;
	Property	prop;

#ifdef REDIRECT_ERR
	/* Set __DMAKE_REDIRECT_STDERR */
	length = 2 + strlen(NOCATGETS("__DMAKE_REDIRECT_STDERR")) + 1;
	env = getmem(length);
	sprintf(env,
	               "%s=%s",
	               NOCATGETS("__DMAKE_REDIRECT_STDERR"),
	               out_err_same ? NOCATGETS("0") : NOCATGETS("1"));
	putenv(env);
#endif

	/* Set DMAKE_PWD to the current working directory */
	current_path = get_current_path();
	length = 2 + strlen(NOCATGETS("DMAKE_PWD")) + strlen(current_path);
	env = getmem(length);
	sprintf(env,
	               "%s=%s",
	               NOCATGETS("DMAKE_PWD"),
	               current_path);
	putenv(env);

	/* Set DMAKE_NPWD to machine:pathname */
	if (avo_path_to_netpath(current_path, netpath)) {
		current_netpath = netpath;
	} else {
		current_netpath = current_path;
	}
	length = 2 + strlen(NOCATGETS("DMAKE_NPWD")) + strlen(current_netpath);
	env = getmem(length);
	sprintf(env,
	               "%s=%s",
	               NOCATGETS("DMAKE_NPWD"),
	               current_netpath);
	putenv(env);

	/* Set DMAKE_UMASK to the value of umask */
	um = umask(0);
	umask(um);
	sprintf(um_buf, NOCATGETS("%ul"), um);
	length = 2 + strlen(NOCATGETS("DMAKE_UMASK")) + strlen(um_buf);
	env = getmem(length);
	sprintf(env,
	               "%s=%s",
	               NOCATGETS("DMAKE_UMASK"),
	               um_buf);
	putenv(env);

	if (((prop = get_prop(shell_name->prop, macro_prop)) != NULL) &&
	    ((dmake_value = prop->body.macro.value) != NULL)) {
		length = 2 + strlen(NOCATGETS("DMAKE_SHELL")) +
		         strlen(dmake_value->string_mb);
		env = getmem(length);
		sprintf(env,
	    	           "%s=%s",
	    	           NOCATGETS("DMAKE_SHELL"),
	     	           dmake_value->string_mb);
		putenv(env);
	}
	MBSTOWCS(wcs_buffer, NOCATGETS("DMAKE_OUTPUT_MODE"));
	dmake_name = GETNAME(wcs_buffer, FIND_LENGTH);
	if (((prop = get_prop(dmake_name->prop, macro_prop)) != NULL) &&
	    ((dmake_value = prop->body.macro.value) != NULL)) {
		length = 2 + strlen(NOCATGETS("DMAKE_OUTPUT_MODE")) +
		         strlen(dmake_value->string_mb);
		env = getmem(length);
		sprintf(env,
	    	           "%s=%s",
	    	           NOCATGETS("DMAKE_OUTPUT_MODE"),
	     	           dmake_value->string_mb);
		putenv(env);
	}
}

/*
 * void
 * distribute_rxm(Avo_DoJobMsg *dmake_job_msg)
 *
 * Write the DMake rule to be distributed down the pipe to rxm.
 *
 */
void
distribute_rxm(Avo_DoJobMsg *dmake_job_msg)
{
	/* Add all dynamic env vars to the dmake_job_msg. */
	setvar_envvar(dmake_job_msg);

	/*
	 * Copying dosys()...
	 * Stat .make.state to see if we'll need to reread it later
	 */
	make_state->stat.time = file_no_time;
	exists(make_state);
	make_state_before = make_state->stat.time;

	// Wait for the first Acknowledge message from the rxm process
	// before sending the first message.
	if (!firstAcknowledgeReceived) {
		firstAcknowledgeReceived++;
		Avo_AcknowledgeMsg *msg = getAcknowledgeMsg();
		if (msg) {
			delete msg;
		}
	}

	RWCollectable *doJobMsg = (RWCollectable *)dmake_job_msg;
	sigset_t newset;
        sigset_t oldset;

	AVO_BLOCK_INTERUPTS;
	int xdrResult = xdr(&xdrs_out, doJobMsg);

	if (xdrResult) {
		fflush(dmake_ofp);
		AVO_UNBLOCK_INTERUPTS;
	} else {
		AVO_UNBLOCK_INTERUPTS;
		fatal("Couldn't send the job request to rxm");
	}

	delete dmake_job_msg;
}

// Queue for JobResult messages.
static RWSlistCollectablesQueue jobResultQueue;

// Queue for Acknowledge messages.
static RWSlistCollectablesQueue acknowledgeQueue;

// Read a message from the rxm process, and put it into a queue, by
// message type. Return the message type.

int
getRxmMessage(void)
{
        RWCollectable	*msg = (RWCollectable *)0;
	int		msgType = 0;
//	sigset_t	newset;
//	sigset_t	oldset;

	// It seems unnecessarily to block interrupts here because
	// any nonignored signal means exit for dmake in distributed mode.
//	AVO_BLOCK_INTERUPTS;
	int xdrResult = xdr(&xdrs_in, msg);
//	AVO_UNBLOCK_INTERUPTS;

	if (xdrResult) {
		switch(msg->isA()) {
		case __AVO_ACKNOWLEDGEMSG:
			acknowledgeQueue.append(msg);
			msgType = __AVO_ACKNOWLEDGEMSG;
			break;
		case __AVO_JOBRESULTMSG:
			jobResultQueue.append(msg);
			msgType = __AVO_JOBRESULTMSG;
			break;
		default:
			warning("Unknown message on rxm input fd");
			msgType = 0;
			break;
		}
	} else {
		if (errno == EINTR) {
			fputs(NOCATGETS("dmake: Internal error: xdr() has been interrupted by a signal.\n"), stderr);
		}
		fatal("Couldn't receive message from rxm");
	}

	return msgType;
}

// Get a JobResult message from it's queue, and
// if the queue is empty, call the getRxmMessage() function until
// a JobResult message shows.

Avo_JobResultMsg *
getJobResultMsg(void)
{
	RWCollectable *msg = 0;

	if (!(msg = jobResultQueue.get())) {
		while (getRxmMessage() != __AVO_JOBRESULTMSG);
		msg = jobResultQueue.get();
	}

	return (Avo_JobResultMsg *)msg;
}

// Get an Acknowledge message from it's queue, and
// if the queue is empty, call the getRxmMessage() function until
// a Acknowledge message shows.

Avo_AcknowledgeMsg *
getAcknowledgeMsg(void)
{
	RWCollectable *msg = 0;

	if (!(msg = acknowledgeQueue.get())) {
		while (getRxmMessage() != __AVO_ACKNOWLEDGEMSG);
		msg = acknowledgeQueue.get();
	}

	return (Avo_AcknowledgeMsg *)msg;
}


/*
 * Doname
 * await_dist(Boolean waitflg)
 *
 * Waits for distributed children to exit and finishes their processing.
 * Rxm will send a msg down the pipe when a child is done.
 *
 */
Doname
await_dist(Boolean waitflg)
{
	Avo_JobResultMsg	*dmake_result_msg;
	int			job_msg_id;
	Doname			result = build_ok;
	int			result_msg_cmd_status;
	int			result_msg_job_status;
	Running			rp;

	while (!(dmake_result_msg = getJobResultMsg()));
	job_msg_id = dmake_result_msg->getId();
	result_msg_cmd_status = dmake_result_msg->getCmdStatus();
	result_msg_job_status = dmake_result_msg->getJobStatus();

	if (waitflg) {
		result = (result_msg_cmd_status == 0) ? build_ok : build_failed;

#ifdef PRINT_EXIT_STATUS
		if (result == build_ok) {
			warning(NOCATGETS("I'm in await_dist(), waitflg is true, and result is build_ok."));
		} else {
			warning(NOCATGETS("I'm in await_dist(), waitflg is true, and result is build_failed."));
		}
#endif

	} else {
		for (rp = running_list;
		     (rp != NULL) && (job_msg_id != rp->job_msg_id);
		     rp = rp->next) {
		}
		if (rp == NULL) {
			fatal("Internal error: returned child job_msg_id not in running_list");
		} else {
			/* XXX - This may not be correct! */
			if (result_msg_job_status == RETURNED) {
				rp->state = build_ok;
			} else {
				rp->state = (result_msg_cmd_status == 0) ? build_ok : build_failed;
			}
			result = rp->state;

#ifdef PRINT_EXIT_STATUS
			if (result == build_ok) {
				warning(NOCATGETS("I'm in await_dist(), waitflg is false, and result is build_ok."));
			} else {
				warning(NOCATGETS("I'm in await_dist(), waitflg is false, and result is build_failed."));
			}
#endif

		}
		parallel_process_cnt--;
	}
	delete dmake_result_msg;
	return result;
}

#endif


