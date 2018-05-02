.PHONY: all clean inst linktime test 

#LD_DEBUG
#intel XED 
CC	?= gcc
CDEBUG	= -Wall -Wextra -g 

CSTD	=-std=gnu99

CFLAGS	= -z noexecstack -O0 $(CDEBUG) $(CSTD) -ldl -fPIC

STATIC	= 
#-static-libgcc -static-libstdc++ 

TARGET ?= hello.txt

find_dyn_addr.o: find_dyn_addr.c find_dyn_addr.h
	$(CC) $(CDEBUG) -c find_dyn_addr.c $(CFLAGS) $(STATIC)

generate_asm.o: generate_asm.c generate_asm.h 
	$(CC) $(CDEBUG) -c generate_asm.c $(CFLAGS)

find_syms.o: find_syms.c  
	$(CC) $(CDEBUG) -c find_syms.c $(CFLAGS)

inst: find_syms.o generate_asm.o 
	$(CC) $(CDEBUG) -o instrument find_syms.o generate_asm.o $(CFLAGS)

test: open_test.c 
	$(CC) $(CDEBUG) -o open_test open_test.c 

wrap: dyn_wrapper.s find_dyn_addr.o
	$(CC) $(CDEBUG) -shared -o wrapper.so dyn_wrapper.s find_dyn_addr.o $(CFLAGS) $(STATIC)	

run: test wrap
	LD_PRELOAD=$(PWD)/wrapper.so /gpfs/main/home/jlevymye/course/cs2951/text-isolation/open_test $(TARGET) 

gdb: test wrap
	LD_PRELOAD=$(PWD)/wrapper.so gdb /gpfs/main/home/jlevymye/course/cs2951/text-isolation/open_test 

trace: test wrap
	LD_PRELOAD=$(PWD)/wrapper.so strace /gpfs/main/home/jlevymye/course/cs2951/text-isolation/open_test $(TARGET)

all: inst test

clean: 
	rm -f instrument find_syms.o generate_asm.o find_dyn_addr.o open_test wrapper.so dyn_wrapper.s 
