
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <setjmp.h>

#include "bool.h"
#include "stream.h"
#include "parser.h"
#include "error.h"
#include "parser_utils.h"

bool parseExpresion(Stream*s, Node* out);
bool parseType(Stream* s, Node* out);
bool parseDeclaration(Stream* s, Node* out);
bool parseStatement(Stream* s, Node* out);

bool parseOperator(Stream* s, Operator* out, bool prefix)
{

	for(int i=0;operators[i].name!=NULL;i++)
	{
		if((operators[i].type==PREFIX) == prefix)
			if(readString(s,operators[i].chars))
			{
				*out=operators[i];
				return true;
			}
	}

	return false;

}

bool parseIdentifier(Stream* s, Node* out)
{

	if(readIdentifier(s,&out->source))
	{
		out->type=IDENTIFIER;
		out->value=strdupString(&out->source);
		return true;
	}
	else
	{
		return false;
	}

}

bool parseSimpleExpresion(Stream* s, Node* out)
{

	if(parseIdentifier(s,out))
	{
	}
	else if(readNumber(s,&out->source))
	{
		out->type=NUMERIC_CONSTANT;
		out->value=strdupString(&out->source);
	}
	else if(readString(s,"("))
	{
		parseExpresion(s,out) or panicStream(s,"expected expresion");
		expect(s,")");
	}
	else
	{
		return false;
	}

	return true;

}

bool parseExpresion(Stream*s, Node* out)
{
	return parseOperatorExpresion(s, out, parseSimpleExpresion,parseOperator);
}

bool parseBlock(Stream* s, Node* out)
{

	readWhitespace(s);
	int start=s->offset;

	if(!readString(s,"{"))
		return false;

	out->type=BLOCK;
	parseTerminated(s,out,parseStatement,";");
	expect(s,"}");

	makeString(s, start, s->offset-start, &out->source);
	return true;

}

bool parseStatement(Stream* s, Node* out)
{

	readWhitespace(s);
	int start=s->offset;

	if(readString(s,"if"))
	{

		out->type=IF_STATEMENT;

		expect(s,"(");
		childParse(s,out,parseExpresion) or panicStream(s,"expected expresion");
		expect(s,")");
		childParse(s,out,parseStatement) or panicStream(s,"expected statement");

		if(readString(s,"else"))
		{
			childParse(s,out,parseStatement) or panicStream(s,"expected statement");
		}

	}
	else if(readString(s,"while"))
	{

		out->type=WHILE_STATEMENT;

		expect(s,"(");
		childParse(s,out,parseExpresion) or panicStream(s,"expected expresion");
		expect(s,")");
		childParse(s,out,parseStatement) or panicStream(s,"expected statement");

	}
	else if(readString(s,"for"))
	{

		out->type=FOR_STATEMENT;

		expect(s,"(");

		childParse(s,out,parseIdentifier) or panicStream(s,"expected identifier");

		expect(s,"in");
		childParse(s,out,parseExpresion) or panicStream(s,"expected expresion");
		expect(s,")");
		childParse(s,out,parseStatement) or panicStream(s,"expected statement");

	}
	else if(readString(s,"return"))
	{
		out->type=RETURN_STATEMENT;
		childParse(s,out,parseExpresion);

	}
	else if(readString(s,"break"))
	{
		out->type=BREAK_STATEMENT;
	}
	else if(readString(s,"continue"))
	{
		out->type=CONTINUE_STATEMENT;

	}
	else if(parseBlock(s,out))
	{
	}
	else if(parseDeclaration(s, out))
	{
	}
	else if(parseExpresion(s,out))
	{
	}
	else
	{
		return false;
	}

	makeString(s, start, s->offset-start, &out->source);
	return true;

}


bool parseVariableDeclaration(Stream* s, Node* out)
{

	readWhitespace(s);
	int start=s->offset;

	if(!childParse(s,out,parseIdentifier))
		return false;

	out->type = VARIABLE_DECLARATION;

	expect(s,":");

	childParse(s,out,parseType) or panicStream(s,"expected type name");

	makeString(s, start, s->offset-start, &out->source);
	return true;

}

bool parseType(Stream* s, Node* out)
{

	readWhitespace(s);
	int start=s->offset;

	if(readString(s,"struct"))
	{
		out->type=STRUCT_TYPE;
		expect(s,"{");
		parseTerminated(s,out,parseVariableDeclaration,";");
		expect(s,"}");
	}
	else if(readString(s,"function"))
	{
		out->type=FUNCTION_TYPE;
		expect(s,"(");
		parseSeparated(s,out,parseVariableDeclaration,",");
		expect(s,")");
		expect(s,":");
		childParse(s,out,parseType);

	}
	else if(parseIdentifier(s,out))
	{
	}
	else
	{
		return false;
	}

	makeString(s, start, s->offset-start, &out->source);
	return true;

}

bool parseDeclaration(Stream* s, Node* out)
{

	readWhitespace(s);
	int start=s->offset;

	if(readString(s,"type"))
	{

		out->type = TYPE_DECLARATION;
		childParse(s,out,parseIdentifier);

		expect(s,"=");

		childParse(s,out,parseType);

	}
	else if(readString(s,"var"))
	{

		out->type = VARIABLE_DECLARATION;
		parseVariableDeclaration(s,out);

		if(readString(s,"="))
		{
			childParse(s,out,parseExpresion);
		}

	}
	else if(readString(s,"function"))
	{
		out->type = FUNCTION_DECLARATION;

		childParse(s,out,parseIdentifier);

		Node type={.type=FUNCTION_TYPE};

		expect(s,"(");
		parseSeparated(s,&type,parseVariableDeclaration,",");
		expect(s,")");
		expect(s,":");
		childParse(s,&type,parseType);

		addChild(out,type);

		childParse(s,out,parseBlock);

	}
	else
	{
		return false;
	}

	makeString(s, start, s->offset-start, &out->source);
	return true;

}

Module* parseModule(Stream* s)
{

	Node *m=malloc(sizeof(Node));
	*m=(Node){0};

	jmp_buf env;

	if(setjmp(env))
	{
		freeModule(m);
		return NULL;
	}

	__attribute__((noreturn)) void errorHandler()
	{
		longjmp(env,true);
	}

	s->errorHandler=errorHandler;

	readWhitespace(s);
	int start=s->offset;

	parseTerminated(s,m,parseDeclaration,";");
	if(!isEof(s))
		panicStream(s,"expected declaration");

	makeString(s, start, s->offset-start, &m->source);


	s->errorHandler=abort;

	return m;


}


