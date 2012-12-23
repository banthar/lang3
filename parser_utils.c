
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

#include "stream.h"
#include "parser_utils.h"
#include "error.h"

#define or ?0:

static bool isAlpha(int c)
{
	switch(c)
	{
		case '_':
		case 'A'...'Z':
		case 'a'...'z':
			return true;
		default:
			return false;
	}
}

static bool isDigit(int c)
{
	switch(c)
	{
		case '0'...'9':
			return true;
		default:
			return false;
	}
}

static bool isAlphaNum(int c)
{
	return isAlpha(c) || isDigit(c);
}

static bool isWhitespace(int c)
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


static bool isString(Stream* s, const char* string)
{
	for(int i=0;i<strlen(string);i++)
		if(peekStream(s,i)!=string[i])
			return false;

	return true;
}

/* whitespace parsers */

static bool readComment(Stream* s, String* out)
{

	assert(out==NULL);//TODO

	if(isString(s,"/*"))
	{

		seekStream(s,2);

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

		return true;

	}
	else if(isString(s,"//"))
	{
		seekStream(s,2);

		while(!isString(s,"\n") && !isEof(s))
		{
			seekStream(s,1);
		}

		seekStream(s,1);

		return true;

	}
	else if(isString(s,"#"))
	{
		seekStream(s,1);

		while(!isString(s,"\n") && !isEof(s))
		{
			seekStream(s,1);
		}

		seekStream(s,1);

		return true;

	}	
	else
	{
		return false;
	}

}

bool readWhitespace(Stream* s)
{

	bool aux()
	{
		if(!isWhitespace(peekStream(s,0)))
			return false;

		while(isWhitespace(peekStream(s,0)))
			seekStream(s,1);

		return true;

	}

	bool ret=false;

	while(aux(s) || readComment(s,NULL))
	{
		ret=true;
	}

	return ret;

}

bool readString(Stream* s, const char* pattern)
{

	assert(s!=NULL);
	assert(pattern!=NULL);
	assert(strlen(pattern)!=0);

	readWhitespace(s);

	for(int i=0;i<strlen(pattern);i++)
	{
		if(peekStream(s,i)!=pattern[i])
			return false;
	}

	if(isAlphaNum(pattern[strlen(pattern)-1]) && isAlphaNum(peekStream(s,strlen(pattern))))
	{
		return false;
	}

	seekStream(s,strlen(pattern));

	return true;

}

void expect(Stream* s, const char* pattern)
{

	if(!readString(s,pattern))
	{
		panicStream(s,"expected '%s'",pattern);
	}

}

/* basic parsers */


bool readIdentifier(Stream* s, String* out)
{

	assert(s!=NULL);

	readWhitespace(s);

	if(!isAlpha(peekStream(s,0)))
		return false;

	int start=s->offset;
	int i=0;

	while(isAlphaNum(peekStream(s,0)))
	{
		i++;
		seekStream(s,1);
	}

	if(out!=NULL)
		makeString(s, start, s->offset-start, out);

	return true;

}

bool readNumber(Stream* s, String* out)
{

	readWhitespace(s);

	if(!isDigit(peekStream(s,0)))
		return false;

	int start=s->offset;

	while(isDigit(peekStream(s,0)))
	{
		seekStream(s,1);
	}
	
	makeString(s, start, s->offset-start, out);
	return true;

}

bool readStringConstant(Stream*s, String* out)
{
	
	readWhitespace(s);
	
	if(peekStream(s,0)!='"')
		return false;
	
	int start=s->offset;

	seekStream(s,1);

	while(peekStream(s,0)!='"')
	{
		seekStream(s,1);
		if(peekStream(s,0)=='\0')
			panicStream(s,"expected '\"' not EOF");
	}
	
	seekStream(s,1);
	
	makeString(s, start, s->offset-start, out);
	return true;
	
}

bool readCharConstant(Stream*s, String* out)
{
	
	readWhitespace(s);
	
	if(peekStream(s,0)!='\'')
		return false;
	
	int start=s->offset;

	seekStream(s,1);

	while(peekStream(s,0)!='\'')
	{
		seekStream(s,1);
		if(peekStream(s,0)=='\0')
			panicStream(s,"expected ''' not EOF");
	}
	
	seekStream(s,1);
	
	makeString(s, start, s->offset-start, out);
	return true;
	
}

char* unquoteString(const String* s)
{
	//TODO: move to parser
	
	char* escaped=strndupa(s->stream->data+s->offset+1,s->length-2);
	char* unescaped=malloc(s->length-2+1);

	int j=0;

	for(int i=0;i<strlen(escaped);i++)
	{

		switch(escaped[i])
		{
			case '\\':
				switch(escaped[i+1])
				{
					case '\\':
						unescaped[j]='\\';
						j++;
						i++;
						break;
					case 'n':
						unescaped[j]='\n';
						j++;
						i++;
						break;
					default:
						panicString(s,"\\ error");
				}
				break;
			default:
				unescaped[j]=escaped[i];
				j++;
				break;
		}

	}

	unescaped[j]='\0';

	return unescaped;
}

/* helpers */

bool childParse(Stream* s, Node* parent, bool (*parser)(Stream* s, Node* out))
{
	Node n={0};

	if(!parser(s,&n))
		return false;

	addChild(parent,n);

	return true;

}

bool parseTerminated(Stream* s, Node* parent, bool (*parser)(Stream* s, Node* out), const char* terminator)
{

	Node n={0};

	while(parser(s,&n))
	{

		addChild(parent,n);
		n=(Node){0};

		readString(s,terminator);

	}
	
	return true;
}

bool parseSeparated(Stream* s, Node* parent, bool (*parser)(Stream* s, Node* out), const char* separator)
{

	Node n={0};

	if(parser(s,&n))
	{
		addChild(parent,n);
		n=(Node){0};
	}
	else
	{
		return true;
	}

	while(readString(s,separator))
	{
		parser(s,&n) or panicStream(s,"unexpected");
		addChild(parent,n);
		n=(Node){0};
	}
	
	return true;

}

static bool parseOperatorExpresion2(Stream* s, Node* left, bool (*leaf_parser)(Stream* s, Node* out), bool (*operator_parser)(Stream*,OperatorId*,bool), int parentPriority, bool rightAssociative)
{

	bool expresionParser(Stream* s, Node* out)
	{
		return parseOperatorExpresion2(s,out,leaf_parser,operator_parser,INT_MAX,false);
	}


	assert(s!=NULL && left!=NULL && leaf_parser!=NULL && operator_parser);
	OperatorId o;

	readWhitespace(s);

	int start=s->offset;

	if(leaf_parser(s,left))
	{
	}
	else if(operator_parser(s,&o,true))
	{

		*left=(Node){
			.type=OPERATION,
			.operator=o,
		};

		Node right={0};
		if(!parseOperatorExpresion2(s,&right,leaf_parser,operator_parser,operators[o].priority,operators[o].rightAssociative))
			panicStream(s,"expected expresion");

		addChild(left,right);

	}
	else
	{
		return false;
	}

	while(true)
	{

		int backup_offset=s->offset;

		if(operator_parser(s,&o,false))
		{
			if( operators[o].priority<parentPriority || (operators[o].priority==parentPriority && rightAssociative))
			{

				Node tmp=(Node){
					.type=OPERATION,
					.operator=o,
				};

				addChild(&tmp,*left);

				*left=tmp;

				if(operators[o].closing_bracket!=NULL)
				{

					while(true)
					{
						Node right={0};
						if(!parseOperatorExpresion(s,&right,leaf_parser,operator_parser))
							break;
						addChild(left,right);

						if(!readString(s,","))
							break;

					}

					expect(s,operators[o].closing_bracket);

				}
				else if(operators[o].type==INFIX)
				{

					Node right={0};
					if(!parseOperatorExpresion2(s,&right,leaf_parser,operator_parser,operators[o].priority,operators[o].rightAssociative))
						panicStream(s,"expected expresion");

					addChild(left,right);
				}

				makeString(s, start, s->offset-start, &left->source);

				continue;

			}
			else
			{
				s->offset=backup_offset;
			}

		}

		int i=s->offset;
		while(i>start && isWhitespace(getStream(s,i)))
			i--;

		makeString(s, start, i-start, &left->source);

		return true;

	}

}

bool parseOperatorExpresion(Stream* s, Node* out, bool (*leaf_parser)(Stream* s, Node* out), bool (*operator_parser)(Stream*,OperatorId*,bool))
{
	return parseOperatorExpresion2(s,out,leaf_parser,operator_parser,INT_MAX,false);
}


