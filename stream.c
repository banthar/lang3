
#include "stream.h"
#include "error.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

char peekStream(Stream* s, int offset)
{
	if(s->offset+offset>=s->length || s->offset+offset<0)
		return '\0';
	else
		return s->data[s->offset+offset];
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

void panicStream(Stream* s, const char* format, ...)
{

	int line=1;
	int column=1;

	for(int i=0;i<s->offset;i++)
	{
		if(s->data[i]=='\n')
		{
			line++;
			column=0;
		}

		column++;

	}

	va_list args;
	va_start(args,format);

	fprintf(stderr, "%s:%i:%i: ", s->filename,line,column);
	vfprintf(stderr,format,args);

	fprintf(stderr, "\n");


	if(!isEof(s))
	{

		fprintf(stderr, "\t");

		int offset=0;

		while(peekStream(s,offset-1)!='\n' && peekStream(s,offset-1)!='\0')
			offset--;

		int n=offset;

		while(peekStream(s,n)!='\n'  && peekStream(s,n)!='\0')
		{
			if(peekStream(s,n)=='\t')
				fprintf(stderr," ");
			else
				fprintf(stderr,"%c",peekStream(s,n));

			n++;
		}

		fprintf(stderr, "\n");
		fprintf(stderr, "\t");

		for(int i=0;i<-offset;i++)
			fprintf(stderr, " ");
		
		fprintf(stderr, "^\n");

		
	}

	fprintf(stderr,"\n");

	va_end(args);

//	exit(1);

    abort();

}

void printString(String* s)
{

	if(s==NULL)
	{
		printf("null");
	}
	else
	{
		for(int i=0;i<s->length;i++)
		{
			printf("%c",s->stream->data[s->offset+i]);
		}
	}
}

bool isAlpha(Stream* s)
{
	switch(peekStream(s, 0))
	{
		case '_':
		case 'A'...'Z':
		case 'a'...'z':
			return true;
		default:
			return false;
	}
}

bool isDigit(Stream* s)
{
	switch(peekStream(s, 0))
	{
		case '0'...'9':
			return true;
		default:
			return false;
	}
}

bool isAlphaNum(Stream* s)
{
	return isAlpha(s) || isDigit(s);
}

bool isWhitespace(Stream* s)
{

	switch(peekStream(s, 0))
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

bool isChar(Stream* s, char c)
{
	return peekStream(s,0)==c;
}

bool isEof(Stream* s)
{
	return peekStream(s,0)=='\0';
}

bool isString(Stream* s, const char* string)
{
	for(int i=0;i<strlen(string);i++)
		if(peekStream(s,i)!=string[i])
			return false;

	return true;
}

bool readWhitespace(Stream* s)
{

	if(!isWhitespace(s))
		return false;

	while(isWhitespace(s))
		seekStream(s,1);

	return true;

}

bool readComment(Stream* s, String* out)
{

	if(isString(s,"/*"))
	{

		seekStream(s,2);
		int start=s->offset;

		int d=1;
		
		while(d>0)
		{

			if(isString(s,"/*"))
			{
				d++;
				seekStream(s,2);
			}
			else if(isString(s,"*/"))
			{
				d--;
				seekStream(s,2);
			}
			else if(isEof(s))
			{
				panic("EOF while looking for '*/'",0);
			}
			else
			{
				seekStream(s,1);
			}

		}

		makeString(s, start, s->offset-start-2, out);

		return true;

	}
	else if(isString(s,"//"))
	{
		seekStream(s,2);
		int start=s->offset;

		while(!isString(s,"\n") && !isEof(s))
		{
			seekStream(s,1);
		}

		seekStream(s,1);

		makeString(s, start, s->offset-start-1, out);

		return true;

	}
	else
	{
		return false;
	}

}

bool readString(Stream* s, const char* pattern)
{
	if(isString(s,pattern))
	{
		seekStream(s,strlen(pattern));
		return true;
	}
	else
	{
		return false;
	}
}

bool readIdentifier(Stream* s, String* out, const char* pattern)
{
	readWhitespace(s);

	if(!isAlpha(s))
		return false;

	int start=s->offset;
	int i=0;

	while(isAlphaNum(s))
	{
		if(pattern!=NULL && pattern[i]!=peekStream(s,0))
			return false;

		i++;
		seekStream(s,1);
	}

	makeString(s, start, s->offset-start, out);

	return true;

}

bool readNumber(Stream* s, String* out)
{

	if(!isDigit(s))
		return false;

	int start=s->offset;

	while(isDigit(s))
	{
		seekStream(s,1);
	}
	
	makeString(s, start, s->offset-start, out);
	return true;

}


