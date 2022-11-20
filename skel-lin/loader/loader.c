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
#include <sys/mman.h>
#include <fcntl.h>

#include "exec_parser.h"

static int fd;
static so_exec_t *exec;

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	if(signum != SIGSEGV || info == NULL)
		signal(SIGSEGV, SIG_DFL);
	
	int done = 0;
	for(int i = 0; i < exec->segments_no; i++) 
	{
		so_seg_t *segment = exec->segments + i * sizeof(so_seg_t);
		
		if((int)info->si_addr >= segment->vaddr 
			&& (int)info->si_addr < (segment->vaddr + segment->mem_size)) 
			{
				done = 1;
				int page_index = ((int)info->si_addr - segment->vaddr) / getpagesize();
				int page_addr = segment->vaddr + page_index * getpagesize();
				int len = (int)info->si_addr - page_addr;
				
				// void *ret_mmap = mmap(page_addr, getpagesize(), segment->perm, 
				// 		MAP_FIXED | MAP_PRIVATE, fd, segment->offset + page_index * getpagesize());
				
				void *ret_mmap = mmap(page_addr, getpagesize(), segment->perm, 
						MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
				
				if(ret_mmap == MAP_FAILED)
				{
					printf("salut ");
					return;
				}

				


				printf("map done at %d\n", page_addr);
			}
	}
	if(done == 0)
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
	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -1;
	
	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}
