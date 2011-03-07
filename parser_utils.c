
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "bool.h"
#include "stream.h"
#include "parser_utils.h"
#include "error.h"

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

		expect(s,terminator);

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
		parser(s,&n) or panicStream(s,"expected expresion");
		addChild(parent,n);
		n=(Node){0};
	}
	
	return true;

}

static bool parseOperatorExpresion2(Stream* s, Node* left, bool (*leaf_parser)(Stream* s, Node* out), bool (*operator_parser)(Stream*,Operator*,bool), int parentPriority, bool rightAssociative)
{

	bool expresionParser(Stream* s, Node* out)
	{
		return parseOperatorExpresion2(s,out,leaf_parser,operator_parser,INT_MAX,false);
	}


	assert(s!=NULL && left!=NULL && leaf_parser!=NULL && operator_parser);
	Operator o;

	readWhitespace(s);

	int start=s->offset;

	if(leaf_parser(s,left))
	{
	}
	else if(operator_parser(s,&o,true))
	{

		*left=(Node){.type=OPERATION};

		Node operator_node={.type=IDENTIFIER};
		makeString(s, s->offset-strlen(o.chars), strlen(o.chars), &operator_node.source);
		operator_node.value=strdup(o.name);
		addChild(left,operator_node);

		Node right={0};
		if(!parseOperatorExpresion2(s,&right,leaf_parser,operator_parser,o.priority,o.rightAssociative))
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
			if( o.priority<parentPriority || (o.priority==parentPriority && rightAssociative))
			{

				Node tmp={.type=OPERATION};

				Node operator_node={.type=IDENTIFIER};
				makeString(s, backup_offset, s->offset-backup_offset, &operator_node.source);
				operator_node.value=strdup(o.name);

				addChild(&tmp,operator_node);

				addChild(&tmp,*left);
				*left=tmp;

				if(o.closing_bracket!=NULL)
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

					expect(s,o.closing_bracket);

				}
				else if(o.type==INFIX)
				{



					Node right={0};
					if(!parseOperatorExpresion2(s,&right,leaf_parser,operator_parser,o.priority,o.rightAssociative))
						panicStream(s,"expected expresion");

					addChild(left,right);
				}


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

bool parseOperatorExpresion(Stream* s, Node* out, bool (*leaf_parser)(Stream* s, Node* out), bool (*operator_parser)(Stream*,Operator*,bool))
{
	return parseOperatorExpresion2(s,out,leaf_parser,operator_parser,INT_MAX,false);
}


