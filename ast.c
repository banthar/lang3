
#include "ast.h"
#include "error.h"
#include "stream.h"

#include <stdlib.h>
#include <stdio.h>

const char* nodeTypeNames[]={
	"NONE",
	"IDENTIFIER",
	"NUMERIC_CONSTANT",
	"CHAR_CONSTANT",
	"STRING_CONSTANT",
	"OPERATION",

	"TYPE_DECLARATION",
	"VARIABLE_DECLARATION",
	"FUNCTION_DECLARATION",

	"POINTER_TYPE",
	"STRUCT_TYPE",
	"FUNCTION_TYPE",

	"IF_STATEMENT",
	"WHILE_STATEMENT",
	"FOR_STATEMENT",
	"RETURN_STATEMENT",
	"BREAK_STATEMENT",
	"CONTINUE_STATEMENT",
	"BLOCK",
};

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

__attribute__((noreturn)) void panicNode(const Node* n, const char* format, ...)
{
	assert(n!=NULL);
	dumpNode(n);
	
	va_list args;
	va_start(args,format);
	vpanicString(&n->source, format, args);	
	va_end(args);
}


void dumpNode(const Node* n)
{

	void aux(const Node* n, int depth)
	{

		for(int i=0;i<depth;i++)
			fprintf(stderr," ");

		fprintf(stderr,"%s(%s) \"",nodeTypeNames[n->type],n->value==NULL?"":n->value);

		//printString(stderr, &n->source);

		int c=' ';

		for(int i=n->source.offset;i<n->source.offset+n->source.length;i++)
		{

			if(isWhitespace(n->source.stream->data[i]))
			{
				if(!isWhitespace(c))
					fprintf(stderr," ");
			}
			else
			{
				fprintf(stderr,"%c",n->source.stream->data[i]);
			}

			c=n->source.stream->data[i];

		}

		fprintf(stderr,"\"\n");

		Node* i=n->child;

		while(i!=NULL)
		{
			aux(i,depth+1);
			i=i->next;
		}

	}

	aux(n,0);


}

void addChild(Node* parent, const Node child)
{

	assert(parent!=NULL);
	assert(parent->next==NULL);

	Node* heap_n=malloc(sizeof(Node));
	*heap_n=child;

	if(parent->child==NULL)
	{
		parent->child=heap_n;
	}
	else
	{
		Node* previous=parent->child;

		while(previous->next!=NULL)
			previous=previous->next;

		previous->next=heap_n;

	}

}

Node* getChild(const Node* parent, const int index)
{

	assert(parent!=NULL);

	Node* child=parent->child;

	for(int i=0;i<index && child!=NULL;i++)
	{
		child=child->next;
	}
	
	if(child==NULL)
	{
		panicNode(parent,"");
	}
	
	assert(child!=NULL);

	return child;

}

int getChildrenCount(const Node* parent)
{
	assert(parent!=NULL);

	Node* child=parent->child;

	int i=0;

	while(child!=NULL)
	{
		i++;
		child=child->next;
	}

	return i;

}

void freeNode(Node* n)
{
	void aux(Node* n)
	{
		if(n==NULL)
			return;
		aux(n->child);
		aux(n->next);

		if(n->value!=NULL)
			free(n->value);

		free(n);
	}

	assert(n->next==NULL);

	aux(n->child);

	*n=(Node){0};

}

void freeModule(Module* m)
{
	freeNode(m);
	free(m);
}


