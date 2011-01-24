
#ifndef __OPERATORS_H_
#define __OPERATORS_H_

#include "bool.h"

typedef enum
{
	INFIX,
	PREFIX,
	POSTFIX,
}OperatorType;

typedef enum
{
	UNKNOWN,
	UNARY_ARITHMETIC,
	BINARY_ARITHMETIC,
	COMPARISION,	
	UNARY_LOGICAL,
	BINARY_LOGICAL,
}OperatorCategory;

typedef struct
{
	const char* name;
	OperatorType type;
	int priority;
	bool rightAssociative;
	const char* chars;
	const char* closing_bracket;

	OperatorCategory category;
	int int_opcode;
	int uin_opcode;
	int float_opcode;

}Operator;

extern Operator operators[];

#endif

