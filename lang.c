
#include "error.h"
#include "parser.h"
#include "llvmgen.h"

#include <stdio.h>

int main(int argc, char* argv[])
{
	assert(argc==2);
	Stream* s=openStream(argv[1]);

	Module m;
	parseModule(s,&m);

/*
	Expresion e;

	parseExpresion(s,&e);
	printExpresion(&e);
	puts("");
*/

	//run(&e);

	closeStream(s);

}

