

#include "error.h"

#include "llvm_utils.h"

#include <llvm-c/Core.h>
#include <llvm/IR/Constants.h>

extern "C" unsigned LLVMConstIntValue(LLVMValueRef v)
{
	llvm::ConstantInt* cint=llvm::dyn_cast_or_null<llvm::ConstantInt>(llvm::unwrap(v));
	
	assert(cint!=NULL);
	
	return cint->getLimitedValue();
}
