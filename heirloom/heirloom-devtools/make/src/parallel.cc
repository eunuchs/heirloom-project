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
 * Copyright 2005 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)parallel.cc 1.75 06/12/12
 */

/*	from OpenSolaris "parallel.cc	1.75	06/12/12"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)parallel.cc	1.7 (gritter) 3/7/07
 */

/*
 *	parallel.cc
 *
 *	Deal with the parallel processing
 */

/*
 * Included files
 */
#ifdef DISTRIBUTED
#include <avo/strings.h>	/* AVO_STRDUP() */
#include <dm/Avo_DoJobMsg.h>
#include <dm/Avo_MToolJobResultMsg.h>
#endif
#include <errno.h>		/* errno */
#include <fcntl.h>
#ifdef TEAMWARE_MAKE_CMN
#include <avo/util.h>		/* avo_get_user(), avo_hostname() */
#endif
#include <mk/defs.h>
#include <mksh/dosys.h>		/* redirect_io() */
#include <mksh/macro.h>		/* expand_value() */
#include <mksh/misc.h>		/* getmem() */
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef SGE_SUPPORT
#include <dmthread/Avo_PathNames.h>
#endif


/*
 * Defined macros
 */
#define MAXRULES		100

/*
 * This const should be in avo_dms/include/AvoDmakeCommand.h
 */
const int local_host_mask = 0x20;


/*
 * typedefs & structs
 */


/*
 * Static variables
 */
#ifdef TEAMWARE_MAKE_CMN
static	Boolean		just_did_subtree = false;
static	char		local_host[MAXNAMELEN] = "";
static	char		user_name[MAXNAMELEN] = "";
#else
#define	local_host	"localhost"
#endif
static	pid_t		process_running = -1;
static	Running		*running_tail = &running_list;
static	Name		subtree_conflict;
static	Name		subtree_conflict2;


/*
 * File table of contents
 */
#ifdef DISTRIBUTED
static	void		append_dmake_cmd(Avo_DoJobMsg *dmake_job_msg, char *orig_cmd_line, int cmd_options);
static	void		append_job_result_msg(Avo_MToolJobResultMsg *msg, char *outFn, char *errFn);
static	void		send_job_result_msg(Running rp);
#endif
static	void		delete_running_struct(Running rp);
static	Boolean		dependency_conflict(Name target);
static	Doname		distribute_process(char **commands, Property line);
static	void		doname_subtree(Name target, Boolean do_get, Boolean implicit);
static	void		dump_out_file(char *filename, Boolean err);
static	void		finish_doname(Running rp);
static	void		maybe_reread_make_state(void);
static	void		process_next(void);
static	void		reset_conditionals(int cnt, Name *targets, Property *locals);
static	pid_t           run_rule_commands(char *host, char **commands);
static	Property	*set_conditionals(int cnt, Name *targets);
static	void		store_conditionals(Running rp);


/*
 *	execute_parallel(line, waitflg)
 *
 *	DMake 2.x:
 *	parallel mode: spawns a parallel process to execute the command group.
 *	distributed mode: sends the command group down the pipe to rxm.
 *
 *	Return value:
 *				The result of the execution
 *
 *	Parameters:
 *		line		The command group to execute
 */
Doname
execute_parallel(Property line, Boolean waitflg, Boolean local)
{
	int			argcnt;
	int			cmd_options = 0;
	char			*commands[MAXRULES + 5];
	char			*cp;
#ifdef DISTRIBUTED
	Avo_DoJobMsg		*dmake_job_msg = NULL;
#endif
	Name			dmake_name;
	Name			dmake_value;
	int			ignore = 0;
	Name			make_machines_name;
	char			**p;
	Property		prop;
#ifdef TEAMWARE_MAKE_CMN
	Doname			result = build_ok;
#endif
	Cmd_line		rule;
	Boolean			silent_flag = false;
#ifdef TEAMWARE_MAKE_CMN
	Name			target = line->body.line.target;
	Boolean			wrote_state_file = false;
#endif

	if ((pmake_max_jobs == 0) &&
	    (dmake_mode_type == parallel_mode)) {
#ifdef TEAMWARE_MAKE_CMN
		if (user_name[0] == '\0') {
			avo_get_user(user_name, NULL);
		}
		if (local_host[0] == '\0') {
			strcpy(local_host, avo_hostname());
		}
#endif
		MBSTOWCS(wcs_buffer, NOCATGETS("DMAKE_MAX_JOBS"));
		dmake_name = GETNAME(wcs_buffer, FIND_LENGTH);
		if (((prop = get_prop(dmake_name->prop, macro_prop)) != NULL) &&
		    ((dmake_value = prop->body.macro.value) != NULL)) {
			pmake_max_jobs = atoi(dmake_value->string_mb);
			if (pmake_max_jobs <= 0) {
				warning("DMAKE_MAX_JOBS cannot be less than or equal to zero.");
				warning("setting DMAKE_MAX_JOBS to %d.", PMAKE_DEF_MAX_JOBS);
				pmake_max_jobs = PMAKE_DEF_MAX_JOBS;
			}
		} else {
			/*
			 * For backwards compatibility w/ PMake 1.x, when
			 * DMake 2.x is being run in parallel mode, DMake
			 * should parse the PMake startup file
			 * $(HOME)/.make.machines to get the pmake_max_jobs.
			 */
			MBSTOWCS(wcs_buffer, NOCATGETS("PMAKE_MACHINESFILE"));
			dmake_name = GETNAME(wcs_buffer, FIND_LENGTH);
			if (((prop = get_prop(dmake_name->prop, macro_prop)) != NULL) &&
			    ((dmake_value = prop->body.macro.value) != NULL)) {
				make_machines_name = dmake_value;
			} else {
				make_machines_name = NULL;
			}
			if ((pmake_max_jobs = read_make_machines(make_machines_name)) <= 0) {
				pmake_max_jobs = PMAKE_DEF_MAX_JOBS;
			}
		}
#ifdef DISTRIBUTED
		if (send_mtool_msgs) {
			send_rsrc_info_msg(pmake_max_jobs, local_host, user_name);
		}
#endif
	}

	if ((dmake_mode_type == serial_mode) ||
	    ((dmake_mode_type == parallel_mode) && (waitflg))) {
		return (execute_serial(line));
	}

#ifdef DISTRIBUTED
	if (dmake_mode_type == distributed_mode) {
		if(local) {
//			return (execute_serial(line));
			waitflg = true;
		}
		dmake_job_msg = new Avo_DoJobMsg();
		dmake_job_msg->setJobId(++job_msg_id);
		dmake_job_msg->setTarget(target->string_mb);
		dmake_job_msg->setImmediateOutput(0);
		called_make = false;
	} else
#endif
	{
		p = commands;
	}

	argcnt = 0;
	for (rule = line->body.line.command_used;
	     rule != NULL;
	     rule = rule->next) {
		if (posix && (touch || quest) && !rule->always_exec) {
			continue;
		}
		if (vpath_defined) {
			rule->command_line =
			  vpath_translation(rule->command_line);
		}
		if (dmake_mode_type == distributed_mode) {
			cmd_options = 0;
			if(local) {
				cmd_options |= local_host_mask;
			}
		} else {
			silent_flag = false;
			ignore = 0;
		}
		if (rule->command_line->hash.length > 0) {
			if (++argcnt == MAXRULES) {
				if (dmake_mode_type == distributed_mode) {
					/* XXX - tell rxm to execute on local host. */
					/* I WAS HERE!!! */
				} else {
					/* Too many rules, run serially instead. */
					return build_serial;
				}
			}
#ifdef DISTRIBUTED
			if (dmake_mode_type == distributed_mode) {
				/*
				 * XXX - set assign_mask to tell rxm
				 * to do the following.
				 */
/* From execute_serial():
				if (rule->assign) {
					result = build_ok;
					do_assign(rule->command_line, target);
 */
				if (0) {
				} else if (report_dependencies_level == 0) {
					if (rule->ignore_error) {
						cmd_options |= ignore_mask;
					}
					if (rule->silent) {
						cmd_options |= silent_mask;
					}
					if (rule->command_line->meta) {
						cmd_options |= meta_mask;
					}
					if (rule->make_refd) {
						cmd_options |= make_refd_mask;
					}
					if (do_not_exec_rule) {
						cmd_options |= do_not_exec_mask;
					}
					append_dmake_cmd(dmake_job_msg,
					                 rule->command_line->string_mb,
					                 cmd_options);
					/* Copying dosys()... */
					if (rule->make_refd) {
						if (waitflg) {
							dmake_job_msg->setImmediateOutput(1);
						}
						called_make = true;
						if (command_changed &&
						    !wrote_state_file) {
							write_state_file(0, false);
							wrote_state_file = true;
						}
					}
				}
			} else
#endif
			{
				if (rule->silent && !silent) {
					silent_flag = true;
				}
				if (rule->ignore_error) {
					ignore++;
				}
				/* XXX - need to add support for + prefix */
				if (silent_flag || ignore) {
					*p = getmem((silent_flag ? 1 : 0) +
						    ignore +
						    (strlen(rule->
						           command_line->
						           string_mb)) +
						    1);
					cp = *p++;
					if (silent_flag) {
						*cp++ = (int) at_char;
					}
					if (ignore) {
						*cp++ = (int) hyphen_char;
					}
					strcpy(cp, rule->command_line->string_mb);
				} else {
					*p++ = rule->command_line->string_mb;
				}
			}
		}
	}
	if ((argcnt == 0) ||
	    (report_dependencies_level > 0)) {
#ifdef DISTRIBUTED
		if (dmake_job_msg) {
			delete dmake_job_msg;
		}
#endif
		return build_ok;
	}
#ifdef DISTRIBUTED
	if (dmake_mode_type == distributed_mode) {
		// Send  a DoJob message to the rxm process.
		distribute_rxm(dmake_job_msg);

		// Wait for an acknowledgement.
		Avo_AcknowledgeMsg *ackMsg = getAcknowledgeMsg();
		if (ackMsg) {
			delete ackMsg;
		}

		if (waitflg) {
			// Wait for, and process a job result.
			result = await_dist(waitflg);
			if (called_make) {
				maybe_reread_make_state();
			}
			check_state(temp_file_name);
			if (result == build_failed) {
				if (!continue_after_error) {

#ifdef PRINT_EXIT_STATUS
					warning(NOCATGETS("I'm in execute_parallel. await_dist() returned build_failed"));
#endif

					fatal(sun_style ?
					"Command failed for target `%s'" :
					NULL,
					      target->string_mb);
				}
				/*
				 * Make sure a failing command is not
				 * saved in .make.state.
				 */
				line->body.line.command_used = NULL;
			}
			if (temp_file_name != NULL) {
				free_name(temp_file_name);
			}
			temp_file_name = NULL;
			Property spro = get_prop(sunpro_dependencies->prop, macro_prop);
			if(spro != NULL) {
				Name val = spro->body.macro.value;
				if(val != NULL) {
					free_name(val);
					spro->body.macro.value = NULL;
				}
			}
			spro = get_prop(sunpro_dependencies->prop, env_mem_prop);
			if(spro) {
				char *val = spro->body.env_mem.value;
				if(val != NULL) {
					retmem_mb(val);
					spro->body.env_mem.value = NULL;
				}
			}
			return result;
		} else {
			parallel_process_cnt++;
			return build_running;
		}
	} else
#endif
	{
		*p = NULL;

		Doname res = distribute_process(commands, line);
		if (res == build_running) {
			parallel_process_cnt++;
		}

		/*
		 * Return only those memory that were specially allocated
		 * for part of commands.
		 */
		for (int i = 0; commands[i] != NULL; i++) {
			if ((commands[i][0] == (int) at_char) ||
			    (commands[i][0] == (int) hyphen_char)) {
				retmem_mb(commands[i]);
			}
		}
		return res;
	}
}

#ifdef DISTRIBUTED
/*
 *	append_dmake_cmd()
 *
 *	Replaces all escaped newline's (\<cr>)
 *	  in the original command line with space's,
 *	  then append the new command line to the DoJobMsg object.
 */
static void
append_dmake_cmd(Avo_DoJobMsg *dmake_job_msg,
                 char *orig_cmd_line,
                 int cmd_options)
{
/*
	Avo_DmakeCommand	*tmp_dmake_command;

	tmp_dmake_command = new Avo_DmakeCommand(orig_cmd_line, cmd_options);
	dmake_job_msg->appendCmd(tmp_dmake_command);
	delete tmp_dmake_command;
 */
	dmake_job_msg->appendCmd(new Avo_DmakeCommand(orig_cmd_line, cmd_options));
}
#endif

#ifdef TEAMWARE_MAKE_CMN
#define MAXJOBS_ADJUST_RFE4694000

#ifdef MAXJOBS_ADJUST_RFE4694000

#include <unistd.h>	/* sysconf(_SC_NPROCESSORS_ONLN) */
#include <sys/ipc.h>		/* ftok() */
#include <sys/shm.h>		/* shmget(), shmat(), shmdt(), shmctl() */
#include <semaphore.h>		/* sem_init(), sem_trywait(), sem_post(), sem_destroy() */
#ifndef __sun
#define	LOADAVG_1MIN	0
#else
#include <sys/loadavg.h>	/* getloadavg() */
#endif /* !__sun */

/*
 *	adjust_pmake_max_jobs (int pmake_max_jobs)
 *
 *	Parameters:
 * 		pmake_max_jobs	- max jobs limit set by user
 *
 *	External functions used:
 *		sysconf()
 * 		getloadavg()
 */
static int
adjust_pmake_max_jobs (int pmake_max_jobs)
{
	static int	ncpu = 0;
	double		loadavg[3];
	int		adjustment;
	int		adjusted_max_jobs;

	if (ncpu <= 0) {
		if ((ncpu = sysconf(_SC_NPROCESSORS_ONLN)) <= 0) {
			ncpu = 1;
		}
	}
	if (getloadavg(loadavg, 3) != 3) return(pmake_max_jobs);
	adjustment = ((int)loadavg[LOADAVG_1MIN]);
	if (adjustment < 2) return(pmake_max_jobs);
	if (ncpu > 1) {
		adjustment = adjustment / ncpu;
	}
	adjusted_max_jobs = pmake_max_jobs - adjustment;
	if (adjusted_max_jobs < 1) adjusted_max_jobs = 1;
	return(adjusted_max_jobs);
}

/*
 *  M2 adjust mode data and functions
 *
 *  m2_init()		- initializes M2 shared semaphore
 *  m2_acquire_job()	- decrements M2 semaphore counter
 *  m2_release_job()	- increments M2 semaphore counter
 *  m2_fini()		- destroys M2 semaphore and shared memory*
 *
 *  Environment variables:
 *	__DMAKE_M2_FILE__
 *
 *  External functions:
 *	ftok(), shmget(), shmat(), shmdt(), shmctl()
 *	sem_init(), sem_trywait(), sem_post(), sem_destroy()
 *	creat(), close(), unlink()
 *	getenv(), putenv()
 *
 *  Static variables:
 *	m2_file		- tmp file name to create ipc key for shared memory
 *	m2_shm_id	- shared memory id
 *	m2_shm_sem	- shared memory semaphore
 */

static char	m2_file[MAXPATHLEN];
static int	m2_shm_id = -1;
static sem_t*	m2_shm_sem = 0;

static int
m2_init() {
	char	*var;
	key_t	key;

	if ((var = getenv(NOCATGETS("__DMAKE_M2_FILE__"))) == 0) {
		/* compose tmp file name */
		sprintf(m2_file, NOCATGETS("%s/dmake.m2.%d.XXXXXX"), tmpdir, getpid());

		/* create tmp file */
		int fd = mkstemp(m2_file);
		if (fd < 0) {
			return -1;
		} else {
			close(fd);
		}
	} else {
		/* using existing semaphore */
		strcpy(m2_file, var);
	}

	/* combine IPC key */
	if ((key = ftok(m2_file, 38)) == (key_t) -1) {
		return -1;
	}

	/* create shared memory */
	if ((m2_shm_id = shmget(key, sizeof(*m2_shm_sem), 0666 | (var ? 0 : IPC_CREAT|IPC_EXCL))) == -1) {
		return -1;
	}

	/* attach shared memory */
	if ((m2_shm_sem = (sem_t*) shmat(m2_shm_id, 0, 0666)) == (sem_t*)-1) {
		return -1;
	}

	/* root process */
	if (var == 0) {
		/* initialize semaphore */
		if (sem_init(m2_shm_sem, 1, pmake_max_jobs)) {
			return -1;
		}

		/* alloc memory for env variable */
		if ((var = (char*) malloc(MAXPATHLEN)) == 0) {
			return -1;
		}

		/* put key to env */
		sprintf(var, NOCATGETS("__DMAKE_M2_FILE__=%s"), m2_file);
		if (putenv(var)) {
			return -1;
		}
	}
	return 0;
}

static void
m2_fini() {
	if (m2_shm_id >= 0) {
		struct shmid_ds stat;

		/* determine the number of attached processes */
		if (shmctl(m2_shm_id, IPC_STAT, &stat) == 0) {
			if (stat.shm_nattch <= 1) {
				/* destroy semaphore */
				if (m2_shm_sem != 0) {
					sem_destroy(m2_shm_sem);
				}

				/* destroy shared memory */
				shmctl(m2_shm_id, IPC_RMID, &stat);

				/* remove tmp file created for the key */
				unlink(m2_file);
			} else {
				/* detach shared memory */
				if (m2_shm_sem != 0) {
					shmdt((char*) m2_shm_sem);
				}
			}
		}

		m2_shm_id = -1;
		m2_shm_sem = 0;
	}
}

static int
m2_acquire_job() {
	if ((m2_shm_id >= 0) && (m2_shm_sem != 0)) {
		if (sem_trywait(m2_shm_sem) == 0) {
			return 1;
		}
		if (errno == EAGAIN) {
			return 0;
		}
	}
	return -1;
}

static int
m2_release_job() {
	if ((m2_shm_id >= 0) && (m2_shm_sem != 0)) {
		if (sem_post(m2_shm_sem) == 0) {
			return 0;
		}
	}
	return -1;
}

/*
 *  job adjust mode
 *
 *  Possible values:
 *    ADJUST_M1		- adjustment by system load (default)
 *    ADJUST_M2		- fixed limit of jobs for the group of nested dmakes
 *    ADJUST_NONE	- no adjustment - fixed limit of jobs for the current dmake
 */
static enum {
	ADJUST_UNKNOWN,
	ADJUST_M1,
	ADJUST_M2,
	ADJUST_NONE
} job_adjust_mode = ADJUST_UNKNOWN;

/*
 *  void job_adjust_fini()
 *
 *  Description:
 *	Cleans up job adjust data.
 *
 *  Static variables:
 *	job_adjust_mode	Current job adjust mode
 */
void
job_adjust_fini() {
	if (job_adjust_mode == ADJUST_M2) {
		m2_fini();
	}
}

/*
 *  void job_adjust_error()
 *
 *  Description:
 *	Prints warning message, cleans up job adjust data, and disables job adjustment
 *
 *  Environment:
 *	DMAKE_ADJUST_MAX_JOBS
 *
 *  External functions:
 *	putenv()
 *
 *  Static variables:
 *	job_adjust_mode	Current job adjust mode
 */
static void
job_adjust_error() {
	if (job_adjust_mode != ADJUST_NONE) {
		/* cleanup internals */
		job_adjust_fini();

		/* warning message for the user */
		warning("Encountered max jobs auto adjustment error - disabling auto adjustment.");

		/* switch off job adjustment for the children */
		putenv(NOCATGETS("DMAKE_ADJUST_MAX_JOBS=NO"));

		/* and for this dmake */
		job_adjust_mode = ADJUST_NONE;
	}
}

/*
 *  void job_adjust_init()
 *
 *  Description:
 *	Parses DMAKE_ADJUST_MAX_JOBS env variable
 *	and performs appropriate initializations.
 *
 *  Environment:
 *	DMAKE_ADJUST_MAX_JOBS
 *	  DMAKE_ADJUST_MAX_JOBS == "NO"	- no adjustment
 *	  DMAKE_ADJUST_MAX_JOBS == "M2"	- M2 adjust mode
 *	  other				- M1 adjust mode
 *
 *  External functions:
 *	getenv()
 *
 *  Static variables:
 *	job_adjust_mode	Current job adjust mode
 */
static void
job_adjust_init() {
	if (job_adjust_mode == ADJUST_UNKNOWN) {
		/* default mode */
		job_adjust_mode = ADJUST_M1;

		/* determine adjust mode */
		if (char *var = getenv(NOCATGETS("DMAKE_ADJUST_MAX_JOBS"))) {
			if (strcasecmp(var, NOCATGETS("NO")) == 0) {
				job_adjust_mode = ADJUST_NONE;
			} else if (strcasecmp(var, NOCATGETS("M2")) == 0) {
				job_adjust_mode = ADJUST_M2;
			}
		}

		/* M2 specific initialization */
		if (job_adjust_mode == ADJUST_M2) {
			if (m2_init()) {
				job_adjust_error();
			}
		}
	}
}

#endif /* MAXJOBS_ADJUST_RFE4694000 */
#endif /* TEAMWARE_MAKE_CMN */

/*
 *	distribute_process(char **commands, Property line)
 *
 *	Parameters:
 *		commands	argv vector of commands to execute
 *
 *	Return value:
 *				The result of the execution
 *
 *	Static variables used:
 *		process_running	Set to the pid of the process set running
 * #if defined (TEAMWARE_MAKE_CMN) && defined (MAXJOBS_ADJUST_RFE4694000)
 *		job_adjust_mode	Current job adjust mode
 * #endif
 */
static Doname
distribute_process(char **commands, Property line)
{
#ifdef TEAMWARE_MAKE_CMN
	static unsigned	file_number = 0;
	wchar_t		string[MAXPATHLEN];
#endif
	char		mbstring[MAXPATHLEN];
#ifdef TEAMWARE_MAKE_CMN
	int		filed;
	int		res;
#endif
	int		tmp_index;
	char		*tmp_index_str_ptr;

#if !defined (TEAMWARE_MAKE_CMN) || !defined (MAXJOBS_ADJUST_RFE4694000)
	while (parallel_process_cnt >= pmake_max_jobs) {
		await_parallel(false);
		finish_children(true);
	}
#else /* TEAMWARE_MAKE_CMN && MAXJOBS_ADJUST_RFE4694000 */
	/* initialize adjust mode, if not initialized */
	if (job_adjust_mode == ADJUST_UNKNOWN) {
		job_adjust_init();
	}

	/* actions depend on adjust mode */
	switch (job_adjust_mode) {
	case ADJUST_M1:
		while (parallel_process_cnt >= adjust_pmake_max_jobs (pmake_max_jobs)) {
			await_parallel(false);
			finish_children(true);
		}
		break;
	case ADJUST_M2:
		if ((res = m2_acquire_job()) == 0) {
			if (parallel_process_cnt > 0) {
				await_parallel(false);
				finish_children(true);

				if ((res = m2_acquire_job()) == 0) {
					return build_serial;
				}
			} else {
				return build_serial;
			}
		}
		if (res < 0) {
			/* job adjustment error */
			job_adjust_error();

			/* no adjustment */
			while (parallel_process_cnt >= pmake_max_jobs) {
				await_parallel(false);
				finish_children(true);
			}
		}
		break;
	default:
		while (parallel_process_cnt >= pmake_max_jobs) {
			await_parallel(false);
			finish_children(true);
		}
	}
#endif /* TEAMWARE_MAKE_CMN && MAXJOBS_ADJUST_RFE4694000 */
#ifdef DISTRIBUTED
	if (send_mtool_msgs) {
		send_job_start_msg(line);
	}
#endif
#ifdef DISTRIBUTED
	setvar_envvar((Avo_DoJobMsg *)NULL);
#else
	setvar_envvar();
#endif
	/*
	 * Tell the user what DMake is doing.
	 */
#ifdef TEAMWARE_MAKE_CMN
	if (!silent && output_mode != txt2_mode) {
#else
	if (debug_level) {
#endif
		/*
		 * Print local_host --> x job(s).
		 */
		fprintf(stdout,
		               "%s --> %d %s\n",
		               local_host,
		               parallel_process_cnt + 1,
		               (parallel_process_cnt == 0) ? "job" : "jobs");

		/* Print command line(s). */
		tmp_index = 0;
		while (commands[tmp_index] != NULL) {
		    /* No @ char. */
		    /* XXX - need to add [2] when + prefix is added */
		    if ((commands[tmp_index][0] != (int) at_char) &&
		        (commands[tmp_index][1] != (int) at_char)) {
			tmp_index_str_ptr = commands[tmp_index];
			if (*tmp_index_str_ptr == (int) hyphen_char) {
				tmp_index_str_ptr++;
			}
                        fprintf(stdout, "%s\n", tmp_index_str_ptr);
		    }
		    tmp_index++;
		}
		fflush(stdout);
	}

#ifdef TEAMWARE_MAKE_CMN
	Bflag = true;
#endif
	if (Bflag) {
#ifdef TEAMWARE_MAKE_CMN
		sprintf(mbstring,
		        NOCATGETS("%s/%s.stdout.%d.%d.XXXXXX"),
			tmpdir,
		        getpid(),
	                file_number++);
#else
		sprintf(mbstring, NOCATGETS("%s/make.XXXXXX"), tmpdir);
#endif

		close(mkstemp(mbstring));

		stdout_file = strdup(mbstring);
	} else {
		stdout_file = NULL;
	}
	stderr_file = NULL;
#if defined (TEAMWARE_MAKE_CMN) && defined(REDIRECT_ERR)
	if (!out_err_same) {
		sprintf(mbstring,
			        NOCATGETS("%s/dmake.stderr.%d.%d.XXXXXX"),
				tmpdir,
			        getpid(),
		                file_number++);

		mktemp(mbstring);

		stderr_file = strdup(mbstring);
	}
#endif

#ifdef SGE_SUPPORT
	if (grid) {
		static char *dir4gridscripts = NULL;
		static char *hostName = NULL;
		if (dir4gridscripts == NULL) {
			Name		dmakeOdir_name, dmakeOdir_value;
			Property	prop;
			MBSTOWCS(wcs_buffer, NOCATGETS("DMAKE_ODIR"));
			dmakeOdir_name = GETNAME(wcs_buffer, FIND_LENGTH);
			if (((prop = get_prop(dmakeOdir_name->prop, macro_prop)) != NULL) &&
			    ((dmakeOdir_value = prop->body.macro.value) != NULL)) {
				dir4gridscripts = dmakeOdir_value->string_mb;
			}
			dir4gridscripts = Avo_PathNames::pathname_output_directory(dir4gridscripts);
			hostName = Avo_PathNames::pathname_local_host();
		}
		sprintf(script_file,
		        	NOCATGETS("%s/dmake.script.%s.%d.%d.XXXXXX"),
				dir4gridscripts,
				hostName,
			        getpid(),
		                file_number++);
	}
#endif /* SGE_SUPPORT */
	process_running = run_rule_commands(local_host, commands);

	return build_running;
}

/*
 *	doname_parallel(target, do_get, implicit)
 *
 *	Processes the given target and finishes up any parallel
 *	processes left running.
 *
 *	Return value:
 *				Result of target build
 *
 *	Parameters:
 *		target		Target to build
 *		do_get		True if sccs get to be done
 *		implicit	True if this is an implicit target
 */
Doname
doname_parallel(Name target, Boolean do_get, Boolean implicit)
{
	Doname		result;

	result = doname_check(target, do_get, implicit, false);
	if (result == build_ok || result == build_failed) {
		return result;
	}
	finish_running();
	return (Doname) target->state;
}

/*
 *	doname_subtree(target, do_get, implicit)
 *
 *	Completely computes an object and its dependents for a
 *	serial subtree build.
 *
 *	Parameters:
 *		target		Target to build
 *		do_get		True if sccs get to be done
 *		implicit	True if this is an implicit target
 *
 *	Static variables used:
 *		running_tail	Tail of the list of running processes
 *
 *	Global variables used:
 *		running_list	The list of running processes
 */
static void
doname_subtree(Name target, Boolean do_get, Boolean implicit)
{
	Running		save_running_list;
	Running		*save_running_tail;

	save_running_list = running_list;
	save_running_tail = running_tail;
	running_list = NULL;
	running_tail = &running_list;
	target->state = build_subtree;
	target->checking_subtree = true;
	while(doname_check(target, do_get, implicit, false) == build_running) {
		target->checking_subtree = false;
		finish_running();
		target->state = build_subtree;
	}
	target->checking_subtree = false;
	running_list = save_running_list;
	running_tail = save_running_tail;
}

/*
 *	finish_running()
 *
 *	Keeps processing until the running_list is emptied out.
 *
 *	Parameters:
 *
 *	Global variables used:
 *		running_list	The list of running processes
 */
void
finish_running(void)
{
	while (running_list != NULL) {
#ifdef DISTRIBUTED
		if (dmake_mode_type == distributed_mode) {
			if ((just_did_subtree) ||
			    (parallel_process_cnt == 0)) {
				just_did_subtree = false;
			} else {
				await_dist(false);
				finish_children(true);
			}
		} else
#endif
		{
			await_parallel(false);
			finish_children(true);
		}
		if (running_list != NULL) {
			process_next();
		}
	}
}

/*
 *	process_next()
 *
 *	Searches the running list for any targets which can start processing.
 *	This can be a pending target, a serial target, or a subtree target.
 *
 *	Parameters:
 *
 *	Static variables used:
 *		running_tail		The end of the list of running procs
 *		subtree_conflict	A target which conflicts with a subtree
 *		subtree_conflict2	The other target which conflicts
 *
 *	Global variables used:
 *		commands_done		True if commands executed
 *		debug_level		Controls debug output
 *		parallel_process_cnt	Number of parallel process running
 *		recursion_level		Indentation for debug output
 *		running_list		List of running processes
 */
static void
process_next(void)
{
	Running		rp;
	Running		*rp_prev;
	Property	line;
	Chain		target_group;
	Dependency	dep;
	Boolean		quiescent = true;
	Running		*subtree_target;
	Boolean		saved_commands_done;
	Property	*conditionals;

	subtree_target = NULL;
	subtree_conflict = NULL;
	subtree_conflict2 = NULL;
	/*
	 * If nothing currently running, build a serial target, if any.
	 */
start_loop_1:
	for (rp_prev = &running_list, rp = running_list;
	     rp != NULL && parallel_process_cnt == 0;
	     rp = rp->next) {
		if (rp->state == build_serial) {
			*rp_prev = rp->next;
			if (rp->next == NULL) {
				running_tail = rp_prev;
			}
			recursion_level = rp->recursion_level;
			rp->target->state = build_pending;
			doname_check(rp->target,
					    rp->do_get,
					    rp->implicit,
					    false);
			quiescent = false;
			delete_running_struct(rp);
			goto start_loop_1;
		} else {
			rp_prev = &rp->next;
		}
	}
	/*
	 * Find a target to build.  The target must be pending, have all
	 * its dependencies built, and not be in a target group with a target
	 * currently building.
	 */
start_loop_2:
	for (rp_prev = &running_list, rp = running_list;
	     rp != NULL;
	     rp = rp->next) {
		if (!(rp->state == build_pending ||
		      rp->state == build_subtree)) {
			quiescent = false;
			rp_prev = &rp->next;
		} else if (rp->state == build_pending) {
			line = get_prop(rp->target->prop, line_prop);
			for (dep = line->body.line.dependencies;
			     dep != NULL;
			     dep = dep->next) {
				if (dep->name->state == build_running ||
				    dep->name->state == build_pending ||
				    dep->name->state == build_serial) {
					break;
				}
			}
			if (dep == NULL) {
				for (target_group = line->body.line.target_group;
				     target_group != NULL;
				     target_group = target_group->next) {
					if (is_running(target_group->name)) {
						break;
					}
				}
				if (target_group == NULL) {
					*rp_prev = rp->next;
					if (rp->next == NULL) {
						running_tail = rp_prev;
					}
					recursion_level = rp->recursion_level;
					rp->target->state = rp->redo ?
					  build_dont_know : build_pending;
					saved_commands_done = commands_done;
					conditionals =
						set_conditionals
						    (rp->conditional_cnt,
						     rp->conditional_targets);
					rp->target->dont_activate_cond_values = true;
					if ((doname_check(rp->target,
							  rp->do_get,
							  rp->implicit,
							  rp->target->has_target_prop ? true : false) !=
					     build_running) &&
					    !commands_done) {
						commands_done =
						  saved_commands_done;
					}
					rp->target->dont_activate_cond_values = false;
					reset_conditionals
						(rp->conditional_cnt,
						 rp->conditional_targets,
						 conditionals);
					quiescent = false;
					delete_running_struct(rp);
					goto start_loop_2;
				} else {
					rp_prev = &rp->next;
				}
			} else {
				rp_prev = &rp->next;
			}
		} else {
			rp_prev = &rp->next;
		}
	}
	/*
	 * If nothing has been found to build and there exists a subtree
	 * target with no dependency conflicts, build it.
	 */
	if (quiescent) {
start_loop_3:
		for (rp_prev = &running_list, rp = running_list;
		     rp != NULL;
		     rp = rp->next) {
			if (rp->state == build_subtree) {
				if (!dependency_conflict(rp->target)) {
					*rp_prev = rp->next;
					if (rp->next == NULL) {
						running_tail = rp_prev;
					}
					recursion_level = rp->recursion_level;
					doname_subtree(rp->target,
						       rp->do_get,
						       rp->implicit);
#ifdef DISTRIBUTED
					just_did_subtree = true;
#endif
					quiescent = false;
					delete_running_struct(rp);
					goto start_loop_3;
				} else {
					subtree_target = rp_prev;
					rp_prev = &rp->next;
				}
			} else {
				rp_prev = &rp->next;
			}
		}
	}
	/*
	 * If still nothing found to build, we either have a deadlock
	 * or a subtree with a dependency conflict with something waiting
	 * to build.
	 */
	if (quiescent) {
		if (subtree_target == NULL) {
			fatal("Internal error: deadlock detected in process_next");
		} else {
			rp = *subtree_target;
			if (debug_level > 0) {
				warning("Conditional macro conflict encountered for %s between %s and %s",
					subtree_conflict2->string_mb,
					rp->target->string_mb,
					subtree_conflict->string_mb);
			}
			*subtree_target = (*subtree_target)->next;
			if (rp->next == NULL) {
				running_tail = subtree_target;
			}
			recursion_level = rp->recursion_level;
			doname_subtree(rp->target, rp->do_get, rp->implicit);
#ifdef DISTRIBUTED
			just_did_subtree = true;
#endif
			delete_running_struct(rp);
		}
	}
}

/*
 *	set_conditionals(cnt, targets)
 *
 *	Sets the conditional macros for the targets given in the array of
 *	targets.  The old macro values are returned in an array of
 *	Properties for later resetting.
 *
 *	Return value:
 *					Array of conditional macro settings
 *
 *	Parameters:
 *		cnt			Number of targets
 *		targets			Array of targets
 */
static Property *
set_conditionals(int cnt, Name *targets)
{
	Property	*locals, *lp;
	Name		*tp;

	locals = (Property *) getmem(cnt * sizeof(Property));
	for (lp = locals, tp = targets;
	     cnt > 0;
	     cnt--, lp++, tp++) {
		*lp = (Property) getmem((*tp)->conditional_cnt *
					sizeof(struct _Property));
		set_locals(*tp, *lp);
	}
	return locals;
}

/*
 *	reset_conditionals(cnt, targets, locals)
 *
 *	Resets the conditional macros as saved in the given array of
 *	Properties.  The resets are done in reverse order.  Afterwards the
 *	data structures are freed.
 *
 *	Parameters:
 *		cnt			Number of targets
 *		targets			Array of targets
 *		locals			Array of dependency macro settings
 */
static void
reset_conditionals(int cnt, Name *targets, Property *locals)
{
	Name		*tp;
	Property	*lp;

	for (tp = targets + (cnt - 1), lp = locals + (cnt - 1);
	     cnt > 0;
	     cnt--, tp--, lp--) {
		reset_locals(*tp,
			     *lp,
			     get_prop((*tp)->prop, conditional_prop),
			     0);
		retmem_mb((caddr_t) *lp);
	}
	retmem_mb((caddr_t) locals);
}

/*
 *	dependency_conflict(target)
 *
 *	Returns true if there is an intersection between
 *	the subtree of the target and any dependents of the pending targets.
 *
 *	Return value:
 *					True if conflict found
 *
 *	Parameters:
 *		target			Subtree target to check
 *
 *	Static variables used:
 *		subtree_conflict	Target conflict found
 *		subtree_conflict2	Second conflict found
 *
 *	Global variables used:
 *		running_list		List of running processes
 *		wait_name		.WAIT, not a real dependency
 */
static Boolean
dependency_conflict(Name target)
{
	Property	line;
	Property	pending_line;
	Dependency	dp;
	Dependency	pending_dp;
	Running		rp;

	/* Return if we are already checking this target */
	if (target->checking_subtree) {
		return false;
	}
	target->checking_subtree = true;
	line = get_prop(target->prop, line_prop);
	if (line == NULL) {
		target->checking_subtree = false;
		return false;
	}
	/* Check each dependency of the target for conflicts */
	for (dp = line->body.line.dependencies; dp != NULL; dp = dp->next) {
		/* Ignore .WAIT dependency */
		if (dp->name == wait_name) {
			continue;
		}
		/*
		 * For each pending target, look for a dependency which
		 * is the same as a dependency of the subtree target.  Since
		 * we can't build the subtree until all pending targets have
		 * finished which depend on the same dependency, this is
		 * a conflict.
		 */
		for (rp = running_list; rp != NULL; rp = rp->next) {
			if (rp->state == build_pending) {
				pending_line = get_prop(rp->target->prop,
							line_prop);
				if (pending_line == NULL) {
					continue;
				}
				for(pending_dp = pending_line->
				    			body.line.dependencies;
				    pending_dp != NULL;
				    pending_dp = pending_dp->next) {
					if (dp->name == pending_dp->name) {
						target->checking_subtree
						  		= false;
						subtree_conflict = rp->target;
						subtree_conflict2 = dp->name;
						return true;
					}
				}
			}
		}
		if (dependency_conflict(dp->name)) {
			target->checking_subtree = false;
			return true;
		}
	}
	target->checking_subtree = false;
	return false;
}

/*
 *	await_parallel(waitflg)
 *
 *	Waits for parallel children to exit and finishes their processing.
 *	If waitflg is false, the function returns after update_delay.
 *
 *	Parameters:
 *		waitflg		dwight
 */
void
await_parallel(Boolean waitflg)
{
#ifdef _CHECK_UPDATE_H
	static int number_of_unknown_children = 0;
#endif /* _CHECK_UPDATE_H */
	Boolean		nohang;
	pid_t		pid;
	int		status;
	Running		rp;
	int		waiterr;

	nohang = false;
	for ( ; ; ) {
		if (!nohang) {
			alarm((int) update_delay);
		}
		pid = waitpid((pid_t)-1,
			      &status,
			      nohang ? WNOHANG : 0);
		waiterr = errno;
		if (!nohang) {
			alarm(0);
		}
		if (pid <= 0) {
			if (waiterr == EINTR) {
				if (waitflg) {
					continue;
				} else {
					return;
				}
			} else {
				return;
			}
		}
		for (rp = running_list;
		     (rp != NULL) && (rp->pid != pid);
		     rp = rp->next) {
			;
		}
		if (rp == NULL) {
#ifdef _CHECK_UPDATE_H
			/* Ignore first child - it is check_update */
			if (number_of_unknown_children <= 0) {
				number_of_unknown_children = 1;
				return;
			}
#endif /* _CHECK_UPDATE_H */
			if (send_mtool_msgs) {
				continue;
			} else {
				fatal("Internal error: returned child pid not in running_list");
			}
		} else {
			rp->state = (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? build_ok : build_failed;
		}
		nohang = true;
		parallel_process_cnt--;

#if defined (TEAMWARE_MAKE_CMN) && defined (MAXJOBS_ADJUST_RFE4694000)
		if (job_adjust_mode == ADJUST_M2) {
			if (m2_release_job()) {
				job_adjust_error();
			}
		}
#endif
	}
}

/*
 *	finish_children(docheck)
 *
 *	Finishes the processing for all targets which were running
 *	and have now completed.
 *
 *	Parameters:
 *		docheck		Completely check the finished target
 *
 *	Static variables used:
 *		running_tail	The tail of the running list
 *
 *	Global variables used:
 *		continue_after_error  -k flag
 *		fatal_in_progress  True if we are finishing up after fatal err
 *		running_list	List of running processes
 */
void
finish_children(Boolean docheck)
{
	int		cmds_length;
	Cmd_line	rule;
	struct stat	out_buf;
	Boolean		silent_flag;
	Property	line;
	Property	line2;
	Running		rp;
	Running		*rp_prev;

	for (rp_prev = &running_list, rp = running_list;
	     rp != NULL;
	     rp = rp->next) {
bypass_for_loop_inc_4:
		/*
		 * If the state is ok or failed, then this target has
		 * finished building.
		 * In parallel_mode, output the accumulated stdout/stderr.
		 * Read the auto dependency stuff, handle a failed build,
		 * update the target, then finish the doname process for
		 * that target.
		 */
		if (rp->state == build_ok || rp->state == build_failed) {
			*rp_prev = rp->next;
			if (rp->next == NULL) {
				running_tail = rp_prev;
			}
			if ((line2 = rp->command) == NULL) {
				line2 = get_prop(rp->target->prop, line_prop);
			}
			if (dmake_mode_type == distributed_mode) {
				if (rp->make_refd) {
					maybe_reread_make_state();
				}
			} else {
				/*
				 * Send an Avo_MToolJobResultMsg to maketool.
				 */
#ifdef DISTRIBUTED
				if (send_mtool_msgs) {
					send_job_result_msg(rp);
				}
#endif
				/*
				 * Check if there were any job output
				 * from the parallel build.
				 */
				if (rp->stdout_file != NULL) {
					if (stat(rp->stdout_file, &out_buf) < 0) {
						fatal("stat of %s failed: %s",
						      rp->stdout_file,
						      errmsg(errno));
					}
					if ((line2 != NULL) &&
					    (out_buf.st_size > 0)) {
						cmds_length = 0;
						for (rule = line2->body.line.command_used,
						     silent_flag = silent;
						     rule != NULL;
						     rule = rule->next) {
							cmds_length += rule->command_line->hash.length + 1;
							silent_flag = BOOLEAN(silent_flag || rule->silent);
						}
						if (out_buf.st_size != cmds_length || silent_flag ||
						    output_mode == txt2_mode) {
							dump_out_file(rp->stdout_file, false);
						}
					}
					unlink(rp->stdout_file);
					retmem_mb(rp->stdout_file);
					rp->stdout_file = NULL;
				}
#if defined(REDIRECT_ERR)
				if (!out_err_same && (rp->stderr_file != NULL)) {
					if (stat(rp->stderr_file, &out_buf) < 0) {
						fatal("stat of %s failed: %s",
						      rp->stderr_file,
						      errmsg(errno));
					}
					if ((line2 != NULL) &&
					    (out_buf.st_size > 0)) {
						dump_out_file(rp->stderr_file, true);
					}
					unlink(rp->stderr_file);
					retmem_mb(rp->stderr_file);
					rp->stderr_file = NULL;
				}
#endif
			}
			check_state(rp->temp_file);
			if (rp->temp_file != NULL) {
				free_name(rp->temp_file);
			}
			rp->temp_file = NULL;
			if (rp->state == build_failed) {
				line = get_prop(rp->target->prop, line_prop);
				if (line != NULL) {
					line->body.line.command_used = NULL;
				}
				if (continue_after_error ||
				    fatal_in_progress ||
				    !docheck) {
					warning(sun_style ?
					"Command failed for target `%s'" :
					NULL,
						rp->command ? line2->body.line.target->string_mb : rp->target->string_mb);
					build_failed_seen = true;
				} else {
					/*
					 * XXX??? - DMake needs to exit(),
					 * but shouldn't call fatal().
					 */
#ifdef PRINT_EXIT_STATUS
					warning(NOCATGETS("I'm in finish_children. rp->state == build_failed."));
#endif

					fatal(sun_style ?
					"Command failed for target `%s'" :
					NULL,
						rp->command ? line2->body.line.target->string_mb : rp->target->string_mb);
				}
			}
			if (!docheck) {
				delete_running_struct(rp);
				rp = *rp_prev;
				if (rp == NULL) {
					break;
				} else {
					goto bypass_for_loop_inc_4;
				}
			}
			update_target(get_prop(rp->target->prop, line_prop),
				      rp->state);
			finish_doname(rp);
			delete_running_struct(rp);
			rp = *rp_prev;
			if (rp == NULL) {
				break;
			} else {
				goto bypass_for_loop_inc_4;
			}
		} else {
			rp_prev = &rp->next;
		}
	}
}

/*
 *	dump_out_file(filename, err)
 *
 *	Write the contents of the file to stdout, then unlink the file.
 *
 *	Parameters:
 *		filename	Name of temp file containing output
 *
 *	Global variables used:
 */
static void
dump_out_file(char *filename, Boolean err)
{
	int		chars_read;
	char		copybuf[BUFSIZ];
	int		fd;
	int		out_fd = (err ? 2 : 1);

	if ((fd = open(filename, O_RDONLY)) < 0) {
		fatal("open failed for output file %s: %s",
		      filename,
		      errmsg(errno));
	}
	if (!silent && output_mode != txt2_mode && sun_style) {
		fprintf(err ? stderr : stdout,
		               err ?
				"%s --> Job errors\n" :
				"%s --> Job output\n",
		               local_host);
		fflush(err ? stderr : stdout);
	}
	for (chars_read = read(fd, copybuf, BUFSIZ);
	     chars_read > 0;
	     chars_read = read(fd, copybuf, BUFSIZ)) {
		/*
		 * Read buffers from the source file until end or error.
		 */
		if (write(out_fd, copybuf, chars_read) < 0) {
			fatal("write failed for output file %s: %s",
			      filename,
			      errmsg(errno));
		}
	}
	close(fd);
	unlink(filename);
}

/*
 *	finish_doname(rp)
 *
 *	Completes the processing for a target which was left running.
 *
 *	Parameters:
 *		rp		Running list entry for target
 *
 *	Global variables used:
 *		debug_level	Debug flag
 *		recursion_level	Indentation for debug output
 */
static void
finish_doname(Running rp)
{
	int		auto_count = rp->auto_count;
	Name		*automatics = rp->automatics;
	Doname		result = rp->state;
	Name		target = rp->target;
	Name		true_target = rp->true_target;
	Property	*conditionals;

	recursion_level = rp->recursion_level;
	if (result == build_ok) {
		if (true_target == NULL) {
			printf(NOCATGETS("Target = %s\n"), target->string_mb);
			printf(NOCATGETS(" State = %d\n"), result);
			fatal(NOCATGETS("Internal error: NULL true_target in finish_doname"));
		}
		/* If all went OK, set a nice timestamp */
		if (true_target->stat.time == file_doesnt_exist) {
			true_target->stat.time = file_max_time;
		}
	}
	target->state = result;
	if (target->is_member) {
		Property member;

		/* Propagate the timestamp from the member file to the member */
		if ((target->stat.time != file_max_time) &&
		    ((member = get_prop(target->prop, member_prop)) != NULL) &&
		    (exists(member->body.member.member) > file_doesnt_exist)) {
			target->stat.time =
/*
			  exists(member->body.member.member);
 */
			  member->body.member.member->stat.time;
		}
	}
	/*
	 * Check if we found any new auto dependencies when we
	 * built the target.
	 */
	if ((result == build_ok) && check_auto_dependencies(target,
							    auto_count,
							    automatics)) {
		if (debug_level > 0) {
			printf("%*sTarget `%s' acquired new dependencies from build, checking all dependencies\n",
				      recursion_level,
				      "",
				      true_target->string_mb);
		}
		target->rechecking_target = true;
		target->state = build_running;

		/* [tolik, Tue Mar 25 1997]
		 * Fix for bug 4038824:
		 *       command line options set by conditional macros get dropped
		 * rp->conditional_cnt and rp->conditional_targets must be copied
		 * to new 'rp' during add_pending(). Set_conditionals() stores
		 * rp->conditional_targets to the global variable 'conditional_targets'
		 * Add_pending() will use this variable to set up 'rp'.
		 */
		conditionals = set_conditionals(rp->conditional_cnt, rp->conditional_targets);
		add_pending(target,
			    recursion_level,
			    rp->do_get,
			    rp->implicit,
			    true);
		reset_conditionals(rp->conditional_cnt, rp->conditional_targets, conditionals);
	}
}

/*
 *	new_running_struct()
 *
 *	Constructor for Running struct. Creates a structure and initializes
 *      its fields.
 *
 */
static Running new_running_struct()
{
	Running		rp;

	rp = ALLOC(Running);
	rp->target = NULL;
	rp->true_target = NULL;
	rp->command = NULL;
	rp->sprodep_value = NULL;
	rp->sprodep_env = NULL;
	rp->auto_count = 0;
	rp->automatics = NULL;
	rp->pid = -1;
	rp->job_msg_id = -1;
	rp->stdout_file = NULL;
	rp->stderr_file = NULL;
	rp->temp_file = NULL;
	rp->next = NULL;
	return rp;
}

/*
 *	add_running(target, true_target, command, recursion_level, auto_count,
 *					automatics, do_get, implicit)
 *
 *	Adds a record on the running list for this target, which
 *	was just spawned and is running.
 *
 *	Parameters:
 *		target		Target being built
 *		true_target	True target for target
 *		command		Running command.
 *		recursion_level	Debug indentation level
 *		auto_count	Count of automatic dependencies
 *		automatics	List of automatic dependencies
 *		do_get		Sccs get flag
 *		implicit	Implicit flag
 *
 *	Static variables used:
 *		running_tail	Tail of running list
 *		process_running	PID of process
 *
 *	Global variables used:
 *		current_line	Current line for target
 *		current_target	Current target being built
 *		stderr_file	Temporary file for stdout
 *		stdout_file	Temporary file for stdout
 *		temp_file_name	Temporary file for auto dependencies
 */
void
add_running(Name target, Name true_target, Property command, int recursion_level, int auto_count, Name *automatics, Boolean do_get, Boolean implicit)
{
	Running		rp;
	Name		*p;

	rp = new_running_struct();
	rp->state = build_running;
	rp->target = target;
	rp->true_target = true_target;
	rp->command = command;
	Property spro_val = get_prop(sunpro_dependencies->prop, macro_prop);
	if(spro_val) {
		rp->sprodep_value = spro_val->body.macro.value;
		spro_val->body.macro.value = NULL;
		spro_val = get_prop(sunpro_dependencies->prop, env_mem_prop);
		if(spro_val) {
			rp->sprodep_env = spro_val->body.env_mem.value;
			spro_val->body.env_mem.value = NULL;
		}
	}
	rp->recursion_level = recursion_level;
	rp->do_get = do_get;
	rp->implicit = implicit;
	rp->auto_count = auto_count;
	if (auto_count > 0) {
		rp->automatics = (Name *) getmem(auto_count * sizeof (Name));
		for (p = rp->automatics; auto_count > 0; auto_count--) {
			*p++ = *automatics++;
		}
	} else {
		rp->automatics = NULL;
	}
#ifdef DISTRIBUTED
	if (dmake_mode_type == distributed_mode) {
		rp->make_refd = called_make;
		called_make = false;
	} else
#endif
	{
		rp->pid = process_running;
		process_running = -1;
		childPid = -1;
	}
	rp->job_msg_id = job_msg_id;
	rp->stdout_file = stdout_file;
	rp->stderr_file = stderr_file;
	rp->temp_file = temp_file_name;
	rp->redo = false;
	rp->next = NULL;
	store_conditionals(rp);
	stdout_file = NULL;
	stderr_file = NULL;
	temp_file_name = NULL;
	current_target = NULL;
	current_line = NULL;
	*running_tail = rp;
	running_tail = &rp->next;
}

/*
 *	add_pending(target, recursion_level, do_get, implicit, redo)
 *
 *	Adds a record on the running list for a pending target
 *	(waiting for its dependents to finish running).
 *
 *	Parameters:
 *		target		Target being built
 *		recursion_level	Debug indentation level
 *		do_get		Sccs get flag
 *		implicit	Implicit flag
 *		redo		True if this target is being redone
 *
 *	Static variables used:
 *		running_tail	Tail of running list
 */
void
add_pending(Name target, int recursion_level, Boolean do_get, Boolean implicit, Boolean redo)
{
	Running		rp;
	rp = new_running_struct();
	rp->state = build_pending;
	rp->target = target;
	rp->recursion_level = recursion_level;
	rp->do_get = do_get;
	rp->implicit = implicit;
	rp->redo = redo;
	store_conditionals(rp);
	*running_tail = rp;
	running_tail = &rp->next;
}

/*
 *	add_serial(target, recursion_level, do_get, implicit)
 *
 *	Adds a record on the running list for a target which must be
 *	executed in serial after others have finished.
 *
 *	Parameters:
 *		target		Target being built
 *		recursion_level	Debug indentation level
 *		do_get		Sccs get flag
 *		implicit	Implicit flag
 *
 *	Static variables used:
 *		running_tail	Tail of running list
 */
void
add_serial(Name target, int recursion_level, Boolean do_get, Boolean implicit)
{
	Running		rp;

	rp = new_running_struct();
	rp->target = target;
	rp->recursion_level = recursion_level;
	rp->do_get = do_get;
	rp->implicit = implicit;
	rp->state = build_serial;
	rp->redo = false;
	store_conditionals(rp);
	*running_tail = rp;
	running_tail = &rp->next;
}

/*
 *	add_subtree(target, recursion_level, do_get, implicit)
 *
 *	Adds a record on the running list for a target which must be
 *	executed in isolation after others have finished.
 *
 *	Parameters:
 *		target		Target being built
 *		recursion_level	Debug indentation level
 *		do_get		Sccs get flag
 *		implicit	Implicit flag
 *
 *	Static variables used:
 *		running_tail	Tail of running list
 */
void
add_subtree(Name target, int recursion_level, Boolean do_get, Boolean implicit)
{
	Running		rp;

	rp = new_running_struct();
	rp->target = target;
	rp->recursion_level = recursion_level;
	rp->do_get = do_get;
	rp->implicit = implicit;
	rp->state = build_subtree;
	rp->redo = false;
	store_conditionals(rp);
	*running_tail = rp;
	running_tail = &rp->next;
}

/*
 *	store_conditionals(rp)
 *
 *	Creates an array of the currently active targets with conditional
 *	macros (found in the chain conditional_targets) and puts that
 *	array in the Running struct.
 *
 *	Parameters:
 *		rp		Running struct for storing chain
 *
 *	Global variables used:
 *		conditional_targets  Chain of current dynamic conditionals
 */
static void
store_conditionals(Running rp)
{
	int		cnt;
	Chain		cond_name;

	if (conditional_targets == NULL) {
		rp->conditional_cnt = 0;
		rp->conditional_targets = NULL;
		return;
	}
	cnt = 0;
	for (cond_name = conditional_targets;
	     cond_name != NULL;
	     cond_name = cond_name->next) {
		cnt++;
	}
	rp->conditional_cnt = cnt;
	rp->conditional_targets = (Name *) getmem(cnt * sizeof(Name));
	for (cond_name = conditional_targets;
	     cond_name != NULL;
	     cond_name = cond_name->next) {
		rp->conditional_targets[--cnt] = cond_name->name;
	}
}

/*
 *	parallel_ok(target, line_prop_must_exists)
 *
 *	Returns true if the target can be run in parallel
 *
 *	Return value:
 *				True if can run in parallel
 *
 *	Parameters:
 *		target		Target being tested
 *
 *	Global variables used:
 *		all_parallel	True if all targets default to parallel
 *		only_parallel	True if no targets default to parallel
 */
Boolean
parallel_ok(Name target, Boolean line_prop_must_exists)
{
	Boolean		assign;
	Boolean		make_refd;
	Property	line;
	Cmd_line	rule;

	assign = make_refd = false;
	if (((line = get_prop(target->prop, line_prop)) == NULL) &&
	    line_prop_must_exists) {
		return false;
	}
	if (line != NULL) {
		for (rule = line->body.line.command_used;
		     rule != NULL;
		     rule = rule->next) {
			if (rule->assign) {
				assign = true;
			} else if (rule->make_refd) {
				make_refd = true;
			}
		}
	}
	if (assign) {
		return false;
	}
	if (target->nmutexes) {
		int	i;
		Name	*np;
		for (i = 0; i < target->nmutexes; i++) {
			for (np = mutexlist[target->mutexes[i]];
					*np != NULL; np++) {
				if (is_running(*np)) {
					if (debug_level)
						fprintf(stdout,
						   "Serializing %s because "
						   "%s is running\n",
					           target->string_mb,
					           (*np)->string_mb);
					return false;
				}
			}
		}
	}
	if (target->parallel) {
		return true;
	} else if (target->no_parallel) {
		return false;
	} if (all_parallel) {
		return true;
	} else if (only_parallel) {
		return false;
	} else if (make_refd) {
		return false;
	} else {
		return true;
	}
}

/*
 *	is_running(target)
 *
 *	Returns true if the target is running.
 *
 *	Return value:
 *				True if target is running
 *
 *	Parameters:
 *		target		Target to check
 *
 *	Global variables used:
 *		running_list	List of running processes
 */
Boolean
is_running(Name target)
{
	Running		rp;

	if (target->state != build_running) {
		return false;
	}
	for (rp = running_list;
	     rp != NULL && target != rp->target;
	     rp = rp->next);
	if (rp == NULL) {
		return false;
	} else {
		return (rp->state == build_running) ? true : false;
	}
}

/*
 * This function replaces the makesh binary.
 */
 
#ifdef SGE_SUPPORT
#define DO_CHECK(f)	if (f <= 0) { \
				fprintf(stderr, \
					"Could not write to file: %s: %s\n", \
						script_file, errmsg(errno)); \
				_exit(1); \
			}
#endif /* SGE_SUPPORT */

static pid_t
run_rule_commands(char *host, char **commands)
{
	Boolean		always_exec;
	Name		command;
	Boolean		ignore;
	int		length;
	Doname		result;
	Boolean		silent_flag;
#ifdef SGE_SUPPORT
	wchar_t		*wcmd, *tmp_wcs_buffer = NULL;
	char		*cmd, *tmp_mbs_buffer = NULL;
	FILE		*scrfp;
	Name		shell = getvar(shell_name);
#else
	wchar_t		*tmp_wcs_buffer;
#endif /* SGE_SUPPORT */

	childPid = fork();
	switch (childPid) {
	case -1:	/* Error */
		fatal("Could not fork child process for dmake job: %s",
		      errmsg(errno));
		break;
	case 0:		/* Child */
		/* To control the processed targets list is not the child's business */
		running_list = NULL;
#if defined(REDIRECT_ERR)
		if(out_err_same) {
			redirect_io(stdout_file, (char*)NULL);
		} else {
			redirect_io(stdout_file, stderr_file);
		}
#else
		redirect_io(stdout_file, (char*)NULL);
#endif
#ifdef SGE_SUPPORT
		if (grid) {
			int fdes = mkstemp(script_file);
			if ((fdes < 0) || (scrfp = fdopen(fdes, "w")) == NULL) {
				fprintf(stderr,
					"Could not create file: %s: %s\n",
					script_file, errmsg(errno));
				_exit(1);
			}
			if (IS_EQUAL(shell->string_mb, "")) {
				shell = shell_name;
			}
		}
#endif /* SGE_SUPPORT */
		for (commands = commands;
		     (*commands != (char *)NULL);
		     commands++) {
			silent_flag = silent;
			ignore = false;
			always_exec = false;
			while ((**commands == (int) at_char) ||
			       (**commands == (int) hyphen_char) ||
			       (**commands == (int) plus_char)) {
				if (**commands == (int) at_char) {
					silent_flag = true;
				}
				if (**commands == (int) hyphen_char) {
					ignore = true;
				}
				if (**commands == (int) plus_char) {
					always_exec = true;
				}
				(*commands)++;
			}
#ifdef SGE_SUPPORT
			if (grid) {
				if ((length = strlen(*commands)) >= MAXPATHLEN / 2) {
					wcmd = tmp_wcs_buffer = ALLOC_WC(length * 2 + 1);
					mbstowcs(tmp_wcs_buffer, *commands, length * 2 + 1);
				} else {
					MBSTOWCS(wcs_buffer, *commands);
					wcmd = wcs_buffer;
					cmd = mbs_buffer;
				}
				wchar_t	*from = wcmd + wslen(wcmd);
				wchar_t	*to = from + (from - wcmd);
				*to = (int) nul_char;
				while (from > wcmd) {
					*--to = *--from;
					if (*from == (int) newline_char) { // newline symbols are already quoted
						*--to = *--from;
					} else if (wschr(char_semantics_char, *from)) {
						*--to = (int) backslash_char;
					}
				}
				if (length >= MAXPATHLEN*MB_LEN_MAX/2) { // sizeof(mbs_buffer) / 2
					cmd = tmp_mbs_buffer = getmem((length * MB_LEN_MAX * 2) + 1);
					wcstombs(tmp_mbs_buffer, to, (length * MB_LEN_MAX * 2) + 1);
				} else {
					WCSTOMBS(mbs_buffer, to);
					cmd = mbs_buffer;
				}
				char *mbst, *mbend;
				if ((length > 0) &&
				    !silent_flag) {
				    	for (mbst = cmd; (mbend = strstr(mbst, "\\\n")) != NULL; mbst = mbend + 2) {
						*mbend = '\0';
						DO_CHECK(fprintf(scrfp, NOCATGETS("/usr/bin/printf '%%s\\n' %s\\\\\n"), mbst));
						*mbend = '\\';
					}
					DO_CHECK(fprintf(scrfp, NOCATGETS("/usr/bin/printf '%%s\\n' %s\n"), mbst));
				}
				if (!do_not_exec_rule ||
				    !working_on_targets ||
				    always_exec) {
#ifndef __sun
					if (0 != strcmp(shell->string_mb, (char*)NOCATGETS("/bin/sh"))) {
						DO_CHECK(fprintf(scrfp, NOCATGETS("%s -c %s\n"), shell->string_mb, cmd));
					} else
#endif
					DO_CHECK(fprintf(scrfp, NOCATGETS("%s -ce %s\n"), shell->string_mb, cmd));
					DO_CHECK(fputs(NOCATGETS("__DMAKECMDEXITSTAT=$?\nif [ ${__DMAKECMDEXITSTAT} -ne 0 ]; then\n"), scrfp));
					if (ignore) {
						DO_CHECK(fprintf(scrfp, NOCATGETS("\techo %s ${__DMAKECMDEXITSTAT} %s\n"),
							"\"*** Error code",
							"(ignored)\""));
					} else {
						DO_CHECK(fprintf(scrfp, NOCATGETS("\techo %s ${__DMAKECMDEXITSTAT}\n"),
							"\"*** Error code\""));
					}
					if (silent_flag) {
						DO_CHECK(fprintf(scrfp, NOCATGETS("\techo %s\n"),
							"The following command caused the error:"));
				 	   	for (mbst = cmd; (mbend = strstr(mbst, "\\\n")) != NULL; mbst = mbend + 2) {
							*mbend = '\0';
							DO_CHECK(fprintf(scrfp, NOCATGETS("\t/usr/bin/printf '%%s\\n' %s\\\\\n"), mbst));
							*mbend = '\\';
						}
						DO_CHECK(fprintf(scrfp, NOCATGETS("\t/usr/bin/printf '%%s\\n' %s\n"), mbst));
					}
					if (!ignore) {
						DO_CHECK(fputs(NOCATGETS("\texit ${__DMAKECMDEXITSTAT}\n"), scrfp));
					}
					DO_CHECK(fputs(NOCATGETS("fi\n"), scrfp));
				}
				if (tmp_wcs_buffer) {
					retmem_mb(tmp_mbs_buffer);
					tmp_mbs_buffer = NULL;
				}
				if (tmp_wcs_buffer) {
					retmem(tmp_wcs_buffer);
					tmp_wcs_buffer = NULL;
				}
				continue;
			}
#endif /* SGE_SUPPORT */
			if ((length = strlen(*commands)) >= MAXPATHLEN) {
				tmp_wcs_buffer = ALLOC_WC(length + 1);
				mbstowcs(tmp_wcs_buffer, *commands, length + 1);
				command = GETNAME(tmp_wcs_buffer, FIND_LENGTH);
				retmem(tmp_wcs_buffer);
			} else {
				MBSTOWCS(wcs_buffer, *commands);
				command = GETNAME(wcs_buffer, FIND_LENGTH);
			}
			if ((command->hash.length > 0) &&
			    !silent_flag) {
				print_command(command->string_mb);
			}
			result = dosys(command,
			               ignore,
			               false,
			               false, /* bugs #4085164 & #4990057 */
			               /* BOOLEAN(silent_flag && ignore), */
			               always_exec, 
			               (Name) NULL,
			               false);
			if (result == build_failed) {
				if (silent_flag) {
					printf("The following command caused the error:\n%s\n", command->string_mb);
				}
				if (!ignore) {
					_exit(1);
				}
			}
		}
#ifndef SGE_SUPPORT
		_exit(0);
#else
		if (!grid) {
			_exit(0);
		}
		DO_CHECK(fputs(NOCATGETS("exit 0\n"), scrfp));
		if (fclose(scrfp) != 0) {
			fprintf(stderr,
				"Could not close file: %s: %s\n",
				script_file, errmsg(errno));
			_exit(1);
		}
		{

#define DEFAULT_QRSH_TRIES_NUMBER	1
#define DEFAULT_QRSH_TIMEOUT		0

		static char	*sge_env_var = NULL;
		static int	qrsh_tries_number = DEFAULT_QRSH_TRIES_NUMBER;
		static int	qrsh_timeout = DEFAULT_QRSH_TIMEOUT;
#define SGE_DEBUG
#ifdef SGE_DEBUG
		static	Boolean do_not_remove = false;
#endif /* SGE_DEBUG */
		if (sge_env_var == NULL) {
			sge_env_var = getenv(NOCATGETS("__SPRO_DMAKE_SGE_TRIES"));
			if (sge_env_var != NULL) {
				qrsh_tries_number = atoi(sge_env_var);
				if (qrsh_tries_number < 1 || qrsh_tries_number > 9) {
					qrsh_tries_number = DEFAULT_QRSH_TRIES_NUMBER;
				}
			}
			sge_env_var = getenv(NOCATGETS("__SPRO_DMAKE_SGE_TIMEOUT"));
			if (sge_env_var != NULL) {
				qrsh_timeout = atoi(sge_env_var);
				if (qrsh_timeout <= 0) {
					qrsh_timeout = DEFAULT_QRSH_TIMEOUT;
				}
			} else {
				sge_env_var = "";
			}
#ifdef SGE_DEBUG
			sge_env_var = getenv(NOCATGETS("__SPRO_DMAKE_SGE_DEBUG"));
			if (sge_env_var == NULL) {
				sge_env_var = "";
			}
			if (strstr(sge_env_var, NOCATGETS("noqrsh")) != NULL)
				qrsh_tries_number = 0;
			if (strstr(sge_env_var, NOCATGETS("donotremove")) != NULL)
				do_not_remove = true;
#endif /* SGE_DEBUG */
		}
		for (int i = qrsh_tries_number; ; i--)
		if ((childPid = fork()) < 0) {
			fatal("Could not fork child process for qrsh job: %s",
		      		errmsg(errno));
			_exit(1);
		} else if (childPid == 0) {
			enable_interrupt((void (*) (int))SIG_DFL);
			if (i > 0) {
				static char qrsh_cmd[50+MAXPATHLEN] = NOCATGETS("qrsh -cwd -V -noshell -nostdin /bin/sh ");
				static char *fname_ptr = NULL;
				static char *argv[] = { NOCATGETS("sh"),
							NOCATGETS("-fce"),
							qrsh_cmd,
							NULL};
				if (fname_ptr == NULL) {
					fname_ptr = qrsh_cmd + strlen(qrsh_cmd);
				}
				strcpy(fname_ptr, script_file);
				execve(NOCATGETS("/bin/sh"), argv, environ);
			} else {
				static char *argv[] = { NOCATGETS("sh"),
							script_file,
							NULL};
				execve(NOCATGETS("/bin/sh"), argv, environ);
			}
			fprintf(stderr,
				"Could not load `qrsh': %s\n",
				errmsg(errno));
			_exit(1);
		} else {
			int		status;
			pid_t pid;
			while ((pid = wait(&status)) != childPid) {
				if (pid == -1) {
					fprintf(stderr,
						"wait() failed: %s\n",
						errmsg(errno));
					_exit(1);
				}
			}
			if (status != 0 && i > 0) {
				if (i > 1) {
					sleep(qrsh_timeout);
				}
				continue;
			}
#ifdef SGE_DEBUG
			if (do_not_remove) {
				if (status) {
					fprintf(stderr,
						NOCATGETS("SGE script failed: %s\n"),
						script_file);
				}
				_exit(status ? 1 : 0);
			}
#endif /* SGE_DEBUG */
			unlink(script_file);
			_exit(status ? 1 : 0);
		}
		}
#endif /* SGE_SUPPORT */
		break;
	default:
		break;
	}
	return childPid;
}

static void
maybe_reread_make_state(void)
{
	/* Copying dosys()... */
	if (report_dependencies_level == 0) {
		make_state->stat.time = file_no_time;
		exists(make_state);
		if (make_state_before == make_state->stat.time) {
			return;
		}
		makefile_type = reading_statefile;
		if (read_trace_level > 1) {
			trace_reader = true;
		}
		temp_file_number++;
		read_simple_file(make_state,
					false,
					false,
					false,
					false,
					false,
					true);
		trace_reader = false;
	}
}

#ifdef DISTRIBUTED
/*
 * Create and send an Avo_MToolJobResultMsg.
 */
static void
send_job_result_msg(Running rp)
{
	Avo_MToolJobResultMsg	*msg;
	RWCollectable		*xdr_msg;

	msg = new Avo_MToolJobResultMsg();
	msg->setResult(rp->job_msg_id,
	               (rp->state == build_ok) ? 0 : 1,
	               DONE);
	append_job_result_msg(msg,
				rp->stdout_file,
				rp->stderr_file);

	xdr_msg = (RWCollectable *)msg;
	xdr(get_xdrs_ptr(), xdr_msg);
	fflush(get_mtool_msgs_fp());

	delete msg;
}

/*
 * Append the stdout/err to Avo_MToolJobResultMsg.
 */
static void
append_job_result_msg(Avo_MToolJobResultMsg *msg, char *outFn, char *errFn)
{
	FILE		*fp;
	char		line[MAXPATHLEN];

	fp = fopen(outFn, "r");
	if (fp == NULL) {
		/* Hmmm... what should we do here? */
		return;
	}
	while (fgets(line, MAXPATHLEN, fp) != NULL) {
		if (line[strlen(line) - 1] == '\n') {
			line[strlen(line) - 1] = '\0';
		}
		msg->appendOutput(AVO_STRDUP(line));
	}
	fclose(fp);
}
#endif

static void
delete_running_struct(Running rp)
{
	if ((rp->conditional_cnt > 0) &&
	    (rp->conditional_targets != NULL)) {
		retmem_mb((char *) rp->conditional_targets);
	}
/**/
	if ((rp->auto_count > 0) &&
	    (rp->automatics != NULL)) {
		retmem_mb((char *) rp->automatics);
	}
/**/
	if(rp->sprodep_value) {
		free_name(rp->sprodep_value);
	}
	if(rp->sprodep_env) {
		retmem_mb(rp->sprodep_env);
	}
	retmem_mb((char *) rp);

}
