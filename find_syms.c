#include <elf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/unistd.h>

#include <fcntl.h>
#include <search.h> 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "generate_asm.h"

#include <string.h>

#include <byteswap.h>
#include <errno.h>

#define OUTPUT_FILE "./dyn_wrapper.s"
#define SYMBOL_MAX 1000000

int sym_count = 0;

int find_dependencies(char *main_path, int fd_asm);
int add_file_symbols(char *file_path, int fd_asm);

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

	hcreate(SYMBOL_MAX); 

	char *main_path = argv[1]; 


	//open output file
	int fd_asm = open(OUTPUT_FILE, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if(fd_asm < 0)
	{
		printf("ERROR: Could not create file 'dyn_wrapper.s'\n");
		return -1;
	}
	//initialize the asm file
	char buf[1024];
	size_t len = 0; 
	len = start_asm_file(buf, 0);
	write(fd_asm, buf, len);

	add_file_symbols(main_path, fd_asm);
	find_dependencies(main_path, fd_asm);

	hdestroy();

	close(fd_asm);

}

int never_instrument(char *str){

	if(!strcmp(str, "/lib/x86_64-linux-gnu/libdl.so.2")){
		return 1;
	}
	return 0; 
}

int never_instrument_symbol(char *sym_name){
	if(!strcmp(sym_name, "dlsym") || 
	   strstr(sym_name, "_dl") || 
	   !strcmp(sym_name, "dl_iterate_phdr")|| 
	   !strcmp(sym_name, "dlerror")){
		return 1;		
	}
	else{
		return 0;
       	}
}


/*
 * function which finds all the dynamic function symbols from file_path, and generates instrumentation
 */ 

int add_file_symbols(char *file_path, int fd_asm){
	char buf[1024];
	size_t len = 0; 
	
	int fd = open(file_path, O_RDONLY);
        if(fd < 0){
		printf("ERROR: Opening file %s\n", file_path);
	}	
	off_t size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	//find elf header 
	Elf64_Ehdr *header; 
	header = (Elf64_Ehdr *) mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0); 
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

	//find all dependencies 
	//search for dynamic symbols 
	for(i = 0; i < shnum; ++i)
	{
		Elf64_Shdr section = shdr[i];
		//find dynamic symbols for instrumentation
		if(section.sh_type==SHT_DYNSYM){
			
			//sh_link points to string table for dynamic symbols
			printf("[%u] Dynamic Symbol Table: %s\n", i, section_strtab + section.sh_name);

			char *dyn_strtab =  header_base + shdr[section.sh_link].sh_offset;
			Elf64_Sym *symboler = (Elf64_Sym *)(&header_base[section.sh_offset]);
		
			//check all symbols in the dymsym section	
			for(j = 0; j<(section.sh_size/sizeof(Elf64_Sym)); j++)
			{
				memset(buf,0, sizeof(buf));
				char *sym_name = dyn_strtab + symboler-> st_name;
				int sym_name_len = strnlen(sym_name, 1024);
				if((sym_name_len > 0 && sym_name_len < 1023) && (ELF64_ST_TYPE(symboler -> st_info) == STT_FUNC)){
					if(never_instrument_symbol(sym_name)){
						printf("Symbol %s is marked never instrument\n", sym_name);
					}
					else{
			       			//check if symbol is a function, and has a name
						char* sym_name_dup = strdup(sym_name);
						ENTRY search_entry = {sym_name_dup, symboler}; 
						ENTRY *symbol = hsearch(search_entry, FIND);
						if(symbol != NULL){
							printf("\t%s Already added from another library\n", sym_name);
						}
						else{
							len = generate_asm(sym_name, buf, sym_count);
							if(len > 0){
								ssize_t num_written = write(fd_asm, buf, len);
								fsync(fd_asm);
								printf("\t[%d] added '%s' instrumentation, bytes written: %li\n", 
										sym_count, sym_name, num_written);
								sym_count++;
								ENTRY* result = hsearch(search_entry, ENTER);
								if(result == NULL){
									printf("ERROR: Adding String to Hash Table\n");
								}	
							}
						}
					}
				}
				else{
					//printf("ERROR: Malformed symbol name\n");
				}

				symboler++; 
			}
		}
	
	}

	//clean up
	munmap(header, size);
	close(fd);
	return 0;
}

/*
 * calls ldd to locate all dynamic dependencies
 * 	stackoverflow.com/questions/646241/c-run-a-system-command-and-get-outputi
 */

int find_dependencies(char *main_path, int fd_asm){

	FILE *fp;
	char path[1035];

	/* Open the command for reading. */
	
	char print_buf[1024]; 
	snprintf(print_buf, 1024, "ldd %s | sed -n -e 's/^.*=> //p' | sed 's/ .*//'", main_path); 
	fp = popen(print_buf, "r");
	if (fp == NULL) {
		printf("Failed to run command\n" );
		exit(1);
	}

	/* Read the output a line at a time - output it. */
	while (fgets(path, sizeof(path)-1, fp) != NULL) {
		//char *parsed_path = strtok(path, "/");
		int len = strnlen(path, 1024);
		path[len - 1] = '\0';
		if(never_instrument(path)){
			printf("Dynamic library %s marked never instrument (always executable\n", path);
		}
		else{
			printf("%s\n", path);
	       		add_file_symbols(path, fd_asm);
		}
	}

	/* close */
	pclose(fp);
	return 0;

}





