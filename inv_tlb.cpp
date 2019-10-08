#include <vector>
#include <iostream>
#include <sstream>
#include <ext/stdio_filebuf.h>
using __gnu_cxx::stdio_filebuf;

#include <iterator>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <chrono>
#include <assert.h>
#include <fcntl.h> 

using namespace std;
using namespace std::chrono;

//#define NUMPROC 1

#define ITER 1024*1024
#define STRIDE 4096

//#define TLB_INV true
//#define MEMSIZE 2*1024*1024
#define MEMSIZE 1*1024*1024
#define MPROT_FLAGS PROT_READ | PROT_WRITE 
#define MPROT_CHANGE PROT_READ

//#define WAIT_ALL true

inline void kernel_loop(char & tmp, void *shmemory, int shmemory_size, int stride) {
	for(int j = 0; j < shmemory_size; j += stride) {
		tmp |= *(((char*)shmemory)+j);
	}
}

std::string read_from_pipe (int fd) {
	FILE* f = fdopen(fd, "r");

	stdio_filebuf<char> * file_buf = new stdio_filebuf<char>(fd, std::ios::in);
	std::istream * stream = new std::istream(file_buf); 

	int c = stream->peek();  // peek character
	
	// return an empty string if no data is available
	if(c == EOF) {
		return std::string();
	}

	string line;
	*stream >> line;
	return line;
}

void write_to_pipe (int fd, string line) {
	stdio_filebuf<char> * file_buf = new stdio_filebuf<char>(fd, std::ios::out);
	std::ostream * stream = new ostream (file_buf);
	
	*stream << line;
	stream->flush();
}


int main(int argc, char** argv) {
	
	// Create pipe for parent and childs to communicate
	int pipe_fd[2];

	if(pipe(pipe_fd)) {
		std::cerr << "Pipe failed" << std::endl;
		return EXIT_FAILURE;
	}

	// Create shared pool of memory	
	void *shmemory = mmap(NULL, MEMSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

	memset(shmemory, 0, MEMSIZE);
	int res = mprotect(shmemory, MEMSIZE, MPROT_FLAGS);	

	pid_t * child_pids = new pid_t[NUMPROC];
	child_pids = (pid_t*)memset(child_pids, 0, sizeof(pid_t)*NUMPROC);
	bool is_parent = true;
	for(int i = 0; i < NUMPROC; ++i) {
		pid_t child_pid = fork();

		if(child_pid) {
			child_pids[i] = child_pid;
		}
		else {
			is_parent = 0;
			break;
		}
	}

	if(is_parent) {
		close(pipe_fd[1]); // close write side of the pipe
		
		string duration_child = string();
		auto start = high_resolution_clock::now();
		for(int i = 0; i < ITER; ++i) {
			if(TLB_INV) {
				int res = mprotect(shmemory, MEMSIZE, i % 2 ? MPROT_CHANGE : MPROT_FLAGS);	
			}
			char tmp;
			kernel_loop(tmp, shmemory, MEMSIZE, STRIDE);
			string line_read = read_from_pipe(pipe_fd[0]);
			if((!line_read.empty())&&(duration_child.empty())) { 
				duration_child = line_read;
				close(pipe_fd[0]);
			}
			if(!WAIT_ALL) {
				break;
			}
		}
		
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(stop - start);

		std::cout << duration.count() << "\t" << duration_child << "\t" << NUMPROC << "\t" << TLB_INV << "\t" << MEMSIZE << "\t" << ITER << std::endl;
	} else {
		close(pipe_fd[0]); // close write side of the pipe
		
		auto start = high_resolution_clock::now();
		for(int i = 0; i < ITER; ++i) {
			char tmp;
			kernel_loop(tmp, shmemory, MEMSIZE, STRIDE);
		}
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(stop - start);

		string done_message = std::to_string(duration.count()) + "\n";
		write_to_pipe(pipe_fd[1], done_message); 
		
		close(pipe_fd[1]);
	}
	
	return EXIT_SUCCESS;
}
