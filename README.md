# text-isolation 
Project which prevents execution across dynamic code sections. 

# Making Project
make inst # generates instrumentation file 
make test # generates open\_test, a working example

# Making Wrapper Project 
./instrument [target] # this will output instrumentation code in a file called dyn\_sym.s

make wrap # compiles wrapper into a dynamic object (.so) 

# Running Linker with Hooks
make run SOURCE=main\_executable # run the program with the LD\_PRELOAD flag set

# Structure of Project

### Static Hook Generation 
1. find\_syms() identifies all dynamic symbols in binary and dependencies 
2. generate\_asm() generates hook file
 
### Runtime 
3. LD\_PRELOAD variable set to file containing dynamic symbol hooks 
4. When a function is hooked, find\_dyn\_addr() resolves the dynamic symbol 
5. Code region containing function symbol is marked executable, other code regions are marked read only 
6. Execution is redirected to dynamic function 
7. TODO: On return from dynamic call, calling code region is again marked executable 
