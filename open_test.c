#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/*
 * model test program which echos a file to stdout with unix read/write API
 */


int
main(int argc, const char* argv[], char* envp[]){
	if(argc < 2){
		//printf("ERROR: No file specified\n");
		return -1; 
	}
	const char *pathname = argv[1]; 

	// OPEN FILE
	int fd = open(pathname, O_RDWR);
	if(fd < 0){
		//printf("ERROR: Opening file %s\n", pathname);
		return -1;
	}

	//READ FILE

	char buf[32];
	ssize_t len = read(fd, buf, 256); 
	
	if(len == 0){
		//printf("ERROR: Reading file %s\n", pathname);
		return -1; 
	}

	//WRITE TO I/O

	write(1, buf, len); 

	//CLOSE FILE

	if(close(fd)){
		//printf("ERROR: Closing file %s\n", pathname);
		return -1; 
	}
	//printf("SUCCESS: File operations finished\n");
	exit(0); 
}

int main_2(int arg, const char* argv[], char* envp[]){
	printf("reached isntrumented main\n");
	exit(0);
}
