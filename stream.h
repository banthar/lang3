
#ifndef __STREAM_H_
#define __STREAM_H_

#include "bool.h"

typedef struct
{
	int length;
	int offset;
	char* data;
	char* filename;
}Stream;

typedef struct
{
	int offset;
	int length;
	Stream* stream;
}String;

void printString(String* s);

Stream* openStream(const char* filename);
void closeStream(Stream* s);
char peekStream(Stream* s, int offset);
void seekStream(Stream* s, int offset);
void makeString(Stream* s, int start, int end, String* out);
void panicStream(Stream* s, const char* format, ...);

bool isAlpha(Stream* s);
bool isDigit(Stream* s);
bool isAlphaNum(Stream* s);
bool isWhitespace(Stream* s);
bool isChar(Stream* s, char c);
bool isEof(Stream* s);
bool isString(Stream* s, const char* string);

bool readWhitespace(Stream* s);
bool readComment(Stream* s, String* out);
bool readString(Stream* s, const char* pattern);
bool readIdentifier(Stream* s, String* out, const char* pattern);
bool readNumber(Stream* s, String* out);

#endif
