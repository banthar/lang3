
#include "stream.h"
#include "error.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static bool charIsWhitespace(char c)
{
	switch(c)
	{
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			return true;
		default:
			return false;
	}
}

Stream* openStream(const char* filename)
{

	assert(filename!=NULL);

	int fd=open(filename,O_RDONLY);
	assert(fd>=0);
		
	struct stat statbuf;
	assert(fstat(fd, &statbuf)==0);

	Stream* s=malloc(sizeof(Stream));
	
	s->length=statbuf.st_size;
	s->offset=0;
	s->data=mmap(NULL, s->length, PROT_READ, MAP_PRIVATE,fd,0);
	s->filename=strdup(filename);

	s->errorHandler=abort;

	assert(s->data!=NULL);

	return s;

}

void closeStream(Stream* s)
{
	assert(s!=NULL);
	munmap(s->data,s->length);
	free(s->filename);
	free(s);
}

bool isEof(Stream* s)
{
	return s->offset>=s->length;
}

char peekStream(Stream* s, int offset)
{
	if(s->offset+offset>=s->length || s->offset+offset<0)
		return '\0';
	else
		return s->data[s->offset+offset];
}

char getStream(Stream* s, int position)
{
	if(position>=s->length || position<0)
		return '\0';
	else
		return s->data[position];
}

void seekStream(Stream* s, int offset)
{
	s->offset+=offset;
}

void makeString(Stream* s, int start, int end, String* out)
{
	if(out==NULL)
		return;

	*out=(String){
		.offset=start,
		.length=end,
		.stream=s,
	};
}

bool compareString(const String* a, const String* b)
{

	if(a->length!=b->length)
		return false;

	for(int i=0;i<a->length;i++)
		if(a->stream->data[a->offset+i]!=b->stream->data[b->offset+i])
			return false;

	return true;

}

static void printContext(Stream* s, int start, int end)
{

	if(start<s->length)
	{


		int quote_start=start;

		while((getStream(s,quote_start))!='\n' && getStream(s,quote_start)!='\0')
			quote_start--;

		while(charIsWhitespace(getStream(s,quote_start)) || getStream(s,quote_start)=='\0')
			quote_start++;

		int quote_end=quote_start;

		while((getStream(s,quote_end))!='\n' && getStream(s,quote_end)!='\0')
			quote_end++;


		fprintf(stderr, "\t");

		for(int i=quote_start;i<quote_end;i++)
		{
			if(getStream(s,i)=='\t')
				fprintf(stderr," ");
			else
				fprintf(stderr,"%c",getStream(s,i));
		}

		fprintf(stderr, "\n");
		fprintf(stderr, "\t");

		for(int i=quote_start;i<quote_end && i<end;i++)
		{
			if(i>=start && i<end)
				fprintf(stderr, "^");
			else
				fprintf(stderr, " ");
		}


		fprintf(stderr, "\n");

		
	}

}

static void printPosition(Stream* s, int offset, int length)
{
	int line=1;
	int column=1;

	for(int i=0;i<offset;i++)
	{
		if(s->data[i]=='\n')
		{
			line++;
			column=0;
		}

		column++;

	}

	if(length==0)
	    fprintf(stderr, "%s:%i:%i: ", s->filename,line,column);
	else
	    fprintf(stderr, "%s:%i:%i-%i: ", s->filename,line,column,column+length);


}

void warnStream(Stream* s, const char* format, ...)
{

    printPosition(s,s->offset,0);

	fprintf(stderr, "warning: ");

	va_list args;
	va_start(args,format);
	vfprintf(stderr,format,args);
	va_end(args);

	fprintf(stderr, "\n");
}

void vpanicString(String* s, const char* format, va_list args)
{
    printPosition(s->stream,s->offset,s->length);
    fprintf(stderr, "error: ");

	vfprintf(stderr,format,args);

	fprintf(stderr, "\n");

	printContext(s->stream,s->offset,s->offset+s->length);

	s->stream->errorHandler();
	assert(false);
}

void panicString(String* s, const char* format, ...)
{

	va_list args;
	va_start(args,format);
	vpanicString(s,format,args);
	va_end(args);

}

bool panicStream(Stream* s, const char* format, ...)
{

    printPosition(s,s->offset,0);
    fprintf(stderr, "error: ");

	va_list args;
	va_start(args,format);
	vfprintf(stderr,format,args);
	va_end(args);

	fprintf(stderr, "\n");

	printContext(s,s->offset,s->offset+1);

	s->errorHandler();
	assert(false);

}

char* strdupString(const String* s)
{
	return strndup(s->stream->data+s->offset,s->length);
}

char* copyString(const String* s, char* buf, int len)
{

	int min(int a, int b)
	{
		return a>b?b:a;
	}

	for(int i=0;i<len&&i<s->length;i++)
		buf[i]=s->stream->data[s->offset+i];

	buf[min(len-1,s->length)]='\0';

	return buf;

}

void printString(FILE* f, String* s)
{

	if(s==NULL)
	{
		fprintf(f,"null");
	}
	else
	{
		for(int i=0;i<s->length;i++)
		{
			fprintf(f,"%c",s->stream->data[s->offset+i]);
		}
	}

}


