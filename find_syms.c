#include <elf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/unistd.h>

#include <fcntl.h>

#include <stdio.h>
#include <unistd.h>
#include "generate_asm.h"

#include <string.h>


#define OUTPUT_FILE "./dyn_wrapper.s"

/*
 * function which finds all the dynamic symbols in an ELF files
 * 	and generates a wrapper function for each symbol 
 *
 */

int
main(int argc, char* argv[])
{
	if(argc < 2)
	{
		printf("ERROR: Missing filename\n");
		return -1;
	}

	int fd = open(argv[1], O_RDONLY); 

	off_t size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	//find elf header 
	Elf64_Ehdr *header; 
	header = (Elf64_Ehdr *) mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0); 

	char *header_base = (char*) header; 

	Elf64_Shdr *shdr = (Elf64_Shdr *)(&header_base[header ->e_shoff]);
	Elf64_Half shnum = header -> e_shnum;


	Elf64_Half i; 	
	Elf64_Off str_off; 
	Elf64_Xword j;

	printf("opened fd %i, size %ld", fd, size);

	//open output file
	int fd_asm = open(OUTPUT_FILE, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if(fd_asm < 0)
	{
		printf("ERROR: Could not create file 'dyn_wrapper.s'\n");
		return -1;
	}

	char buf[1024];

	size_t len = 0; 
	len = start_asm_file(buf);
	write(fd_asm, buf, len);

	//search for dynamic symbols 
	for(i = 0; i < shnum; ++i)
	{
		if(shdr[i].sh_type==SHT_DYNSYM){
			str_off =  shdr[shdr[i].sh_link].sh_offset;
			Elf64_Sym *symboler = (Elf64_Sym *)(&header_base[shdr[i].sh_offset]);
			for(j = 0; j<(shdr[i].sh_size/shdr[i].sh_entsize); j++)
			{
				memset(buf,0, sizeof(buf));
				char *sym_name = (char*) (&header_base[str_off + symboler-> st_name]);
				len = generate_asm(sym_name, buf); 
				if(len > 0){
					ssize_t num_written = write(fd_asm, buf, len);
					printf("added '%s' instrumentation, bytes written: %li\n", sym_name, num_written);
				}
				fsync(fd_asm);
				symboler++; 
			}
		}
	}

	//clean up
	munmap(header, size);
	close(fd);
	close(fd_asm);
	return 0;

}

//Elf64_Shdr *sh_strtab = &shdr[header -> e_shstrndx];
