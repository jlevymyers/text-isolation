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

#include <byteswap.h>
#include <errno.h>

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

	int fd = open(argv[1], O_RDWR); 

	off_t size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	//find elf header 
	Elf64_Ehdr *header; 
	header = (Elf64_Ehdr *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
	if(header == ((void*) -1)){
		printf("ERROR Opening File: %s\n", strerror(errno));
		return -1;
	}

	char *header_base = (char*) header; 

	Elf64_Shdr *shdr = (Elf64_Shdr*) (header_base + header ->e_shoff);
	Elf64_Addr base_addr = header -> e_entry; /* entry point */ 
	Elf64_Half shnum = header -> e_shnum;


	Elf64_Half i; 	
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
	len = start_asm_file(buf, base_addr);
	write(fd_asm, buf, len);

	//Index of Section Name String Table
	Elf64_Half strtab_idx = header -> e_shstrndx;
	
	char* section_strtab = header_base + shdr[strtab_idx].sh_offset;

	char* strtab = NULL;  
	Elf64_Off main_off, inst_func_off = 0;
	Elf64_Xword main_size, inst_func_size = 0;

	//Find String Table
	for(i = 0; i < shnum; ++i){
		Elf64_Shdr section = shdr[i];
		if(section.sh_type == SHT_STRTAB && i != strtab_idx){
		       	if(strtab != NULL){
				printf("ERROR: Two string tables\n"); 
			}
			strtab = &header_base[section.sh_offset];
		}
	}


	//Search for dynamic symbols 
	for(i = 0; i < shnum; ++i)
	{
		Elf64_Shdr section = shdr[i];
		//find dynamic symbols for instrumentation
		if(section.sh_type==SHT_DYNSYM){
			
			//sh_link points to string table for dynamic symbols
			printf("[%u] Dynamic Symbol Table: %s\n", i, section_strtab + section.sh_name);

			char *dyn_strtab =  header_base + shdr[section.sh_link].sh_offset;
			Elf64_Sym *symboler = (Elf64_Sym *)(&header_base[section.sh_offset]);
			
			for(j = 0; j<(section.sh_size/sizeof(Elf64_Sym)); j++)
			{
				memset(buf,0, sizeof(buf));
				char *sym_name = dyn_strtab + symboler-> st_name;
				len = generate_asm(sym_name, buf); 
				if(len > 0 && (ELF64_ST_TYPE(symboler -> st_info) == STT_FUNC)){
					ssize_t num_written = write(fd_asm, buf, len);
					printf("\t added '%s' instrumentation, bytes written: %li\n", sym_name, num_written);
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
