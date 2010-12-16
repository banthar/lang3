
#include "llvm-c/Core.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/BitWriter.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Transforms/Scalar.h"

#include "llvmgen.h"
#include "error.h"

LLVMValueRef llvmExpresionValue(Expresion *e)
{

	switch(e->type)
	{
		case CONSTANT:
			return LLVMConstIntOfStringAndSize(LLVMInt32Type(), e->source.stream->data+e->source.offset, e->source.length, 10);
		case VARIABLE:
			return NULL;
		case OPERATION:
			return LLVMConstAdd(llvmExpresionValue(&e->arg[0]),llvmExpresionValue(&e->arg[1]));
	}

	panic("error",0);

}

void run(Expresion *e)
{

	LLVMModuleRef module=LLVMModuleCreateWithName("main");

	LLVMValueRef f=LLVMAddFunction(module, "main", LLVMFunctionType(LLVMInt32Type(), NULL,0,false));
	LLVMSetFunctionCallConv(f, LLVMFastCallConv);
	LLVMSetLinkage(f,LLVMExternalLinkage);

	LLVMBasicBlockRef bblock=LLVMAppendBasicBlock(f, "entry");
	LLVMBuilderRef builder=LLVMCreateBuilder();
	LLVMPositionBuilderAtEnd(builder, bblock);

	LLVMBuildRet(builder,llvmExpresionValue(e));

	LLVMVerifyModule(module,LLVMAbortProcessAction,NULL);
	LLVMDumpModule(module);

}


