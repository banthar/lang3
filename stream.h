
#ifndef __STREAM_H_
#define __STREAM_H_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "bool.h"

typedef struct
{
	int length;
	int offset;
	char* data;
	char* filename;
	__attribute__((noreturn)) void (*errorHandler)(); 

}Stream;

typedef struct
{
	int offset;
	int length;
	Stream* stream;
}String;


/* char* cstr(const String s); */
//#define cstr(s) ((s).length>0?(strndupa((s).stream->data+(s).offset,(s).length)):"")
bool compareString(const String* a, const String* b);
char* copyString(const String* s, char* buf, int len);
char* strdupString(const String* s);
#define cstrString(s) ((s)->length==0?"":copyString((s),alloca((s)->length+1),(s)->length+1))
void printString(FILE* f, String* s);

Stream* openStream(const char* filename);
void closeStream(Stream* s);
char peekStream(Stream* s, int offset);
char getStream(Stream* s, int position);
void seekStream(Stream* s, int offset);
bool isEof(Stream* s);
void makeString(Stream* s, int start, int end, String* out);
void warnStream(Stream* s, const char* format, ...);
__attribute__((noreturn)) bool panicStream(Stream* s, const char* format, ...);
__attribute__((noreturn)) void panicString(String* s, const char* format, ...);
__attribute__((noreturn)) void vpanicString(String* s, const char* format, va_list args);
bool isEof(Stream* s);

#endif

