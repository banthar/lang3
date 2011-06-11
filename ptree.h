

#ifndef __PTREE_H_
#define __PTREE_H_

#include "stream.h"
#include "operators.h"

typedef enum
{
	NONE,
	IDENTIFIER,
	OPERATOR,
	NUMERIC_CONSTANT,
	CHAR_CONSTANT,
	STRING_CONSTANT,
	OPERATION,

	TYPE_DECLARATION,
	VARIABLE_DECLARATION,
	FUNCTION_DECLARATION,

	POINTER_TYPE,
	STRUCT_TYPE,
	FUNCTION_TYPE,

	IF_STATEMENT,
	WHILE_STATEMENT,
	FOR_STATEMENT,
	RETURN_STATEMENT,
	BREAK_STATEMENT,
	CONTINUE_STATEMENT,
	BLOCK,
}NodeType;

extern const char* nodeTypeNames[];

typedef struct Node Node;
struct Node
{
	int type;
	String source;
	char* value;
	OperatorId operator;
	
	Node* child;
	Node* next;
};

typedef Node Module;

__attribute__((noreturn)) void panicNode(const Node* n, const char* format, ...);
void freeNode(Node* n);
void addChild(Node* parent, const Node children);
Node* getChild(const Node* parent, const int index);
int getChildrenCount(const Node* parent);
void dumpNode(const Node* n);
void freeModule(Module *m);

#endif

