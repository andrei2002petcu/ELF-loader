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

static void segv_handler(int signum, siginfo_t *info, void *context) {
	
	if(signum != SIGSEGV || info == NULL) {
		perror("Signal not recognized!");
		signal(SIGSEGV, SIG_DFL);
	}
	
	int done = 0;
	//iteram prin vectorul de segmente cautand locatia unde s-a produs page fault-ul
	for(int i = 0; i < exec->segments_no && done == 0; i++) {
		so_seg_t *segment = exec->segments + i;
		
		//se initializeaza un vector care va contine 0/1 la indexul paginii indicand daca pagina este mapata
		if(segment->data == NULL)
			segment->data = calloc(segment->mem_size/getpagesize(), sizeof(int));
		
		if((int)info->si_addr >= segment->vaddr && (int)info->si_addr < (segment->vaddr + segment->mem_size)) {
			done = 1;
			int page_index = ((int)info->si_addr - segment->vaddr) / getpagesize();
			int page_offset = page_index * getpagesize();
			int page_addr = segment->vaddr + page_offset;
			
			//se efectueaza maparea daca pagina nu a fost anterior deja mapata
			if(*((int *)(segment->data) + page_index) != 0) {
				perror("Page already mapped!");
				signal(SIGSEGV, SIG_DFL);
			}

			void *ret_mmap = mmap((void *)page_addr, getpagesize(), PERM_W, 
					MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			
			if(ret_mmap == MAP_FAILED) {
				perror("Map failed");
				signal(SIGSEGV, SIG_DFL);
			}
			else *((int *)(segment->data) + page_index) = 1;

			//citirea datelor in memorie in functie de pozitia paginii in segment
			lseek(fd, segment->offset + page_offset, SEEK_SET);
			if (page_offset + getpagesize() < segment->file_size)
				read(fd, ret_mmap, getpagesize());
			
			else if(page_offset <= segment->file_size) {
				read(fd, ret_mmap, segment->file_size - page_offset);
				memset((void *)segment->vaddr + segment->file_size, 0, page_offset + getpagesize() - segment->file_size);
			}
			
			else if(page_offset > segment->file_size)
				memset((void *)page_addr, 0, getpagesize());
			
			mprotect((void *)page_addr, getpagesize(), segment->perm);
		}
	}
	if(done == 0) {
		perror("Page not found!");
		signal(SIGSEGV, SIG_DFL);
	}
}

int so_init_loader(void) {
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

int so_execute(char *path, char *argv[]) {
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("File open failed!");
		exit(EXIT_FAILURE);
	}
	
	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	close(fd);
	return -1;
}
