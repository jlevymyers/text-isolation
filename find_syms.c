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
	Elf64_Addr main_off = 0;
	Elf64_Xword main_size = 0;

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


	//Search for dynamic symbols, and main 
	for(i = 0; i < shnum; ++i)
	{
		Elf64_Shdr section = shdr[i];
		//find dynamic symbols for instrumentation
		if(section.sh_type==SHT_DYNSYM){
			
			//sh_link points to string table for dynamic symbols
			printf("[%u] Dynamic Symbol Table: %s\n", i, section_strtab + shdr[i].sh_name);

			char *dyn_strtab =  header_base + shdr[section.sh_link].sh_offset;
			Elf64_Sym *symboler = (Elf64_Sym *)(&header_base[section.sh_offset]);
			
			for(j = 0; j<(section.sh_size/sizeof(Elf64_Sym)); j++)
			{
				memset(buf,0, sizeof(buf));
				char *sym_name = dyn_strtab + symboler-> st_name;
				len = generate_asm(sym_name, buf); 
				if(len > 0){
					ssize_t num_written = write(fd_asm, buf, len);
					printf("\t added '%s' instrumentation, bytes written: %li\n", sym_name, num_written);
				}
				fsync(fd_asm);
				symboler++; 
			}
		}
		//find main function
		if(section.sh_type==SHT_SYMTAB){
			printf("[%u] Symbol Table: %s\n", i, section_strtab + section.sh_name);
			
			
			Elf64_Sym *symboler = (Elf64_Sym *)(&header_base[section.sh_offset]);
			for(j = 0; j<(section.sh_size/sizeof(Elf64_Sym)); j++)
			{
					memset(buf,0, sizeof(buf));
					char *sym_name = &strtab[symboler-> st_name];
					if(!strncmp(sym_name, "main", 5) && (ELF64_ST_TYPE(symboler -> st_info) == STT_FUNC)){
						printf( 
							"\t[%lu]: %s, Strtab Index: 0x%x, Value: 0x%lx, Size: 0x%lx, Info: 0x%hhx\n",
							j ,sym_name, symboler -> st_name, symboler -> st_value, 
							symboler -> st_size, ELF64_ST_TYPE(symboler -> st_info));
						//Save Main Value for Instrumentation
						if(main_off != 0){
							printf("ERROR: Two Main Functions Found\n"); 
						}
						main_off = symboler -> st_value;
						main_size = symboler -> st_size; 
					}
				symboler++;
			}	
		}
	}

	if(main_off == 0){
		printf("ERROR: Main function could not be found\n");
		return -1;	
	}

	//Find Main Executable Text
	for(i = 0; i < shnum; ++i){
		Elf64_Shdr section = shdr[i];	
	}

	//clean up
	munmap(header, size);
	close(fd);
	close(fd_asm);
	return 0;

}


//Elf64_Shdr *sh_strtab = &shdr[header -> e_shstrndx];
