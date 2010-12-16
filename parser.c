
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "parser.h"
#include "error.h"
#include "operator.h"

void parseWhitespace(Stream* s)
{
	while(readWhitespace(s) || readComment(s,NULL))
	{
	}
}

void expectString(Stream* s, const char* pattern)
{
	parseWhitespace(s);
	if(!readString(s,pattern))
		panicStream(s,"expected '%s'",pattern);
}

bool parseString(Stream* s, const char* pattern)
{
	parseWhitespace(s);
	return readString(s,pattern);
}

bool parseIdentifier(Stream* s, String* out, const char* pattern)
{
	parseWhitespace(s);

	return readIdentifier(s,out,pattern);
}

bool parseVariable(Stream* s, Expresion* out)
{

	assert(out!=NULL);

	if(parseIdentifier(s, &(out->source),NULL))
	{
		out->type=VARIABLE;
		return true;
	}
	else
	{
		return false;
	}

}

bool parseConstant(Stream* s, Expresion* out)
{

	assert(out!=NULL);

	parseWhitespace(s);

	if(readNumber(s, &(out->source)))
	{
		out->type=CONSTANT;
		return true;
	}
	else
	{
		return false;
	}

}

bool peekOperator(Stream* s, Operator* out, bool prefix)
{

	assert(out!=NULL);

	parseWhitespace(s);

	for(int i=0;i<sizeof(operators)/sizeof(Operator);i++)
	{

		if(prefix != (operators[i].type==PREFIX))
			continue;

		if(isString(s,operators[i].chars))
		{

			if(out!=NULL)
				*out=operators[i];

			return true;

		}

	}

	return false;

}

bool parseOperator(Stream* s, Operator* out, bool prefix)
{

	if(peekOperator(s, out, prefix))
	{
		seekStream(s,strlen(out->chars));
		return true;
	}

	return false;

}

bool parseExpresion4(Stream* s, Expresion* out, int parentPriority, bool rightAssociative)
{

	assert(out!=NULL);

	Expresion e;

	if(parseConstant(s, &e))
	{
	}
	else if(parseVariable(s, &e))
	{
	}
	else if(parseString(s,"("))
	{

		if(!parseExpresion(s,&e))
			panicStream(s,"expected expresion");

		expectString(s,")");

	}
	else if(parseOperator(s,&(e.operator),true))
	{

		e.type=OPERATION;
		e.args=1;
		e.arg=malloc(sizeof(Expresion)*e.args);

		if(!parseExpresion4(s,&(e.arg[0]),e.operator.priority,e.operator.type==INFIX_RIGHT))
			panicStream(s,"expected expresion");

	}
	else
	{
		return false;
	}

	while(true)
	{

		Expresion o;

		if(peekOperator(s,&o.operator,false) && ( o.operator.priority<parentPriority || (o.operator.priority==parentPriority && rightAssociative)) )
		{

			parseOperator(s,&o.operator,false);

			o.type=OPERATION;
			o.args=o.operator.type==POSTFIX?1:2;
			o.arg=malloc(sizeof(Expresion)*o.args);

			o.arg[0]=e;

			if(o.args>1)
				if(!parseExpresion4(s,&(o.arg[1]),o.operator.priority,o.operator.type==INFIX_RIGHT))
					panicStream(s,"expected expresion");

			e=o;


		}
		else if(parseString(s,"("))
		{

			o.type=OPERATION;
			o.operator.chars="call";
			o.args=1;
			o.arg=malloc(sizeof(Expresion)*o.args);

			o.arg[0]=e;

			while(true)
			{
				
				o.args++;
				o.arg=realloc(o.arg,sizeof(Expresion)*o.args);

				if(!parseExpresion(s,&(o.arg[o.args-1])))
					panicStream(s,"expected expresion");

				if(parseString(s,")"))
					break;

				if(!parseString(s,","))
					panicStream(s,"expected ',' or ')'");

			}

			e=o;			

		}
		else
		{
			*out=e;
			return true;
		}

	}

}

bool parseExpresion(Stream* s, Expresion* out)
{
	return parseExpresion4(s, out, INT_MAX, false);
}

void printExpresion(Expresion* e)
{

	switch(e->type)
	{
		case CONSTANT:
			printString(&e->source);
			break;
		case VARIABLE:
			printString(&e->source);
			break;
		case OPERATION:
			printf("(%s",e->operator.chars);
			for(int i=0;i<e->args;i++)
			{
				printf(", ");
				printExpresion(&(e->arg[i]));
			}
			printf(")");
			break;
	}
}

bool parseNamedType(Stream* s, NamedType* out, bool name)
{

	if(name)
	{

		if(!parseIdentifier(s,&out->name,NULL))
			return false;

		expectString(s,":");

		if(!parseType(s,&out->type))
			panicStream(s,"expected type");

		return true;

	}
	else
	{
		makeString(s, s->offset, s->offset, &out->name);
		return parseType(s,&out->type);

	}

}

void parseTypeArgs(Stream* s, Type* out, const char* separator, bool with_names)
{

	while(true)
	{

		NamedType n;

		if(parseNamedType(s,&n,with_names))
		{
			out->args++;

			if(out->arg==NULL)
				out->arg=malloc(sizeof(NamedType)*out->args);
			else
				out->arg=realloc(out->arg,sizeof(NamedType)*out->args);

			out->arg[out->args-1]=n;

			if(!parseString(s,separator))
				break;

		}
		else
		{
			break;
		}

	}

}

bool parseType(Stream* s, Type* out)
{

	out->args=0;
	out->arg=NULL;

	if(parseIdentifier(s,NULL,"struct"))
	{
		expectString(s, "{");
		parseTypeArgs(s,out,";",true);
		expectString(s, "}");
	}
	else if(parseIdentifier(s,NULL,"union"))
	{
		expectString(s, "{");
		parseTypeArgs(s,out,";",false);
		expectString(s, "}");
	}
	else if(parseIdentifier(s,NULL,"function"))
	{
		expectString(s, "(");
		parseTypeArgs(s,out,",",true);
		expectString(s, ")");

		expectString(s, ":");

		out->args++;
		out->arg=realloc(out->arg,sizeof(NamedType)*out->args);

		if(!parseNamedType(s,&out->arg[out->args-1],false))
			panicStream(s,"expected return type");


	}
	else if(parseIdentifier(s,&out->source,NULL))
	{
		out->type=TYPE_NAME;
	}
	else
	{
		return false;
	}

	return true;

}

bool parseDeclaration(Stream* s, Statement* out)
{

	if(parseIdentifier(s,NULL,"type"))
	{
		Type t;

		if(!parseIdentifier(s,NULL,NULL))
			panicStream(s,"expected type name");

		expectString(s,"=");

		if(!parseType(s,&t))
			panicStream(s,"expected type");

	}
	else if(parseIdentifier(s,NULL,"var"))
	{
		NamedType n;
		parseNamedType(s,&n,true);

		if(parseString(s,"="))
		{
			Expresion e;
			parseExpresion(s,&e);
		}

	}
	else if(parseIdentifier(s,NULL,"function"))
	{
		NamedType n;

		if(!parseIdentifier(s,&n.name,NULL))
			panicStream(s,"expected function name");

		expectString(s, "(");
		parseTypeArgs(s,&n.type,",",true);
		expectString(s, ")");

		expectString(s, ":");

		n.type.args++;
		n.type.arg=realloc(n.type.arg,sizeof(NamedType)*n.type.args);

		if(!parseNamedType(s,&n.type.arg[n.type.args-1],false))
			panicStream(s,"expected return type");

		Statement b;

		parseBlock(s,&b);


	}
	else
	{
		return false;
	}

	return true;

}

bool parseStatement(Stream* s, Statement* out)
{

	Expresion ex;

	if(parseDeclaration(s,out))
	{
		return true;
	}
	else if(parseString(s,"return"))
	{
		Expresion e;
		parseExpresion(s,&e);
		return true;
	}
	else if(parseExpresion(s,&ex))
	{
		return true;
	}
	else
	{
		return false;
	}

}

bool parseBlock(Stream* s, Statement* out)
{

	if(!parseString(s,"{"))
		return false;

	Statement statement;

	while(parseStatement(s,&statement))
	{
		expectString(s,";");
	}

	expectString(s,"}");

}

bool parseModule(Stream* s, Module* out)
{

	if(parseIdentifier(s,NULL,"package"))
	{
		parseIdentifier(s,&out->name,NULL);
	}

	while(true)
	{

		Statement d;

		if(parseDeclaration(s,&d))
		{
			expectString(s,";");
		}
		else if(isEof(s))
		{
			return true;
		}
		else
		{
			panicStream(s,"expected declaration");
		}
	
	}

}


