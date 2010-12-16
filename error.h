
#ifndef __ERROR_H__
#define __ERROR_H__

__attribute__((__noreturn__)) void panic(char *file,int line,char *format,...);

#define panic(format,...) panic(__FILE__,__LINE__,format,__VA_ARGS__)
#define assert(arg) {if(!(arg)){panic("failed assertion: %s",#arg);}}

#endif
