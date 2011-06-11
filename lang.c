
#include "error.h"
#include "parser.h"
#include "llvmgen.h"

#include <stdio.h>
#include <string.h>

typedef struct
{
	bool dumpPT;
	bool dumpLLVM;
	bool dontRun;
}Config;

int run(Config config, int argc, const char* argv[])
{
	Stream* s=openStream(argv[0]);
	Module* m=parseModule(s);

	if(m==NULL)
	{
		closeStream(s);
		return -1;
	}
	
	if(config.dumpPT)
	{
		dumpNode(m);
	}
		
	LLVMModuleRef llvmModule=compileModule(m);
	closeStream(s);
	freeModule(m);

	if(config.dumpLLVM)
	{
		LLVMDumpModule(llvmModule);
	}
	
	if(config.dontRun)
		return 0;
	
	return runModule(llvmModule,argc,argv);

}

int main(int argc, const char* argv[])
{

	Config config={0};

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

