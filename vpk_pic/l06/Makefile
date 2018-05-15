DBG		= gdb			# debugger
DBG_CMD		= "unset environment"	# debugger commands (initialization)
RUNAS		= setarch i686 -3 -R	# x86, 3GB address space, no ASLR
ENV		= env -i		# empty environment
LDFLAGS		+= -z norelro		# no RELRO
					# x86 (32-bit) code, no {PIC, PIE},
					# debug information 
CFLAGS		+= -m32 -no-pie -fno-pic -ggdb3


# phony targets
.PHONY: all esrv_run esrv_dbg clean

# build everything (default target)
all: esrv expl4 expl5

# run the server
esrv_run: esrv
	$(RUNAS) $(ENV) ./$<
# debug the server
esrv_dbg: esrv
	$(RUNAS) $(DBG) -iex=$(DBG_CMD) ./$<

# build the server
esrv: esrv.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

# build the exploits
expl4: expl4.c
	$(CC) $(CFLAGS) $< -o $@
expl5: expl5.c
	$(CC) $(CFLAGS) $< -o $@

# clean
clean:
	$(RM) esrv expl4 expl5
