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
 * @(#)bsd.cc 1.6 06/12/12
 */

/*	from OpenSolaris "bsd.cc	1.6	06/12/12"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)bsd.cc	1.6 (gritter) 01/22/07
 */

#include <signal.h>

#include <bsd/bsd.h>

/* External references.
 */

/* Forward references.
 */

/* Static data.
 */

extern SIG_PF
bsdsignal (int Signal, SIG_PF Handler)
{
  auto SIG_PF                   previous_handler;
#ifdef SUN5_0
#ifdef sun
  previous_handler = sigset (Signal, Handler);
#else
  auto struct sigaction         new_action;
  auto struct sigaction         old_action;

  new_action.sa_flags = SA_SIGINFO;
  new_action.sa_handler = (void (*) ()) Handler;
  sigemptyset (&new_action.sa_mask);
  sigaddset (&new_action.sa_mask, Signal);

  sigaction (Signal, &new_action, &old_action);

  previous_handler = (SIG_PF) old_action.sa_handler;
#endif
#elif defined (__FreeBSD__)
  previous_handler = signal (Signal, Handler);
#else
  previous_handler = sigset (Signal, Handler);
#endif
  return previous_handler;
}

extern void
bsd_signals (void)
{
  static int                    initialized = 0;

  if (initialized == 0)
    {
      initialized = 1;
#if 0
#if !defined(SUN5_0) && !defined(linux)
#if defined(SIGHUP)
      bsdsignal (SIGHUP, SIG_DFL);
#endif
#if defined(SIGINT)
      bsdsignal (SIGINT, SIG_DFL);
#endif
#if defined(SIGQUIT)
      bsdsignal (SIGQUIT, SIG_DFL);
#endif
#if defined(SIGILL)
      bsdsignal (SIGILL, SIG_DFL);
#endif
#if defined(SIGTRAP)
      bsdsignal (SIGTRAP, SIG_DFL);
#endif
#if defined(SIGIOT)
      bsdsignal (SIGIOT, SIG_DFL);
#endif
#if defined(SIGABRT)
      bsdsignal (SIGABRT, SIG_DFL);
#endif
#if defined(SIGEMT)
      bsdsignal (SIGEMT, SIG_DFL);
#endif
#if defined(SIGFPE)
      bsdsignal (SIGFPE, SIG_DFL);
#endif
#if defined(SIGBUS)
      bsdsignal (SIGBUS, SIG_DFL);
#endif
#if defined(SIGSEGV)
      bsdsignal (SIGSEGV, SIG_DFL);
#endif
#if defined(SIGSYS)
      bsdsignal (SIGSYS, SIG_DFL);
#endif
#if defined(SIGPIPE)
      bsdsignal (SIGPIPE, SIG_DFL);
#endif
#if defined(SIGALRM)
      bsdsignal (SIGALRM, SIG_DFL);
#endif
#if defined(SIGTERM)
      bsdsignal (SIGTERM, SIG_DFL);
#endif
#if defined(SIGUSR1)
      bsdsignal (SIGUSR1, SIG_DFL);
#endif
#if defined(SIGUSR2)
      bsdsignal (SIGUSR2, SIG_DFL);
#endif
#if defined(SIGCLD)
      bsdsignal (SIGCLD, SIG_DFL);
#endif
#if defined(SIGCHLD)
      bsdsignal (SIGCHLD, SIG_DFL);
#endif
#if defined(SIGPWR)
      bsdsignal (SIGPWR, SIG_DFL);
#endif
#if defined(SIGWINCH)
      bsdsignal (SIGWINCH, SIG_DFL);
#endif
#if defined(SIGURG)
      bsdsignal (SIGURG, SIG_DFL);
#endif
#if defined(SIGIO)
      bsdsignal (SIGIO, SIG_DFL);
#else
#if defined(SIGPOLL)
      bsdsignal (SIGPOLL, SIG_DFL);
#endif
#endif
#if defined(SIGTSTP)
      bsdsignal (SIGTSTP, SIG_DFL);
#endif
#if defined(SIGCONT)
      bsdsignal (SIGCONT, SIG_DFL);
#endif
#if defined(SIGTTIN)
      bsdsignal (SIGTTIN, SIG_DFL);
#endif
#if defined(SIGTTOU)
      bsdsignal (SIGTTOU, SIG_DFL);
#endif
#if defined(SIGVTALRM)
      bsdsignal (SIGVTALRM, SIG_DFL);
#endif
#if defined(SIGPROF)
      bsdsignal (SIGPROF, SIG_DFL);
#endif
#if defined(SIGXCPU)
      bsdsignal (SIGXCPU, SIG_DFL);
#endif
#if defined(SIGXFSZ)
      bsdsignal (SIGXFSZ, SIG_DFL);
#endif
#endif
#endif
    }

  return;
}
