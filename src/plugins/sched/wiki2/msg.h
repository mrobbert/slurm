/*****************************************************************************\
 *  msg.h - Message/communcation manager for Wiki plugin
 *****************************************************************************
 *  Copyright (C) 2006 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Morris Jette <jette1@llnl.gov>
 *  UCRL-CODE-217948.
 *  
 *  This file is part of SLURM, a resource management program.
 *  For details, see <http://www.llnl.gov/linux/slurm/>.
 *  
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission 
 *  to link the code of portions of this program with the OpenSSL library under 
 *  certain conditions as described in each individual source file, and 
 *  distribute linked combinations including the two. You must obey the GNU 
 *  General Public License in all respects for all of the code used other than 
 *  OpenSSL. If you modify file(s) with this exception, you may extend this 
 *  exception to your version of the file(s), but you are not obligated to do 
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in 
 *  the program, then also delete it here.
 *  
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with SLURM; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

/*
 * Two modes of operation are currently supported for job prioritization:
 *
 * PRIO_HOLD: Wiki is a polling scheduler, so the initial priority is always 
 * zero to keep SLURM from spontaneously starting the job.  The scheduler will 
 * suggest which job's priority should be made non-zero and thus allowed to 
 * proceed.
 *
 * PRIO_DECREMENT: Set the job priority to one less than the last job and let 
 * Wiki change priorities of jobs as desired to re-order the queue
 */

#if HAVE_CONFIG_H
#  include "config.h"
#  if HAVE_INTTYPES_H
#    include <inttypes.h>
#  else
#    if HAVE_STDINT_H
#      include <stdint.h>
#    endif
#  endif  /* HAVE_INTTYPES_H */
#else   /* !HAVE_CONFIG_H */
#  include <inttypes.h>
#endif  /*  HAVE_CONFIG_H */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <slurm/slurm_errno.h>

#include "src/common/hostlist.h"
#include "src/common/log.h"
#include "src/common/parse_config.h"
#include "src/common/read_config.h"
#include "src/common/slurm_protocol_api.h"
#include "src/common/slurm_protocol_interface.h"
#include "src/common/xmalloc.h"
#include "src/common/xsignal.h"
#include "src/common/xstring.h"
#include "src/slurmctld/sched_plugin.h"

/* Global configuration parameters */
#define PRIO_HOLD      0
#define PRIO_DECREMENT 1
extern int	init_prio_mode;
extern char *	auth_key;
extern uint16_t	e_port;
extern uint16_t	job_aggregation_time;

extern int	event_notify(char *msg);
extern int	spawn_msg_thread(void);
extern void	term_msg_thread(void);

/* Functions called from within msg.c (rather than creating a bunch 
 * more header files with one function definition each */
extern int	cancel_job(char *cmd_ptr, int *err_code, char **err_msg);
extern int	get_jobs(char *cmd_ptr, int *err_code, char **err_msg);
extern int 	get_nodes(char *cmd_ptr, int *err_code, char **err_msg);
extern int	job_add_task(char *cmd_ptr, int *err_code, char **err_msg);
extern int	job_release_task(char *cmd_ptr, int *err_code, char **err_msg);
extern int	job_requeue(char *cmd_ptr, int *err_code, char **err_msg);
extern int	job_will_run(char *cmd_ptr, int *err_code, char **err_msg);
extern int	start_job(char *cmd_ptr, int *err_code, char **err_msg);
extern int	suspend_job(char *cmd_ptr, int *err_code, char **err_msg);
extern int	resume_job(char *cmd_ptr, int *err_code, char **err_msg);
