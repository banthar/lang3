
#include "error.h"
#include "parser.h"
#include "llvmgen.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{


	assert(argc==3);

	assert(strlen(argv[1])==2);
	assert(argv[1][0]=='-');

	Stream* s=openStream(argv[2]);
	Module* m=parseModule(s);

	if(m==NULL)
	{
		closeStream(s);
		return 1;
	}

	switch(argv[1][1])
	{
		case 'c':
			compileModule(m);
			break;
		case 'd':
			dumpNode(m);
			break;
		case 'r':
			{
				LLVMModuleRef llvmModule=compileModule(m);
				runModule(llvmModule);
			}
			break;
		default:
			panic("unknown action -%c",argv[1][1]);
	}

	freeModule(m);
	closeStream(s);
	return 0;

}

