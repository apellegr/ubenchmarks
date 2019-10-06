#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <chrono>
#include <assert.h>

using namespace std;
using namespace std::chrono;

//#define NUMPROC 1

#define ITER 1024*1024
#define STRIDE 4096

//#define TLB_INV true
#define MEMSIZE 2*1024*1024
#define MPROT_FLAGS PROT_READ | PROT_WRITE 
#define MPROT_CHANGE PROT_READ



int main(int argc, char** argv) {

	pid_t * child_pids = new pid_t[NUMPROC];
	child_pids = (pid_t*)memset(child_pids, 0, sizeof(pid_t)*NUMPROC);
	
	pid_t wpid;
	int status = 0;

	void *shmemory = mmap(NULL, MEMSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

	memset(shmemory, 0, MEMSIZE);
	int res = mprotect(shmemory, MEMSIZE, MPROT_FLAGS);	

	for(int i = 0; i < NUMPROC; ++i) {
		pid_t child_pid = fork();

		if(child_pid) {
			child_pids[i] = child_pid;
		}
		else {
			break;
		}
	}

	bool is_parent = true;
	for(int i = 0; i < NUMPROC; ++i) {
		if(!child_pids[i]) {
			is_parent = false;
			break;
		}
	}


	char tmp = 0;
	if(is_parent) {	
		auto start = high_resolution_clock::now();
		for(int i = 0; i < ITER; ++i) {
			if(TLB_INV) {
				int res = mprotect(shmemory, MEMSIZE, i % 2 ? MPROT_CHANGE : MPROT_FLAGS);	
			}
			for(int j = 0; j < MEMSIZE; j += STRIDE) {
				tmp |= *(((char*)shmemory)+j);
			}
		}
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(stop - start);
		
		while((wpid == wait(&status)) > 0);

		std::cout << duration.count() << "\t" << NUMPROC << "\t" << TLB_INV << "\t" << MEMSIZE << "\t" << ITER << std::endl;
	} else {
		for(int i = 0; i < ITER; ++i) {
			for(int j = 0; j < MEMSIZE; j += STRIDE) {
				tmp |= *(((char*)shmemory)+j);
			}	
		}

	}

	return 0;
}
