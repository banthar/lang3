
#include <stdlib.h>
#include <string.h>

#include "llvm-c/Core.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/BitWriter.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Transforms/Scalar.h"

#include "llvmgen.h"
#include "error.h"
#include "operators.h"

typedef struct Context Context;

typedef struct
{
	Node* declaration;
	LLVMValueRef llvm_value;
}Variable;

typedef struct
{
	Node* declaration;
	LLVMTypeRef llvm_type;
}Type;

struct Context
{
	LLVMModuleRef module;
	LLVMBuilderRef builder;
	LLVMValueRef function;
	LLVMBasicBlockRef block;

	Context* parent;

	int variables;
	Variable* variable;

	int types;
	Type* type;
};

static Context pushContext(Context *ctx);
static Context* popContext(Context *ctx);
static void addVariable(Context *ctx, Variable variable);
static void addType(Context *ctx, Type type);
static Variable* getVariable(const Context* ctx, const char* name);
static Type* getType(const Context* ctx, const char* name);
void llvmBuildStatement(Context* ctx, Node* n);
void llvmBuildFunction(Context* ctx, Node* f);
LLVMTypeRef llvmBuildType(Context* ctx, Node* n);
void llvmGenHeader(Context* ctx, Node* n);
LLVMValueRef llvmBuildExpresion(Context* ctx, Node* n);
LLVMValueRef llvmBuildLExpresion(Context* ctx, Node* n);


static Context pushContext(Context *ctx)
{

	Context new_context={
		.parent=ctx,
		.module=ctx->module,
		.builder=ctx->builder,
		.function=ctx->function,
		.block=ctx->block,
	};

	return new_context;

}

static Context* popContext(Context *ctx)
{

	if(ctx->variable!=NULL)
		free(ctx->variable);

	if(ctx->type!=NULL)
		free(ctx->type);

	ctx->parent->builder=ctx->builder;

	*ctx=(Context){0};

	return ctx->parent;
}

static void addVariable(Context *ctx, Variable variable)
{

    if((ctx->variable==NULL) != (ctx->variables==0))
        panic("illegal state",0);

    ctx->variables++;

    if(ctx->variable==NULL)
	    ctx->variable=malloc(sizeof(Variable)*ctx->variables);
    else
	    ctx->variable=realloc(ctx->variable,sizeof(Variable)*ctx->variables);

    assert(ctx->variable!=NULL);

    ctx->variable[ctx->variables-1]=variable;

}

static void addType(Context *ctx, Type type)
{

    if((ctx->type==NULL) != (ctx->types==0))
        panic("illegal state",0);

    ctx->types++;

    if(ctx->type==NULL)
	    ctx->type=malloc(sizeof(Type)*ctx->types);
    else
	    ctx->type=realloc(ctx->type,sizeof(Type)*ctx->types);

    assert(ctx->type!=NULL);

    ctx->type[ctx->types-1]=type;
}

static Variable* getVariable(const Context* ctx, const char* name)
{

	if(ctx==NULL || name==NULL)
		return NULL;

	for(int i=0;i<ctx->variables;i++)
	{

		Node* declaration=ctx->variable[i].declaration;

		if(strcmp(name,getChild(declaration,0)->value)==0)
			return &ctx->variable[i];
	}

	return getVariable(ctx->parent,name);
}

static Type* getType(const Context* ctx, const char* name)
{
	
	if(ctx==NULL || name==NULL)
		return NULL;

	for(int i=0;i<ctx->types;i++)
	{
		Node* declaration=ctx->type[i].declaration;

		if(strcmp(name,getChild(declaration,0)->value)==0)
			return &ctx->type[i];

	}

	return getType(ctx->parent,name);
}

static Type* getTypeById(const Context* ctx, LLVMTypeRef id)
{
	
	if(ctx==NULL || id==NULL)
		return NULL;

	for(int i=0;i<ctx->types;i++)
	{
		if(ctx->type[i].llvm_type==id)
			return &ctx->type[i];
	}

	return getTypeById(ctx->parent,id);
}

LLVMValueRef llvmBuildLOperation(Context* ctx, Node* n)
{

	LLVMValueRef larg(int index)
	{
		assert(getChildrenCount(n)>index);

		return llvmBuildLExpresion(ctx,getChild(n,index+1));

	}

	if(strcmp(getChild(n,0)->value,".")==0)
	{
		LLVMValueRef left=larg(0);

		LLVMTypeRef left_type=LLVMTypeOf(left);

		assert(LLVMGetTypeKind(left_type)==LLVMPointerTypeKind);

		LLVMTypeRef struct_type=LLVMGetElementType(left_type);

		assert(LLVMGetTypeKind(struct_type)==LLVMStructTypeKind);

		Type* t=getTypeById(ctx,struct_type);

		//

		return LLVMBuildStructGEP(ctx->builder, left,0,cstrString(&n->source));
	}

	panicNode(n,"not a l-value");

}

LLVMValueRef llvmBuildOperation(Context* ctx, Node* n)
{

	LLVMValueRef arg(int index)
	{
		assert(getChildrenCount(n)>index);

		return llvmBuildExpresion(ctx,getChild(n,index+1));

	}

	LLVMValueRef larg(int index)
	{
		assert(getChildrenCount(n)>index);

		return llvmBuildLExpresion(ctx,getChild(n,index+1));

	}

	if(strcmp(getChild(n,0)->value,"prefix+")==0)
	{
		return arg(0);
	}
	else if(strcmp(getChild(n,0)->value,"prefix-")==0)
	{
		return LLVMBuildNeg(ctx->builder,arg(0),cstrString(&n->source));
	}
	else if(strcmp(getChild(n,0)->value,"()")==0)
	{

		int num_args=getChildrenCount(n)-2;
		LLVMValueRef args[num_args];

		for(int i=0;i<num_args;i++)
		{
			args[i]=arg(i+1);
		}

		return LLVMBuildCall(ctx->builder, larg(0), args,num_args,cstrString(&n->source));

	}
	else if(strcmp(getChild(n,0)->value,".")==0)
	{
		return LLVMBuildLoad(ctx->builder,llvmBuildLExpresion(ctx,n),cstrString(&n->source));
	}

	for(Operator* o=operators;o->name!=NULL;o++)
	{
		if(strcmp(o->name,getChild(n,0)->value)==0)
		{
			switch(o->category)
			{
				case BINARY_ARITHMETIC:
					return LLVMBuildBinOp(ctx->builder, o->int_opcode,arg(0),arg(1),cstrString(&n->source));
				case COMPARISION:
					return LLVMBuildICmp(ctx->builder, o->int_opcode, arg(0),arg(1),cstrString(&n->source));
				default:
					break;
			}
		}
	}

	panicNode(n,"operator '%s' not implemented",getChild(n,0)->value);

}

LLVMValueRef llvmBuildLExpresion(Context* ctx, Node* n)
{
	switch(n->type)
	{
		case IDENTIFIER:
		{

			if(ctx->builder==NULL)
				panicNode(n,"expresion has to be constant");

			Variable* v=getVariable(ctx, n->value);

			if(v==NULL)
				panicNode(n,"'%s' undeclared",n->value);

			return v->llvm_value;

		}
		case OPERATION:
			return llvmBuildLOperation(ctx,n);
		default:
			panicNode(n,"not an L-expresion");
	}
}

LLVMValueRef llvmBuildExpresion(Context* ctx, Node* n)
{


	switch(n->type)
	{
		case NUMERIC_CONSTANT:
			return LLVMConstIntOfString(LLVMInt32Type(), n->value, 10);
		case IDENTIFIER:
			return LLVMBuildLoad(ctx->builder,llvmBuildLExpresion(ctx,n),n->value);
		case OPERATION:
			return llvmBuildOperation(ctx,n);
		default:
			panicNode(n,"not an expresion");
	}

}

void llvmBuildStatement(Context* ctx, Node* n)
{

	if(ctx->builder==NULL)
		panicNode(n,"unreachable code");

	switch(n->type)
	{

		case OPERATION:
			llvmBuildExpresion(ctx,n);
			break;
		case VARIABLE_DECLARATION:
			{

				Variable v;

				v.declaration=n;
				v.llvm_value=LLVMBuildAlloca(ctx->builder, llvmBuildType(ctx,getChild(n,1)), getChild(n,0)->value );

				if(getChildrenCount(n)>1)
					LLVMBuildStore(ctx->builder,llvmBuildExpresion(ctx,getChild(n,1)),v.llvm_value);

				addVariable(ctx,v);

			}
			break;
		case BLOCK:
			{

				Context block_ctx=pushContext(ctx);

				for(int i=0;i<getChildrenCount(n);i++)
				{
					llvmBuildStatement(&block_ctx,getChild(n,i));
				}

				popContext(&block_ctx);

			}
			break;
		case RETURN_STATEMENT:
			{

				if(getChildrenCount(n)==0)
					LLVMBuildRetVoid(ctx->builder);
				else
					LLVMBuildRet(ctx->builder,llvmBuildExpresion(ctx,getChild(n,0)));
			
				LLVMDisposeBuilder(ctx->builder);
				ctx->builder=NULL;

			}
			break;
		default:
			{
				dumpNode(n);
				panicString(&n->source, "not a statement");
			}
	}
}


LLVMTypeRef llvmBuildType(Context* ctx, Node* n)
{
	switch(n->type)
	{

		case STRUCT_TYPE:
			{
				LLVMTypeRef llvm_args[getChildrenCount(n)];

				for(int i=0;i<getChildrenCount(n);i++)
				{
					llvm_args[i]=llvmBuildType(ctx,getChild(getChild(n,i),1));
				}

				return LLVMStructType(llvm_args, getChildrenCount(n),false);
			}
			break;
		case POINTER_TYPE:
			{
				if(getChildrenCount(n)!=1)
					panicNode(n,"Pointer type needs one argument");
				return LLVMPointerType(llvmBuildType(ctx,getChild(n,0)),0);
			}
			break;
		case IDENTIFIER:
			{

				LLVMTypeRef llvm_type=LLVMGetTypeByName(ctx->module,n->value);

				if(llvm_type!=NULL)
				{
					if(getChildrenCount(n)!=0)
						panicNode(n,"type doesn't accept any arguments");

					return llvm_type;
				}
				else
					panicNode(n,"unknown type '%s'",n->value);

			}
			break;
		case FUNCTION_TYPE:
			{
				LLVMTypeRef llvm_args[getChildrenCount(n)-1];

				for(int i=0;i<getChildrenCount(n)-1;i++)
				{
					llvm_args[i]=llvmBuildType(ctx,getChild(getChild(n,i),1));
				}

				LLVMTypeRef ret=llvmBuildType(ctx,getChild(n,getChildrenCount(n)-1));

				return LLVMFunctionType(ret, llvm_args,getChildrenCount(n)-1,false);
			}
			break;
		default:
			dumpNode(n);
			panicString(&n->source,"not a type");
	}

	assert(false);

}

void llvmDeclareType(Context* ctx, Node* n)
{

	if(n->type == TYPE_DECLARATION)
	{

		Type t={
			.declaration=n,
			.llvm_type=LLVMOpaqueType(),
		};

		LLVMAddTypeName(ctx->module,getChild(n,0)->value,t.llvm_type);

		addType(ctx,t);

	}

}

void llvmDefineType(Context* ctx, Node* n)
{
	if(n->type == TYPE_DECLARATION)
	{
		Type* t=getType(ctx,getChild(n,0)->value);

		assert(t!=NULL);

		LLVMTypeRef concrete_type=llvmBuildType(ctx,getChild(n,1));

		LLVMRefineType(t->llvm_type, concrete_type);
		t->llvm_type=concrete_type;

	}

}

void llvmDeclareVariable(Context* ctx, Node* n)
{

	if(n->type == VARIABLE_DECLARATION)
	{

		Variable v={
			.llvm_value = LLVMAddGlobal(ctx->module, llvmBuildType(ctx,getChild(n,1)), getChild(n,0)->value),
			.declaration = n,
		};

		addVariable(ctx,v);

	}
	else if(n->type == FUNCTION_DECLARATION)
	{
		Variable v={
			.llvm_value = LLVMAddFunction(ctx->module, getChild(n,0)->value, llvmBuildType(ctx,getChild(n,1))),
			.declaration = n,
		};

		addVariable(ctx,v);
	}

}

void llvmDefineFunction(Context* ctx, Node* n)
{

	assert(n->type==FUNCTION_DECLARATION);
	Variable function=*getVariable(ctx, getChild(n,0)->value);

	LLVMBasicBlockRef bblock=LLVMAppendBasicBlock(function.llvm_value, "entry");
	ctx->builder=LLVMCreateBuilder();
	LLVMPositionBuilderAtEnd(ctx->builder, bblock);


	Node* type=getChild(n,1);

	for(int i=0;i<getChildrenCount(type)-1;i++)
	{

		Node* arg=getChild(type,i);

		Variable v={
			.declaration=arg,
			.llvm_value=LLVMBuildAlloca(ctx->builder, llvmBuildType(ctx,getChild(arg,1)), getChild(arg,0)->value),
		};

		LLVMBuildStore(ctx->builder,LLVMGetParam(function.llvm_value,i),v.llvm_value);

		addVariable(ctx,v);	

	}

	llvmBuildStatement(ctx,getChild(n,2));

	if(ctx->builder!=NULL)
	{
		LLVMBuildUnreachable(ctx->builder);
		LLVMDisposeBuilder(ctx->builder);
		ctx->builder=NULL;
//		panicNode(f,"no return statement");
//		LLVMBuildRetVoid(ctx->builder);

	}


}


void llvmDefineVariable(Context* ctx, Node* n)
{

	if(n->type == VARIABLE_DECLARATION && getChildrenCount(n)==3)
	{

		Variable* v=getVariable(ctx, getChild(n,0)->value);

		LLVMSetInitializer(v->llvm_value,llvmBuildExpresion(ctx,getChild(n,2)));

	}
	else if(n->type == FUNCTION_DECLARATION && getChildrenCount(n)==3)
	{
		llvmDefineFunction(ctx,n);
	}

}

void compileModule(Module *m)
{

	Context ctx={0};

	ctx.module=LLVMModuleCreateWithName("");

	LLVMAddTypeName(ctx.module,"Bool",LLVMInt1Type());
	LLVMAddTypeName(ctx.module,"Int",LLVMInt32Type());
	LLVMAddTypeName(ctx.module,"Void",LLVMVoidType());

	for(Node* n=m->child;n!=NULL;n=n->next)
	{
		llvmDeclareType(&ctx,n);
	}

	for(Node* n=m->child;n!=NULL;n=n->next)
	{
		llvmDefineType(&ctx,n);
	}

	for(Node* n=m->child;n!=NULL;n=n->next)
	{
		llvmDeclareVariable(&ctx,n);
	}

	for(Node* n=m->child;n!=NULL;n=n->next)
	{
		llvmDefineVariable(&ctx,n);
	}


//	LLVMBuildCall(ctx->builder, LLVMAddFunction()  ,NULL,0,"");

	for(Node* n=m->child;n!=NULL;n=n->next)
	{
		//if(n->type==FUNCTION_DECLARATION)
			//llvmBuildFunction(&ctx,n);
	}

	LLVMDumpModule(ctx.module);
	LLVMVerifyModule(ctx.module,LLVMAbortProcessAction,NULL);


	LLVMPassManagerRef manager=LLVMCreatePassManager();
	LLVMAddPromoteMemoryToRegisterPass(manager);
	LLVMAddSCCPPass(manager);
	LLVMAddJumpThreadingPass(manager);
	LLVMAddSimplifyLibCallsPass(manager);
	LLVMAddInstructionCombiningPass(manager);

	LLVMRunPassManager(manager,ctx.module);
	LLVMDisposePassManager(manager);

	LLVMDumpModule(ctx.module);

}


