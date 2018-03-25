.PHONY: all clean inst linktime test 

CC	?= clang
CDEBUG	= -Wall -Wextra -g 

CSTD	=-std=c99

CFLAGS	= -O0 $(CDEBUG) $(CSTD) -ldl -fPIC


find_dyn_addr.o: find_dyn_addr.c find_dyn_addr.h
	$(CC) $(CDEBUG) -c find_dyn_addr.c $(CFLAGS)


generate_asm.o: generate_asm.c find_dyn_addr.h generate_asm.h 
	$(CC) $(CDEBUG) -c generate_asm.c $(CFLAGS)

find_syms.o: find_syms.c generate_asm.h 
	$(CC) $(CDEBUG) -c find_syms.c $(CFLAGS)

inst: find_syms.o generate_asm.o find_dyn_addr.o 
	$(CC) $(CDEBUG) -o instrument find_syms.o generate_asm.o find_dyn_addr.o $(CFLAGS)

test: open_test.c 
	$(CC) $(CDEBUG) -o open_test open_test.c 

wrap: dyn_wrapper.s find_dyn_addr.o
	$(CC) $(CDEBUG) -shared -o wrapper.so dyn_wrapper.s find_dyn_addr.o $(CFLAGS)	

all: inst test

clean: 
	rm -f instrument find_syms.o generate_asm.o find_dyn_addr.o open_test
