/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "exec_parser.h"

static so_exec_t *exec;

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	for(int i = 0; i < exec->segments_no; i++) {
		so_seg_t *segment = exec->segments + i * sizeof(so_seg_t);
		int address = info->si_addr;
		if(address >= segment->vaddr && 
			address < (segment->vaddr + segment->file_size)) { //vezi ca ai pus file nu mem
				int page_addr =  (address - segment->vaddr) / getpagesize();
				//alocam
			}

	}


		signal(SIGSEGV, SIG_DFL);

	/* TODO - actual loader implementation */
}

int so_init_loader(void)
{
	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, NULL);
	if (rc < 0) {
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}
