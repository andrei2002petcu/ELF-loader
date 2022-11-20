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
	//printf("am intrat");
	//getchar();
	if(signum != SIGSEGV || info == NULL)
		signal(SIGSEGV, SIG_DFL);
	
	int done = 0;
	for(int i = 0; i < exec->segments_no && done == 0; i++) 
	{
		so_seg_t *segment = exec->segments + i;
		
		if((int)info->si_addr >= segment->vaddr 
			&& (int)info->si_addr < (segment->vaddr + segment->mem_size)) 
			{
				done = 1;
				int page_index = ((int)info->si_addr - segment->vaddr) / getpagesize();
				//int page_addr = segment->vaddr + page_index * getpagesize();
				int page_offset = page_index * getpagesize();
				
				// void *ret_mmap = mmap(segment->vaddr + page_offset, getpagesize(), segment->perm, 
				// 		MAP_FIXED | MAP_PRIVATE, fd, segment->offset + page_index * getpagesize());
				
				void *ret_mmap = mmap((void *)segment->vaddr + page_offset, getpagesize(), PERM_R | PERM_W, 
						MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
				
				if(ret_mmap == MAP_FAILED)
				{
					printf("salut ");
					return;
				}

				lseek(fd, segment->offset + page_offset, SEEK_SET);
				if (page_offset + getpagesize() < segment->file_size)
					read(fd, ret_mmap, getpagesize());
				else if(page_offset <= segment->file_size) 
					{
					read(fd, ret_mmap, segment->file_size - page_offset);
					memset((void *)segment->vaddr + segment->file_size, 0, page_offset + getpagesize() - segment->file_size);
					}
				else if(page_offset > segment->file_size)
					memset((void *)segment->vaddr + page_offset, 0, getpagesize());

				mprotect((void *)segment->vaddr + page_offset, getpagesize(), segment->perm);

				//printf("map done at %d\n", segment->vaddr + page_offset);
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
