
/* operators.h */

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

typedef enum
{
	OPERATOR_ELEMENT,
	OPERATOR_INDEX,
	OPERATOR_CALL,

	OPERATOR_POSTFIX_INCREMENT,
	OPERATOR_POSTFIX_DECREMENT,
	
	OPERATOR_DEREF,
	OPERATOR_ADDRESS,
	
	OPERATOR_PREFIX_INCREMENT,
	OPERATOR_PREFIX_DECREMENT,

	OPERATOR_PREFIX_PLUS,
	OPERATOR_PREFIX_MINUS,

	OPERATOR_MUL,
	OPERATOR_DIV,
	OPERATOR_IDIV,
	OPERATOR_MOD,
	OPERATOR_PLUS,
	OPERATOR_MINUS,
	
	OPERATOR_LT,
	OPERATOR_GT,
	OPERATOR_LE,
	OPERATOR_GE,
	OPERATOR_EQ,
	OPERATOR_NE,
	
	OPERATOR_ASSIGMENT,
	
	OPERATOR_LAST,
}OperatorId;

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

