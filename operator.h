
#ifndef __OPERATOR_H_
#define __OPERATOR_H_

static Operator operators[]=
{
	{".",INFIX,1},

	{"*",PREFIX,2},
	{"&",PREFIX,2},

	{"²",POSTFIX,2},
	{"³",POSTFIX,2},
	{"°",POSTFIX,2},

	{"+",PREFIX,2},
	{"-",PREFIX,2},

	{"*",INFIX,3},
	{"/",INFIX,3},

	{"div",INFIX,3},
	{"mod",INFIX,3},

	{"·",INFIX,3},
	{"×",INFIX,3},

	{"+",INFIX,4},
	{"-",INFIX,4},

	{"<",INFIX,6},
	{">",INFIX,6},
	{"<=",INFIX,6},
	{">=",INFIX,6},

	{"==",INFIX,7},
	{"!=",INFIX,7},


	{"=",INFIX_RIGHT,14},
};

#endif

