#text-isolation 
Project which prevents execution across dynamic code sections. 

#Making Project
make inst 

make test 

#Making Wrapper Project 
./instrument [target] 

make wrap

#Running Linker with Hooks
make run

#Structure of Project

### Static Hook Generation 
1. find\_syms() identifies all dynamic symbols in binary 
2. generate\_asm() generates hook file
 
### Runtime 
3. LD\_PRELOAD variable set to file containing dynamic symbol hooks 
4. When a function is hooked, find\_dyn\_addr() resolves the dynamic symbol 
5. Code region containing function symbol is marked executable, other code regions are marked read only 
6. Execution is redirected to dynamic function 
7. TODO: On return from dynamic call, calling code region is again marked executable 
