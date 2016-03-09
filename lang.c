
#include "error.h"
#include "parser.h"
#include "llvmgen.h"


#include <llvm-c/Core.h>
#include <llvm-c/BitWriter.h>

#include <stdio.h>
#include <string.h>

typedef struct
{
	bool dumpPT;
	bool dumpLLVM;
	bool run;
	const char* output;
}Config;

int run(Config config, int argc, const char* argv[])
{
	Stream* s=openStream(argv[0]);
	Node m;
	if(!parseModule(s,&m)) {
		closeStream(s);
		return -1;
	}
	
	if(config.dumpPT)
	{
		dumpNode(&m);
	}
		
	LLVMModuleRef llvmModule=compileModule(&m);
	closeStream(s);
	freeNode(&m);

	if(config.dumpLLVM)
	{
		LLVMDumpModule(llvmModule);
	}
	
	if(config.run)
		return runModule(llvmModule,argc,argv);
	else
	{
		
		LLVMWriteBitcodeToFile(llvmModule,config.output);
		return 0;
	}
}

int main(int argc, const char* argv[])
{

	Config config={
		.output="out.bc",
		.run=true
	};

	for(int i=1;i<argc;i++)
	{
		if(strlen(argv[i])>0)
		{
			if(argv[i][0]=='-')
			{
				if(strcmp(argv[i],"--dump-pt")==0)
				{
					config.dumpPT=true;
				}
				else if(strcmp(argv[i],"--dump-llvm")==0)
				{
					config.dumpLLVM=true;
				}
				else if(strcmp(argv[i],"--output")==0)
				{
					config.run=false;
					assert(i+1<argc);
					config.output=argv[i+1];
					i++;
				}	
				else
				{
					panic("unknown option: %s",argv[i]);
				}
			}
			else
			{
				return run(config,argc-i,&argv[i]);
			}
		}
	}

	panic("no filename specified");

}

