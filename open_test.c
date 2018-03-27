#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

/*
 * model test program which echos a file to stdout with unix read/write API
 */


int
main(int argc, const char* argv[]){
	if(argc < 2){
		printf("ERROR: No file specified\n");
		return -1; 
	}
	const char *pathname = argv[1]; 

	// OPEN FILE
	int fd = open(pathname, O_RDWR);
	if(fd < 0){
		printf("ERROR: Opening file %s\n", pathname);
		return -1;
	}

	//READ FILE

	char buf[64];
	ssize_t len = read(fd, buf, 64); 
	
	if(len == 0){
		printf("ERROR: Reading file %s\n", pathname);
		return -1; 
	}

	//WRITE TO I/O

	printf("\n %s \n", buf); 

	//CLOSE FILE

	if(close(fd)){
		printf("ERROR: Closing file %s\n", pathname);
		return -1; 
	}
	printf("SUCCESS: File operations finished\n");
	return 0; 
}
