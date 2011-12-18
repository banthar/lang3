
/* llvmgen.h */

#ifndef __LLVMGEN_H_
#define __LLVMGEN_H_

#include "parser.h"

#include "llvm-c/Core.h"

LLVMModuleRef compileModule(Module *module);
int runModule(LLVMModuleRef llvmModule, int argc, const char*argv[]);

#endif

