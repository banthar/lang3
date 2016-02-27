
#include <stdlib.h>
#include <string.h>

#include "llvm-c/Core.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/BitWriter.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Transforms/Scalar.h"

#include "llvm_utils.h"
#include "llvmgen.h"
#include "error.h"
#include "operators.h"

typedef struct
{
	LLVMOpcode integer;
	LLVMOpcode unsigned_integer;
	LLVMOpcode real;	
}OperatorOpcodes;

static const OperatorOpcodes operatorOpcodes[]={
	[OPERATOR_MUL]={ LLVMMul,  LLVMMul,  LLVMFMul},
	{ LLVMFDiv, LLVMFDiv, LLVMFDiv},
	{ LLVMSDiv, LLVMUDiv, 0},
	{ LLVMSRem, LLVMURem, LLVMFRem},
	{ LLVMAdd,  LLVMAdd,  LLVMFAdd},
	{ LLVMSub,  LLVMSub,  LLVMFSub},

	{ LLVMIntSLT, LLVMIntULT, LLVMRealOLT},
	{ LLVMIntSGT, LLVMIntUGT, LLVMRealOGT},
	{ LLVMIntSLE, LLVMIntULE, LLVMRealOLE},
	{ LLVMIntSGE, LLVMIntUGE, LLVMRealOGE},
	{ LLVMIntEQ, LLVMIntEQ, LLVMRealOEQ},
	{ LLVMIntNE, LLVMIntNE, LLVMRealONE},
};

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

	Context* parent;

	int variables;
	Variable* variable;

	int types;
	Type* type;

	LLVMModuleRef module;
	LLVMValueRef function;

	LLVMBuilderRef builder;
	bool terminated;
	LLVMBasicBlockRef breakBlock;
	LLVMBasicBlockRef continueBlock;
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
LLVMValueRef llvmBuildExpresion(Context* ctx, Node* n, LLVMTypeRef expectedType);
LLVMValueRef llvmBuildLExpresion(Context* ctx, Node* n, LLVMTypeRef expectedType);

static Context pushContext(Context *ctx)
{

	Context new_context={
		.parent=ctx,
		.module=ctx->module,
		.function=ctx->function,
		.builder=ctx->builder,
		.terminated=false,
		.continueBlock=ctx->continueBlock,
		.breakBlock=ctx->breakBlock,
	};

	return new_context;

}

static void destroyContext(Context *ctx)
{
	if(ctx->variable!=NULL)
		free(ctx->variable);

	if(ctx->type!=NULL)
		free(ctx->type);
}

static Context* popContext(Context *ctx)
{

	destroyContext(ctx);

	ctx->parent->terminated=ctx->terminated;

	*ctx=(Context){0};

	return ctx->parent;
}

static void addVariable(Context *ctx, Variable variable)
{

    assert((ctx->variable==NULL) == (ctx->variables==0))

    if(getVariable(ctx,getChild(variable.declaration,0)->value)!=NULL) {
        panicNode(variable.declaration,"duplicated name");
	}

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

    assert(name != NULL);

	if(ctx==NULL)
		return NULL;

	for(int i=0;i<ctx->variables;i++)
	{

		Node* declaration=ctx->variable[i].declaration;

		if(strcmp(name,getChild(declaration,0)->value)==0)
			return &ctx->variable[i];
	}

	return NULL;
}

static Variable* resolveVariable(const Context* ctx, const char* name)
{
	while(ctx!=NULL) {
		Variable* v=getVariable(ctx,name);
		if(v!=NULL){
			return v;
		} else {
			ctx=ctx->parent;
		}
    }
	return NULL;
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

int llvmGetFieldId(Context* ctx, Node* n, LLVMTypeRef llvmType)
{
	Type* type=getTypeById(ctx,llvmType);
	assert(type!=NULL);
	
	Node* structNode=getChild(type->declaration,1);
	
	assert(structNode->type==STRUCT_TYPE);
	
	for(int i=0;i<getChildrenCount(structNode);i++)
	{
		const char* name=getChild(getChild(structNode,i),0)->value;
		
		if(strcmp(name,n->value)==0)
			return i;
	}

	panicNode(n,"no field '%s' in type '%s'",n->value,getChild(type->declaration,0)->value);

}

/* Expression */

LLVMValueRef llvmBuildIncrement(Context* ctx, LLVMValueRef value)
{
	LLVMTypeRef type=LLVMTypeOf(value);
	LLVMValueRef one=LLVMConstInt(type,1,false);
	return LLVMBuildAdd(ctx->builder,value,one,"");
}

LLVMValueRef llvmBuildDecrement(Context* ctx, LLVMValueRef value)
{
	LLVMTypeRef type=LLVMTypeOf(value);
	LLVMValueRef one=LLVMConstInt(type,1,false);
	return LLVMBuildSub(ctx->builder,value,one,"");
}

LLVMValueRef llvmBuildOperation(Context* ctx, Node* n)
{

	LLVMValueRef arg(int index)
	{
		assert(getChildrenCount(n)>index);

		return llvmBuildExpresion(ctx,getChild(n,index),NULL);

	}

	LLVMValueRef larg(int index)
	{
		assert(getChildrenCount(n)>index);

		return llvmBuildLExpresion(ctx,getChild(n,index),NULL);

	}

	switch(operators[n->operator].category)
	{
		case BINARY_ARITHMETIC:
			return LLVMBuildBinOp(ctx->builder, operatorOpcodes[n->operator].integer,arg(0),arg(1),cstrString(&n->source));
		case COMPARISION:
			return LLVMBuildICmp(ctx->builder, operatorOpcodes[n->operator].integer, arg(0),arg(1),cstrString(&n->source));
		default:
			break;
	}

	switch(n->operator)
	{
		case OPERATOR_PREFIX_PLUS:
			return arg(0);
		case OPERATOR_PREFIX_MINUS:
			return LLVMBuildNeg(ctx->builder,arg(0),cstrString(&n->source));
		case OPERATOR_PREFIX_INCREMENT:
		case OPERATOR_PREFIX_DECREMENT:
		case OPERATOR_POSTFIX_INCREMENT:
		case OPERATOR_POSTFIX_DECREMENT:
			{
				LLVMValueRef lvalue=larg(0);
				LLVMValueRef value=LLVMBuildLoad(ctx->builder,lvalue,"");
				LLVMValueRef new_value;
				
				if(n->operator==OPERATOR_PREFIX_INCREMENT||n->operator==OPERATOR_POSTFIX_INCREMENT)
					new_value=llvmBuildIncrement(ctx,value);
				else
					new_value=llvmBuildDecrement(ctx,value);
				
				LLVMBuildStore(ctx->builder,new_value,lvalue);
				
				if(n->operator==OPERATOR_PREFIX_INCREMENT || n->operator==OPERATOR_PREFIX_DECREMENT)
					return new_value;
				else
					return value;
					
			}
		case OPERATOR_CALL:
			{

				int num_args=getChildrenCount(n)-1;
				LLVMValueRef args[num_args];

				for(int i=0;i<num_args;i++)
				{
					args[i]=arg(i+1);
				}

				return LLVMBuildCall(ctx->builder, larg(0), args,num_args,"");

			}
		case OPERATOR_ASSIGMENT:
			return LLVMBuildStore(ctx->builder,arg(1),larg(0));
		case OPERATOR_ADDRESS:
			return larg(0);
		case OPERATOR_DEREF:
			return LLVMBuildLoad(ctx->builder,arg(0),"");
		case OPERATOR_INDEX:
			{
				
				LLVMValueRef ptr=LLVMBuildGEP(ctx->builder, arg(0),(LLVMValueRef[]){arg(1)}, 1,"");
				LLVMTypeRef type=LLVMTypeOf(ptr);
				
				switch(LLVMGetTypeKind(type))
				{
					case LLVMPointerTypeKind:
						return LLVMBuildLoad(ctx->builder,ptr,"");
					default:
						panicNode(n,"");
				}
				
				assertNode(n,LLVMGetTypeKind(type)==LLVMPointerTypeKind,"is not indexable");
				
				return LLVMBuildLoad(ctx->builder,ptr,"");
			}
		case OPERATOR_ELEMENT:
			{
				LLVMValueRef left=arg(0);

				LLVMTypeRef left_type=LLVMTypeOf(left);

				assert(LLVMGetTypeKind(left_type)==LLVMStructTypeKind);

				int field_id=llvmGetFieldId(ctx, getChild(n,1),left_type);

				return LLVMBuildExtractValue(ctx->builder, left,field_id, "");
                                     
				return LLVMBuildStructGEP(ctx->builder, left,field_id,"");
			}			
		default:
			break;
	}

	panicNode(n,"operator '%s' not implemented",operators[n->operator].name);

}


static LLVMValueRef pointerToConstString(LLVMBuilderRef builder, const char* value) {
		LLVMValueRef string=LLVMConstString(value, strlen(value),false);			
		LLVMValueRef ptr=LLVMBuildAlloca(builder,LLVMTypeOf(string),"");
		LLVMBuildStore(builder,string,ptr);
		return LLVMBuildGEP(builder, ptr,(LLVMValueRef[]){LLVMConstNull(LLVMInt32Type()),LLVMConstNull(LLVMInt32Type())}, 2,"");
}

LLVMValueRef llvmBuildStructConstant(Context* ctx, Node* n, LLVMTypeRef expectedType) {

    if(expectedType==NULL){
        panicNode(n,"Unable to initialize unknown struct");
    }

    if(LLVMGetTypeKind(expectedType) != LLVMStructTypeKind){
        panicNode(n,"Unable to initialize non-struct type");
    }


    int vals=getChildrenCount(n);
	LLVMValueRef val[vals];

	for(int i=0;i<vals;i++){
		val[i]=llvmBuildExpresion(ctx,getChild(n,i),NULL);
	}

    return LLVMConstNamedStruct(expectedType, val, vals);

}

LLVMValueRef llvmBuildExpresion(Context* ctx, Node* n, LLVMTypeRef expectedType) {

	switch(n->type)
	{
		case NUMERIC_CONSTANT:
			return LLVMConstIntOfString(LLVMInt32Type(), n->value, 10);
		case CHAR_CONSTANT:
			if(strlen(n->value)!=1)
				panicNode(n,"invalid char constant");
			return LLVMConstInt(LLVMInt8Type(), n->value[0],false);
		case STRING_CONSTANT:
		{
			return pointerToConstString(ctx->builder, n->value);
		}
		case STRUCT_CONSTANT:
		{
			return llvmBuildStructConstant(ctx,n,expectedType);
		}
		case NULL_CONSTANT:
			return LLVMConstNull(LLVMPointerType(LLVMVoidType(),0));
		case TRUE_CONSTANT:
			return LLVMConstInt(LLVMInt1Type(),1,false);
		case FALSE_CONSTANT:
			return LLVMConstInt(LLVMInt1Type(),0,false);
		case IDENTIFIER:
			return LLVMBuildLoad(ctx->builder,llvmBuildLExpresion(ctx,n,NULL),n->value);
		case OPERATION:
			return llvmBuildOperation(ctx,n);
		default:
			panicNode(n,"not an expresion");
	}

}

/* L-Expression */

LLVMValueRef llvmBuildLOperation(Context* ctx, Node* n) {

	LLVMValueRef arg(int index)
	{
		assert(getChildrenCount(n)>index);

		return llvmBuildExpresion(ctx,getChild(n,index),NULL);

	}

	LLVMValueRef larg(int index)
	{
		assert(getChildrenCount(n)>index);

		return llvmBuildLExpresion(ctx,getChild(n,index),NULL);

	}

	switch(n->operator)
	{
		case OPERATOR_ELEMENT:
			{
				LLVMValueRef left=larg(0);

				LLVMTypeRef left_type=LLVMTypeOf(left);

				assert(LLVMGetTypeKind(left_type)==LLVMPointerTypeKind);

				LLVMTypeRef struct_type=LLVMGetElementType(left_type);

				assert(LLVMGetTypeKind(struct_type)==LLVMStructTypeKind);

				int field_id=llvmGetFieldId(ctx, getChild(n,1),struct_type);

				//TODO
				//printf("type: %p",t);

				return LLVMBuildStructGEP(ctx->builder, left,field_id,"");
			}
		case OPERATOR_INDEX:
			{
				
				LLVMValueRef array=arg(0);
				LLVMTypeRef type=LLVMTypeOf(array);
				
				switch(LLVMGetTypeKind(type))
				{
					case LLVMPointerTypeKind:
						return LLVMBuildGEP(ctx->builder, array,(LLVMValueRef[]){arg(1)}, 1,"");
					default:
						panicNode(n,"invalid use of []");
				}
				
			}
		case OPERATOR_DEREF:
			{
				return arg(0);
			}
		default:
			break;
	}
	panicNode(n,"is not a l-value");

}

LLVMValueRef llvmBuildLExpresion(Context* ctx, Node* n, LLVMTypeRef expectedType)
{
	switch(n->type)
	{
		case IDENTIFIER:
		{

			if(ctx->builder==NULL)
				panicNode(n,"expresion has to be constant");

			Variable* v=resolveVariable(ctx, n->value);

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

/* Statement */

void llvmBuildStatement(Context* ctx, Node* n)
{

	if(ctx->terminated)
		panicNode(n,"unreachable code");

	switch(n->type)
	{

		case OPERATION:
			llvmBuildExpresion(ctx,n,NULL);
			break;
		case VARIABLE_DECLARATION:
			{

				Variable v;

				v.declaration=n;
                LLVMTypeRef type = llvmBuildType(ctx,getChild(n,1));
				v.llvm_value=LLVMBuildAlloca(ctx->builder, type, getChild(n,0)->value );

				if(getChildrenCount(n)>2)
					LLVMBuildStore(ctx->builder,llvmBuildExpresion(ctx,getChild(n,2),type),v.llvm_value);

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
					LLVMBuildRet(ctx->builder,llvmBuildExpresion(ctx,getChild(n,0),NULL));

				ctx->terminated=true;
			}
			break;
		case ASSERT_STATEMENT:
			{

				Node* value = getChild(n,0);

				LLVMValueRef condition=llvmBuildExpresion(ctx,value,NULL);

				LLVMBasicBlockRef failed=LLVMAppendBasicBlock(ctx->function,"assertion_failed");
				LLVMBasicBlockRef passed=LLVMAppendBasicBlock(ctx->function,"assertion_passed");

				LLVMBuildCondBr(ctx->builder, condition,passed,failed);

				LLVMPositionBuilderAtEnd(ctx->builder, failed);
				
				const char* filename = value->source.stream->filename;
				const char* fmt = "assertion failed: %s";
				const char* msg = cstrString(&value->source);

				LLVMValueRef abortFunction = LLVMGetNamedFunction(ctx->module,"panic");
				LLVMBuildCall(ctx->builder, abortFunction , (LLVMValueRef[]){
					pointerToConstString(ctx->builder, filename),
					LLVMConstInt(LLVMInt32Type(), 0, false),
					pointerToConstString(ctx->builder, fmt),
					pointerToConstString(ctx->builder, msg),
				}, 4, "");
				LLVMBuildUnreachable(ctx->builder);

				LLVMPositionBuilderAtEnd(ctx->builder, passed);

			}
			break;
		case WHILE_STATEMENT:
			{
				
				Context loop_ctx=pushContext(ctx);
				
				loop_ctx.continueBlock=LLVMAppendBasicBlock(loop_ctx.function,"while_begin");
				LLVMBasicBlockRef bodyBlock=LLVMAppendBasicBlock(loop_ctx.function,"while_body");
				loop_ctx.breakBlock=LLVMAppendBasicBlock(loop_ctx.function,"while_end");
				
				LLVMBuildBr(loop_ctx.builder, loop_ctx.continueBlock);
				
				LLVMPositionBuilderAtEnd(loop_ctx.builder, loop_ctx.continueBlock);
				LLVMValueRef condition=llvmBuildExpresion(&loop_ctx,getChild(n,0),NULL);
				LLVMBuildCondBr(loop_ctx.builder, condition,bodyBlock,loop_ctx.breakBlock);

				LLVMPositionBuilderAtEnd(loop_ctx.builder, bodyBlock);
				llvmBuildStatement(&loop_ctx,getChild(n,1));
				if(!loop_ctx.terminated)
					LLVMBuildBr(loop_ctx.builder, loop_ctx.continueBlock);
				
				LLVMPositionBuilderAtEnd(loop_ctx.builder, loop_ctx.breakBlock);
				
				popContext(&loop_ctx);
				ctx->terminated=false;
				
			}
			break;
		case FOR_STATEMENT:
			{
				Context loop_ctx=pushContext(ctx);
				
				llvmBuildStatement(&loop_ctx,getChild(n,0));
				
				LLVMBasicBlockRef beginBlock=LLVMAppendBasicBlock(loop_ctx.function,"for_begin");
				LLVMBasicBlockRef bodyBlock=LLVMAppendBasicBlock(loop_ctx.function,"for_body");
				loop_ctx.continueBlock=LLVMAppendBasicBlock(loop_ctx.function,"for_end");
				loop_ctx.breakBlock=LLVMAppendBasicBlock(loop_ctx.function,"for_break");
				
				LLVMBuildBr(loop_ctx.builder, beginBlock);
				
				LLVMPositionBuilderAtEnd(loop_ctx.builder, beginBlock);
				LLVMValueRef condition=llvmBuildExpresion(&loop_ctx,getChild(n,1),NULL);
				LLVMBuildCondBr(loop_ctx.builder, condition,bodyBlock,loop_ctx.breakBlock);

				LLVMPositionBuilderAtEnd(loop_ctx.builder, bodyBlock);
				llvmBuildStatement(&loop_ctx,getChild(n,3));
				if(!loop_ctx.terminated)
					LLVMBuildBr(loop_ctx.builder, loop_ctx.continueBlock);

				LLVMPositionBuilderAtEnd(loop_ctx.builder, loop_ctx.continueBlock);
				llvmBuildStatement(&loop_ctx,getChild(n,2));
				if(!loop_ctx.terminated)
					LLVMBuildBr(loop_ctx.builder, beginBlock);
				
				LLVMPositionBuilderAtEnd(loop_ctx.builder, loop_ctx.breakBlock);
				
				popContext(&loop_ctx);
				ctx->terminated=false;
			}
			break;
		case BREAK_STATEMENT:
			{
				if(ctx->breakBlock==NULL)
					panicNode(n,"break without a loop");
				LLVMBuildBr(ctx->builder, ctx->breakBlock);
				ctx->terminated=true;
			}
			break;
		case CONTINUE_STATEMENT:
			{
				if(ctx->continueBlock==NULL)
					panicNode(n,"continue without a loop");
				LLVMBuildBr(ctx->builder, ctx->continueBlock);
				ctx->terminated=true;
			}
			break;
		case IF_STATEMENT:
			{
				LLVMValueRef condition=llvmBuildExpresion(ctx,getChild(n,0),NULL);

				LLVMBasicBlockRef thenBlock=LLVMAppendBasicBlock(ctx->function,"then");
				LLVMBasicBlockRef elseBlock=LLVMAppendBasicBlock(ctx->function,"else");
				LLVMBasicBlockRef exitBlock=LLVMAppendBasicBlock(ctx->function,"fi");

				LLVMBuildCondBr(ctx->builder, condition,thenBlock,elseBlock);				

				LLVMPositionBuilderAtEnd(ctx->builder, thenBlock);
				ctx->terminated=false;
				llvmBuildStatement(ctx,getChild(n,1));

				if(!ctx->terminated)
					LLVMBuildBr(ctx->builder, exitBlock);

				LLVMPositionBuilderAtEnd(ctx->builder, elseBlock);
				ctx->terminated=false;
				if(getChildrenCount(n)>=3)
				{
					llvmBuildStatement(ctx,getChild(n,2));
				}
				if(!ctx->terminated)
					LLVMBuildBr(ctx->builder, exitBlock);

				LLVMPositionBuilderAtEnd(ctx->builder, exitBlock);
				ctx->terminated=false;

			}
			break;
		default:
			{
				panicString(&n->source, "not a statement");
			}
	}
}

/* Type */

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
		case ARRAY_TYPE:
			{
				if(getChildrenCount(n)!=2)
					panicNode(n,"Array type needs two arguments");
					
				LLVMValueRef elem_count=llvmBuildExpresion(ctx,getChild(n,0),NULL);
				
				if(!LLVMIsConstant(elem_count))
					panicNode(n,"Array size is not constant");
				
				printf(">>> %i\n",LLVMConstIntValue(elem_count));

				return LLVMArrayType(llvmBuildType(ctx,getChild(n,1)),LLVMConstIntValue(elem_count));
			}
			break;
			
		case IDENTIFIER:
			{
				
				if(strcmp(n->value,"Void")==0) {
					return LLVMVoidType();
				} else if (strcmp(n->value,"Bool")==0) {
					return LLVMInt1Type();
				} else if (strcmp(n->value,"Byte")==0) {
					return LLVMInt8Type();
				} else if (strcmp(n->value,"Int")==0) {
					return LLVMInt32Type();
				}

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
				
				bool varArg=false;
				int nArgs;
				
				nArgs=getChildrenCount(n)-1;
				
				if(nArgs-1>=0 && getChild(n,nArgs-1)->type==ELLIPSIS)
				{
					nArgs-=1;
					varArg=true;
				}
				
				LLVMTypeRef llvm_args[nArgs];

				for(int i=0;i<nArgs;i++)
				{
					llvm_args[i]=llvmBuildType(ctx,getChild(getChild(n,i),1));
				}

				LLVMTypeRef ret=llvmBuildType(ctx,getChild(n,getChildrenCount(n)-1));

				return LLVMFunctionType(ret, llvm_args,nArgs,varArg);
			}
			break;
		default:
			panicString(&n->source,"not a type");
	}

	assert(false);

}

void llvmDeclareStruct(Context* ctx, Node* n)
{

	if(n->type == STRUCT_DECLARATION)
	{
		Type t={
			.declaration=n,
			.llvm_type=LLVMStructCreateNamed(LLVMGetModuleContext(ctx->module), getChild(n,0)->value),
		};

		addType(ctx,t);
	}

}

void llvmDefineStruct(Context* ctx, Node* n)
{
	if(n->type == STRUCT_DECLARATION)
	{
		Type* t=getType(ctx,getChild(n,0)->value);

		assert(t!=NULL);

		LLVMTypeRef struct_type=llvmBuildType(ctx,getChild(n,1));

		unsigned size = LLVMCountStructElementTypes(struct_type);
		LLVMTypeRef elements[size];
		LLVMGetStructElementTypes(struct_type, elements);

		LLVMStructSetBody(t->llvm_type, elements, size, false);

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

    Context functionCtx = pushContext(ctx);
    ctx = &functionCtx;

	ctx->function=function.llvm_value;

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

	ctx->terminated=false;

	llvmBuildStatement(ctx,getChild(n,2));

	if(!ctx->terminated)
	{
		
		if(LLVMGetReturnType(LLVMGetReturnType(LLVMTypeOf(ctx->function)))==LLVMVoidType())
		{
			LLVMBuildRetVoid(ctx->builder);
		}
		else
		{
//			panicNode(n,"no return in function returning non-void");
			LLVMBuildUnreachable(ctx->builder);
		}
		ctx->terminated=true;
	}

    ctx = popContext(ctx);

}

void llvmDefineVariable(Context* ctx, Node* n)
{

	if(n->type == VARIABLE_DECLARATION && getChildrenCount(n)==3)
	{

		Variable* v=getVariable(ctx, getChild(n,0)->value);

		LLVMSetInitializer(v->llvm_value,llvmBuildExpresion(ctx,getChild(n,2),NULL));

	}
	else if(n->type == FUNCTION_DECLARATION && getChildrenCount(n)==3)
	{
		llvmDefineFunction(ctx,n);
	}

}

void addOptimizations(LLVMPassManagerRef pm)
{
	LLVMAddDemoteMemoryToRegisterPass(pm);
	LLVMAddAggressiveDCEPass(pm);
	LLVMAddCFGSimplificationPass(pm);
	LLVMAddDeadStoreEliminationPass(pm);
	LLVMAddGVNPass(pm);
	LLVMAddIndVarSimplifyPass(pm);
	LLVMAddInstructionCombiningPass(pm);
	LLVMAddJumpThreadingPass(pm);
	LLVMAddLICMPass(pm);
	LLVMAddLoopDeletionPass(pm);
	LLVMAddLoopRotatePass(pm);
	LLVMAddLoopUnrollPass(pm);
	LLVMAddLoopUnswitchPass(pm);
	LLVMAddMemCpyOptPass(pm);
	LLVMAddPromoteMemoryToRegisterPass(pm);
	LLVMAddReassociatePass(pm);
	LLVMAddSCCPPass(pm);
	LLVMAddScalarReplAggregatesPass(pm);
	LLVMAddSimplifyLibCallsPass(pm);
	LLVMAddTailCallEliminationPass(pm);
	LLVMAddConstantPropagationPass(pm);
	LLVMAddVerifierPass(pm);
}

LLVMModuleRef compileModule(Node *m)
{

	Context ctx={0};

	ctx.module=LLVMModuleCreateWithName("");

	LLVMAddFunction(ctx.module,"panic",LLVMFunctionType(LLVMVoidType(), (LLVMTypeRef[]){
		LLVMPointerType(LLVMInt8Type(),0),
		LLVMInt32Type(),
		LLVMPointerType(LLVMInt8Type(),0),
	},3,true));


	for(Node* n=m->child;n!=NULL;n=n->next)	{
		llvmDeclareStruct(&ctx,n);
	}

	for(Node* n=m->child;n!=NULL;n=n->next) {
		llvmDefineStruct(&ctx,n);
	}

	for(Node* n=m->child;n!=NULL;n=n->next)	{
		llvmDeclareVariable(&ctx,n);
	}

	for(Node* n=m->child;n!=NULL;n=n->next)	{
		llvmDefineVariable(&ctx,n);
	}


//	LLVMBuildCall(ctx->builder, LLVMAddFunction()  ,NULL,0,"");

	for(Node* n=m->child;n!=NULL;n=n->next)
	{
		//if(n->type==FUNCTION_DECLARATION)
			//llvmBuildFunction(&ctx,n);
	}

	//LLVMDumpModule(ctx.module);
	LLVMVerifyModule(ctx.module,LLVMAbortProcessAction,NULL);

	LLVMPassManagerRef manager=LLVMCreatePassManager();
	LLVMAddPromoteMemoryToRegisterPass(manager);
	//LLVMAddSCCPPass(manager);
	//LLVMAddJumpThreadingPass(manager);
	//LLVMAddSimplifyLibCallsPass(manager);
	//LLVMAddInstructionCombiningPass(manager);

	addOptimizations(manager);
	addOptimizations(manager);
	addOptimizations(manager);

	LLVMRunPassManager(manager,ctx.module);
	LLVMDisposePassManager(manager);

	//LLVMDumpModule(ctx.module);

	destroyContext(&ctx);
	
	return ctx.module;

}

void writeString(const char* s)
{
	printf("%s\n",s);
}

void writeInt(int i)
{
	printf("%d\n",i);
}

int readInt()
{
	int i;
	scanf("%d",&i);
	return i;
}

static void defineFunction(LLVMExecutionEngineRef engine, LLVMModuleRef module, const char* name, void* implementation) {
	void* function = LLVMGetNamedFunction(module, name);
	if(function != NULL) {
		LLVMAddGlobalMapping(engine, function, implementation);
	}
}

int runModule(LLVMModuleRef llvmModule, int argc, const char*argv[])
{
	
	LLVMLinkInMCJIT();
	LLVMInitializeNativeTarget();
	LLVMInitializeNativeAsmPrinter();

	char *err;
	LLVMExecutionEngineRef engine;
	bool ret=LLVMCreateMCJITCompilerForModule(&engine, llvmModule, NULL, 0, &err);
	
	if(ret)
	{
		panic(err);
		return -1;
	}

	defineFunction(engine, llvmModule, "writeInt", &writeInt);
	defineFunction(engine, llvmModule, "readInt", &readInt);
	defineFunction(engine, llvmModule, "atoi", &atoi);
	defineFunction(engine, llvmModule, "printf", &printf);
	defineFunction(engine, llvmModule, "panic", &panic);

	int returnValue=LLVMRunFunctionAsMain(engine, LLVMGetNamedFunction(llvmModule,"main"),argc,argv,(const char*[]){NULL});
					  
	LLVMDisposeExecutionEngine(engine);
	
	
	return returnValue;
}

