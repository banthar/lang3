
#include "operators.h"

#include "llvm-c/Core.h"
#include <stddef.h>

Operator operators[]=
{

	{".",       INFIX,  1, false,".",  NULL, UNKNOWN},
	{"[]",      POSTFIX,1, false,"[",  "]",  UNKNOWN},
	{"()",      POSTFIX,1, false,"(",  ")",  UNKNOWN},

	{"postfix++", POSTFIX, 1, false,"++",  NULL, UNKNOWN},
	{"postfix--", POSTFIX, 1, false,"--",  NULL, UNKNOWN},

	{"deref",   PREFIX, 2, false,"*",  NULL, UNKNOWN},
	{"&",       PREFIX, 2, false,"&",  NULL, UNKNOWN},

	{"prefix+", PREFIX, 2, false,"+",  NULL, UNKNOWN},
	{"prefix-", PREFIX, 2, false,"-",  NULL, UNKNOWN},

	{"prefix++", PREFIX, 2, false,"++",  NULL, UNKNOWN},
	{"prefix--", PREFIX, 2, false,"--",  NULL, UNKNOWN},

	{"*",       INFIX,  3, false,"*",  NULL, BINARY_ARITHMETIC},
	{"/",       INFIX,  3, false,"/",  NULL, BINARY_ARITHMETIC},
	{"div",     INFIX,  3, false,"div",NULL, BINARY_ARITHMETIC},
	{"mod",     INFIX,  3, false,"mod",NULL, BINARY_ARITHMETIC},
	{"+",       INFIX,  4, false,"+", NULL,  BINARY_ARITHMETIC},
	{"-",       INFIX,  4, false,"-", NULL,  BINARY_ARITHMETIC},

	{"<",       INFIX,  6, false,"<", NULL,  COMPARISION},
	{">",       INFIX,  6, false,">", NULL,  COMPARISION},
	{"<=",      INFIX,  6, false,"<=",NULL,  COMPARISION},
	{">=",      INFIX,  6, false,">=",NULL,  COMPARISION},
	{"==",      INFIX,  7, false,"==",NULL,  COMPARISION},
	{"!=",      INFIX,  7, false,"!=",NULL,  COMPARISION},

	{"=",       INFIX, 14, true, "=", NULL,  UNKNOWN},


	{},

};


