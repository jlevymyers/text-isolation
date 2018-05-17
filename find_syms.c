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


	//Search for dynamic symbols, and main 
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
		//find main function

		/*

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

		*/
	}

	/*

	if(main_off == 0){
		printf("ERROR: Main function could not be found\n");
		return -1;	
	}

	char* text = NULL;
	Elf64_Xword text_size = 0;
       	Elf64_Addr text_off = 0;

	Elf64_Addr *dti_addr = NULL; 
	Elf64_Off *dti_off = NULL;	

	//Find Main Executable Text and Instrumented Section
	for(i = 0; i < shnum; ++i){
		Elf64_Shdr section = shdr[i];	
		char *section_name = section.sh_name + section_strtab; 
		if((section.sh_offset <= main_off) && (main_off < section.sh_offset + section.sh_size)){
			printf("[%u] Symbol Table: %s\n", i, section_strtab + section.sh_name);
			printf("\tSection Offset: 0x%lx, Section Size: 0x%lx\n", section.sh_offset, section.sh_size); 
			text = header_base + section.sh_offset;
			text_size = section.sh_size;
			text_off = section.sh_offset;
		}


		if(!strncmp(".dti", section_name, 5)){
			printf("[%u] Instrumented Section: %s\n", i, section_strtab + section.sh_name);
			printf("\tSection Offset: 0x%lx, Section Size: 0x%lx\n", section.sh_offset, section.sh_size); 
			inst_func_off = section.sh_offset;
			inst_func_size = section.sh_size;	
			//set section address to be loadable executable program section
			dti_addr = &shdr[i].sh_addr;
			dti_off = &shdr[i].sh_offset;
		}

		
	}
	*/

	/*
	 *Extend Program Header 
	 */

	/*
	Elf64_Half phnum = header -> e_phnum;
	Elf64_Phdr* phdr_iter =(Elf64_Phdr*) (header_base + header -> e_phoff);
	
	Elf64_Addr code_end = 0; 

	if(dti_addr){
		for(int i=0; i<phnum; i++){
			if(phdr_iter -> p_type ==  PT_LOAD && (phdr_iter -> p_flags & PF_X)){
				printf("[%d] Physical Address: 0x%lx, Virtual Address 0x%lx, File Size: 0x%lx, Mem Size: 0x%lx, Offset: 0x%lx, Align: 0x%lu\n", 
					i, phdr_iter -> p_paddr, phdr_iter -> p_vaddr, phdr_iter -> p_filesz, phdr_iter -> p_memsz, phdr_iter -> p_offset, phdr_iter -> p_align);
				code_end = (phdr_iter -> p_vaddr) + (phdr_iter -> p_memsz);
				phdr_iter -> p_memsz += inst_func_size;
				phdr_iter -> p_filesz += inst_func_size;
		        	//*dti_addr = code_end;
				printf(".dti section offset set to 0x%lx\n", code_end);
				//*dti_off = code_end;
				*dti_addr = code_end;	
			}
			phdr_iter++;	
		}
	}
	*/


	

	/*
	 * Update References to Main 
	 *     Example: 6fdi: 48 8d e8 0c 01 00 00 lea 0x10c(%rip), %rdi
	 */
	
	/*

	for(i = 0; i < text_size; i++){
		if(memcmp(text + i, "\x8b", 2)){
			int* offset_ptr = (int*) (text + i + 3);
			Elf64_Addr relative_off = *offset_ptr;

			Elf64_Addr offset = *offset_ptr + text_off;
			Elf64_Addr target = offset + i + 7;
		       	
			if(i + text_off == 0x6fd)	
			printf("Address: 0x%lx, Target: 0x%lx, Offset: 0x%lx, Main: 0x%lx\n", i + text_off, target, offset, main_off); 
			if(main_off == target){
				//int new_addr = relative_off + (inst_func_off - main_off);
				//printf("Setting new LEA offset to: %d\n", new_addr);
				//*offset_ptr = new_addr;
				//printf("Setting main reference to 0x%x\n", code_end - (i + text_off));
				//*offset_ptr = (int) (code_end - (i + text_off)); 
				//*offset_ptr += main_size;
			}
		}
	}

	*/

	//clean up
	munmap(header, size);
	close(fd);
	close(fd_asm);
	return 0;

}


//Elf64_Shdr *sh_strtab = &shdr[header -> e_shstrndx];
