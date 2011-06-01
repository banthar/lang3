

#ifndef __AST_H_
#define __AST_H_

#include "stream.h"

typedef enum
{
	NONE=0,
	IDENTIFIER,
	NUMERIC_CONSTANT,
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

typedef struct Node Node;
struct Node
{
	int type;
	String source;
	char* value;

	Node* child;
	Node* next;
};

typedef Node Module;

__attribute__((noreturn)) void panicNode(const Node* n, const char* format, ...);
void freeNode(Node* n);
void addChild(Node* parent, const Node children);
Node* getChild(const Node* parent, const int index);
int getChildrenCount(const Node* parent);
void dumpNode(Node* n);
void freeModule(Module *m);

#endif

