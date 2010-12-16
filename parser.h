
#ifndef __PARSER_H_
#define __PARSER_H_

#include "bool.h"
#include "stream.h"
#include "string.h"

typedef enum
{
	INFIX,
	INFIX_RIGHT,
	PREFIX,
	POSTFIX,
}OperatorType;

typedef struct
{
	const char* chars;
	OperatorType type;
	int priority;	
}Operator;

typedef enum
{
	CONSTANT,
	VARIABLE,
	OPERATION,
}ExpresionType;

typedef struct Expresion Expresion;

struct Expresion
{
	ExpresionType type;
	String source;

	int args;
	Expresion* arg;

	Operator operator;

};

typedef enum
{
	TYPE_NAME,
	STRUCT,
	FUNCTION,
	UNION,
}TypeType;

typedef struct Type Type;
typedef struct NamedType NamedType;

struct Type
{
	TypeType type;
	String source;

	int args;
	NamedType* arg;
};

struct NamedType
{
	String name;
	Type type;
};

typedef struct
{
	String name;
	Type type;
}TypeDeclaration;

typedef struct
{
	String name;
}Module;


typedef struct
{
}Statement;

void parseWhitespace(Stream* s);
void expectString(Stream* s, const char* pattern);
bool parseString(Stream* s, const char* pattern);
bool parseVariable(Stream* s, Expresion* out);
bool parseConstant(Stream* s, Expresion* out);
bool peekOperator(Stream* s, Operator* out, bool prefix);
bool parseOperator(Stream* s, Operator* out, bool prefix);
bool parseExpresion4(Stream* s, Expresion* out, int parentPriority, bool rightAssociative);
bool parseExpresion(Stream* s, Expresion* out);
void printExpresion(Expresion* e);
bool parseNamedType(Stream* s, NamedType* out, bool name);
void parseTypeArgs(Stream* s, Type* out, const char* separator, bool with_names);
bool parseType(Stream* s, Type* out);
bool parseDeclaration(Stream* s, Statement* out);
bool parseStatement(Stream* s, Statement* out);
bool parseBlock(Stream* s, Statement* out);
bool parseModule(Stream* s, Module* out);


#endif
