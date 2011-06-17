
#pragma once

#include <llvm-c/Core.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned LLVMConstIntValue(LLVMValueRef v);

#ifdef __cplusplus
}
#endif
