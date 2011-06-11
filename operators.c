
#include "operators.h"

#include "llvm-c/Core.h"
#include <stddef.h>

Operator operators[]=
{

	{".",       INFIX,  1, false,".",  NULL, UNKNOWN},
	{"[]",      POSTFIX,1, false,"[",  "]",  UNKNOWN},
	{"()",      POSTFIX,1, false,"(",  ")",  UNKNOWN},

	{"deref",   PREFIX, 2, false,"*",  NULL, UNKNOWN},
	{"&",       PREFIX, 2, false,"&",  NULL, UNKNOWN},

	{"prefix+", PREFIX, 2, false,"+",  NULL, UNKNOWN},
	{"prefix-", PREFIX, 2, false,"-",  NULL, UNKNOWN},

	{"*",       INFIX,  3, false,"*",  NULL, BINARY_ARITHMETIC, LLVMMul,  LLVMMul,  LLVMFMul},
	{"/",       INFIX,  3, false,"/",  NULL, BINARY_ARITHMETIC, LLVMFDiv, LLVMFDiv, LLVMFDiv},
	{"div",     INFIX,  3, false,"div",NULL, BINARY_ARITHMETIC, LLVMSDiv, LLVMUDiv, 0},
	{"mod",     INFIX,  3, false,"mod",NULL, BINARY_ARITHMETIC, LLVMSRem, LLVMURem, LLVMFRem},
	{"+",       INFIX,  4, false,"+", NULL,  BINARY_ARITHMETIC, LLVMAdd,  LLVMAdd,  LLVMFAdd},
	{"-",       INFIX,  4, false,"-", NULL,  BINARY_ARITHMETIC, LLVMSub,  LLVMSub,  LLVMFSub},

	{"<",       INFIX,  6, false,"<", NULL,  COMPARISION, LLVMIntSLT, LLVMIntULT, LLVMRealOLT},
	{">",       INFIX,  6, false,">", NULL,  COMPARISION, LLVMIntSGT, LLVMIntUGT, LLVMRealOGT},
	{"<=",      INFIX,  6, false,"<=",NULL,  COMPARISION, LLVMIntSLE, LLVMIntULE, LLVMRealOLE},
	{">=",      INFIX,  6, false,">=",NULL,  COMPARISION, LLVMIntSGE, LLVMIntUGE, LLVMRealOGE},
	{"==",      INFIX,  7, false,"==",NULL,  COMPARISION, LLVMIntEQ, LLVMIntEQ, LLVMRealOEQ},
	{"!=",      INFIX,  7, false,"!=",NULL,  COMPARISION, LLVMIntNE, LLVMIntNE, LLVMRealONE},

	{"=",       INFIX, 14, true, "=", NULL,  UNKNOWN},


	{},

};


