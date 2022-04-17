/*
 * Calculator.c
 *
 * Author: bsoelch
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "BigInt.h"

static const int STACK_INIT_CAPACITY = 8;
static const int READ_LINE_INCREMENT = 50;


typedef struct{
	size_t length;
	char* chars;
}String;

typedef struct{
	String* strings;
	size_t capacity;
	size_t length;
}StringArray;


//space is weakest operator
#define FLAG_RTL 0x10
#define LEVEL_BIT_OFFSET 4
#define OPTYPE_AND 0x20
#define OPTYPE_OR 0x21
#define OPTYPE_XOR 0x22
#define OPTYPE_ADD 0x40
#define OPTYPE_SUBT 0x41
#define OPTYPE_MULT 0x60
#define OPTYPE_DIV 0x61
#define OPTYPE_MOD 0x62
#define OPTYPE_POW 0x70
#define OPTYPE_SGN 0x80
#define OPTYPE_PLUS 0x81
#define OPTYPE_NEG 0x82
#define OPTYPE_NOT 0x83

#define DATA_NULL 0
#define DATA_NONE 1
#define DATA_BIGINT 2
#define DATA_BRACKET 3
#define DATA_NODE 4
#define DATA_STRING 5

typedef struct{
	uint32_t opType;
	uint8_t leftType;
	uint8_t rightType;
	void* left;
	void* right;
}TreeNode;

void freeStringArray(StringArray *target) {
	//cleanup already allocated data
	if(target&&target->strings){
		while(target->length >0) {
			free(target->strings[--target->length].chars);
		}
		free(target->strings);
	}
	free(target);
}

static StringArray* addNonemptyString(StringArray* target,char* chars,size_t off,size_t len){
	if(len>0){
		target->strings[target->length].length=len;
		target->strings[target->length].chars=malloc((len+1)*sizeof(char));
		if(target->strings[target->length].chars!=NULL){
			memcpy(target->strings[target->length].chars,chars+off,len);
			target->strings[target->length].chars[len]='\0';
		}else{//cleanup already allocated data
			freeStringArray(target);
			return NULL;
		}
		target->length++;
		if(target->length>=target->capacity){
			String* tmp=realloc(target->strings,2*target->capacity*sizeof(String));
			if(tmp){
				memset(tmp+target->capacity,0,target->capacity*sizeof(String));
				target->strings=tmp;
				target->capacity*=2;
			}else{//cleanup already allocated data
				freeStringArray(target);
				return NULL;
			}
		}
	}
	return target;
}

StringArray* parseString(char* s,size_t size){
	size_t i0=0;
	StringArray* ret=malloc(sizeof(StringArray));
	if(ret){
		ret->capacity=STACK_INIT_CAPACITY;
		ret->strings=calloc(ret->capacity,sizeof(String));
		ret->length=0;
		if(ret->strings){
			for(size_t i=0;i<size;i++){
				if(isspace(s[i])){
					if(i>i0){
						ret=addNonemptyString(ret,s,i0,i-i0);
						if(ret){
							i0=i+1;
						}else{
							return NULL;
						}
					}else{
						i0++;
					}
				}else{
					switch(s[i]){
						case '(':
						case ')':
						case '?'://sgn
						case '#'://xor
						case '~':
						case '+':
						case '-':
						case '*':
						case '/':
						case '%':
						case '&':
						case '|':
						case '^':
							if(i>i0){
								ret=addNonemptyString(ret,s,i0,i-i0);
								if(ret){
									i0=i+1;
								}else{
									return NULL;
								}
							}else{
								i0++;
							}
							ret=addNonemptyString(ret,&s[i],0,1);
							if(!ret){
								return NULL;
							}
							break;
						//TODO additional operators
						// < << <=
						// > >> >=
						// ==
						// !=
					}
				}
			}
			if(size>i0){
				ret=addNonemptyString(ret,s,i0,size-i0-1);
				if(!ret){
					return NULL;
				}
			}
		}else{
			free(ret);
		}
	}
	return ret;
}

StringArray* getLine(){
	size_t size = READ_LINE_INCREMENT, off = 0, len;
	char* s=malloc(size*sizeof(char));
	char* t;
	for(;;){
		t=fgets(s+off,size,stdin);
		if(t==NULL){
			if(ferror(stdin)){
				free(s);
				return NULL;
			}else{
				break;
			}
		}
		len=strlen(t);
		if(t[len-1]=='\n'||t[len-1]=='\r'){
			t=realloc(s,off+len+1);
			if(t){
				s=t;
			}else{
				free(s);
				return NULL;
			}
			size=off+len;
			break;
		}
		off+=len;
		t=realloc(s,off+READ_LINE_INCREMENT);
		if(t){
			s=t;
		}else{
			free(s);
			return NULL;
		}
	}
	StringArray* ret= parseString(s,size);
	free(s);
	return ret;
}


static void freeTree(TreeNode* toFree){
	if(toFree){
		switch(toFree->leftType){
			case DATA_BIGINT:
				freeBigInt(toFree->left);
				break;
			case DATA_NODE:
				freeTree(toFree->left);
				break;
			case DATA_STRING:
				free(((String*)toFree->left)->chars);
				free((String*)toFree->left);
				break;
		}
		switch(toFree->rightType){
			case DATA_BIGINT:
				freeBigInt(toFree->right);
				break;
			case DATA_NODE:
				freeTree(toFree->right);
				break;
			case DATA_STRING:
				free(((String*)toFree->right)->chars);
				free((String*)toFree->right);
				break;
		}
		free(toFree);
	}
}
static void freeTreeStack(TreeNode** stack,size_t size){
	if(stack){
		while(size>0){
			freeTree(stack[--size]);
		}
		free(stack);
	}
}
static void printTree(TreeNode* toPrint){
	if(toPrint){
		printf("(");
		switch(toPrint->leftType){
			case DATA_NULL:
				printf("_");
			    break;
			case DATA_NONE:
				break;
			case DATA_BIGINT:
				printBigIntHex(toPrint->left,false);
				break;
			case DATA_BRACKET:
			case DATA_NODE:
				printTree(toPrint->left);
				break;
			case DATA_STRING:
				printf("\"%s\"",((String*)toPrint->left)->chars);
				break;
		}
		switch(toPrint->opType){
			case 0:
				putchar(' ');
				break;
			case OPTYPE_SGN:
				putchar('?');
				break;
			case OPTYPE_PLUS:
			case OPTYPE_ADD:
				putchar('+');
				break;
			case OPTYPE_NEG:
			case OPTYPE_SUBT:
				putchar('-');
				break;
			case OPTYPE_NOT:
				putchar('~');
				break;
			case OPTYPE_MULT:
				putchar('*');
				break;
			case OPTYPE_DIV:
				putchar('/');
				break;
			case OPTYPE_MOD:
				putchar('%');
				break;
			case OPTYPE_AND:
				putchar('&');
				break;
			case OPTYPE_OR:
				putchar('|');
				break;
			case OPTYPE_XOR:
				putchar('#');
				break;
			case OPTYPE_POW:
				putchar('^');
				break;
			default:
				printf("[%#x]",toPrint->opType);
		}

		switch(toPrint->rightType){
			case DATA_NULL:
				printf("_");
				break;
			case DATA_NONE:
				break;
			case DATA_BIGINT:
				printBigIntHex(toPrint->right,false);
				break;
			case DATA_BRACKET:
			case DATA_NODE:
				printTree(toPrint->right);
				break;
			case DATA_STRING:
				printf("\"%s\"",((String*)toPrint->right)->chars);
				break;
		}
		printf(")");
	}else{
		printf("NULL");
	}
}

static TreeNode* pushValue(TreeNode* root,void* value,uint8_t type){
	if(root){
		if(root->leftType==DATA_NULL){
			root->left=value;
			root->leftType=type;
		}else if(root->rightType==DATA_NULL){
			root->right=value;
			root->rightType=type;
		}else if(root->rightType==DATA_NODE){
			root->right=pushValue(root->right,value,type);
			root->rightType=DATA_NODE;
		}else{
			TreeNode* next=calloc(1,sizeof(TreeNode));
			if(next){
				next->left=root;
				next->leftType=DATA_NODE;
				next->right=value;
				next->rightType=type;
				return next;
			}else{
				freeTree(root);
				return NULL;
			}
		}
		return root;
	}else{
		root=calloc(1,sizeof(TreeNode));
		if(root){
			root->left=value;
			root->leftType=type;
		}
		return root;
	}
	return NULL;
}

static TreeNode* pushOperator(TreeNode* root,uint32_t opType){
	if(root){
		if(root->opType==0&&(root->rightType==DATA_NULL)){
			if(root->leftType==DATA_NULL){
				root->leftType=DATA_NONE;
			}
			root->opType=opType;
		}else{
			int32_t delta=(opType>>LEVEL_BIT_OFFSET)-(root->opType>>LEVEL_BIT_OFFSET);
			if((root->rightType!=DATA_NULL)&&((delta<0)||(delta==0&&((opType&FLAG_RTL)==0)))){
				if(root->leftType!=DATA_NULL){
					TreeNode* next=calloc(1,sizeof(TreeNode));
					if(next){
						next->opType=opType;
						next->left=root;
						next->leftType=DATA_NODE;
						return next;
					}else{
						freeTree(root);
						return NULL;
					}
				}else{
					freeTree(root);
					return NULL;
				}
			}else{
				if(root->rightType==DATA_NULL||root->rightType==DATA_NODE){
					root->right=pushOperator(root->right,opType);
					root->rightType=DATA_NODE;
				}else{
					TreeNode* next=calloc(1,sizeof(TreeNode));
					if(next){
						next->opType=opType;
						next->left=root->right;
						next->leftType=root->rightType;
						root->right=next;
						root->rightType=DATA_NODE;
						return root;
					}else{
						freeTree(root);
						return NULL;
					}
				}
			}
		}
		return root;
	}else{
		root=calloc(1,sizeof(TreeNode));
		if(root){
			root->leftType=DATA_NONE;
			root->opType=opType;
		}
		return root;
	}
	return NULL;
}

static TreeNode* parse(StringArray* strings){
	size_t stackSize = 0, stackCapacity = STACK_INIT_CAPACITY;
	TreeNode** stack=malloc(stackCapacity*sizeof(TreeNode*));
	if(strings&&stack){
		TreeNode* tree=NULL;
		String* tmp;
		bool isUnary=true;
		for(size_t i=0;i<strings->length;i++){
			tmp=(strings->strings)+i;
			if(tmp->length>0){
				switch(tmp->chars[0]){
					case '(':
						if(stackSize>=stackCapacity){
							TreeNode** tmpStack=realloc(stack,2*stackCapacity*sizeof(TreeNode*));
							if(tmpStack){
								stack=tmpStack;
								stackCapacity*=2;
							}else{
								freeStringArray(strings);
								freeTree(tree);
								freeTreeStack(stack,stackSize);
								return NULL;
							}
						}
						stack[stackSize++]=tree;
						tree=NULL;
						break;
					case ')':
						if(stackSize>0){
							TreeNode* tmpNode=stack[--stackSize];
							stack[stackSize]=NULL;//set to null to prevent deletion
							tree=pushValue(tmpNode,tree,DATA_BRACKET);
							free(tmp->chars);
							tmp->chars=NULL;
							break;
						}
						freeStringArray(strings);
						freeTree(tree);
						freeTreeStack(stack,stackSize);
						return NULL;
					case '?':
						if(isUnary){
							tree=pushOperator(tree,OPTYPE_SGN);
							if(tree){
								break;
							}
						}
						freeStringArray(strings);
						freeTree(tree);
						freeTreeStack(stack,stackSize);
						return NULL;
					case '~':
						if(isUnary){
							tree=pushOperator(tree,OPTYPE_NOT);
							if(tree){
								break;
							}
						}
						freeStringArray(strings);
						freeTree(tree);
						freeTreeStack(stack,stackSize);
						return NULL;
					case '+':
						tree=pushOperator(tree,isUnary?OPTYPE_PLUS:OPTYPE_ADD);
						if(tree){
							break;
						}else{
							freeStringArray(strings);
							freeTree(tree);
							freeTreeStack(stack,stackSize);
							return NULL;
						}
					case '-':
						tree=pushOperator(tree,isUnary?OPTYPE_NEG:OPTYPE_SUBT);
						if(tree){
							break;
						}else{
							freeStringArray(strings);
							freeTree(tree);
							freeTreeStack(stack,stackSize);
							return NULL;
						}
					case '*':
						if(!isUnary){
							tree=pushOperator(tree,OPTYPE_MULT);
							if(tree){
								break;
							}
						}
						freeStringArray(strings);
						freeTree(tree);
						return NULL;
					case '/':
						if(!isUnary){
							tree=pushOperator(tree,OPTYPE_DIV);
							if(tree){
								break;
							}
						}
						freeStringArray(strings);
						freeTree(tree);
						freeTreeStack(stack,stackSize);
						return NULL;
					case '%':
						if(!isUnary){
							tree=pushOperator(tree,OPTYPE_MOD);
							if(tree){
								break;
							}
						}
						freeStringArray(strings);
						freeTree(tree);
						freeTreeStack(stack,stackSize);
						return NULL;
					case '&':
						if(!isUnary){
							tree=pushOperator(tree,OPTYPE_AND);
							if(tree){
								break;
							}
						}
						freeStringArray(strings);
						freeTree(tree);
						freeTreeStack(stack,stackSize);
						return NULL;
					case '|':
						if(!isUnary){
							tree=pushOperator(tree,OPTYPE_OR);
							if(tree){
								break;
							}
						}
						freeStringArray(strings);
						freeTree(tree);
						freeTreeStack(stack,stackSize);
						return NULL;
					case '#':
						if(!isUnary){
							tree=pushOperator(tree,OPTYPE_XOR);
							if(tree){
								break;
							}
						}
						freeStringArray(strings);
						freeTree(tree);
						freeTreeStack(stack,stackSize);
						return NULL;
					case '^':
						if(!isUnary){
							tree=pushOperator(tree,OPTYPE_POW);
							if(tree){
								break;
							}
						}
						freeStringArray(strings);
						freeTree(tree);
						freeTreeStack(stack,stackSize);
						return NULL;
					//TODO other operators
					default:{
						if(!isUnary){//two consecutive numbers
							tree=pushOperator(tree,0);//separate by ' '
							if(!tree){
								freeStringArray(strings);
								freeTree(tree);
								freeTreeStack(stack,stackSize);
								return NULL;
							}
						}
						BigInt* asInt=createBigIntStr(tmp->chars,tmp->length,10);
						if(asInt){
							tree=pushValue(tree,asInt,DATA_BIGINT);
							isUnary=false;
							free(tmp->chars);
							tmp->chars=NULL;//for isUnary detection
							break;
						}else{
							String* copy=malloc(sizeof(String));
							if(copy){
								copy->length=tmp->length;
								copy->chars=tmp->chars;
								tmp->chars=NULL;
								tree=pushValue(tree,copy,DATA_STRING);
								isUnary=false;
								break;
							}
						}
						freeStringArray(strings);
						freeTree(tree);
						freeTreeStack(stack,stackSize);
						return NULL;
					}
				}
				if(tmp->chars){//tmp.chars is only NULL if the last argument was no operator
					isUnary=true;
				}
				free(tmp->chars);
				tmp->chars=NULL;
			}
		}
		freeStringArray(strings);
		if(stackSize>0){
			freeTree(tree);
			tree=NULL;
		}
		freeTreeStack(stack,stackSize);
		return tree;
	}else{
		freeStringArray(strings);
		freeTreeStack(stack,stackSize);
		return NULL;
	}
}

//evaluates tree (tree is consumed in the process)
static BigInt* evaluateTree(TreeNode* tree){
	if(tree){
		BigInt* leftValue=NULL;
		if((tree->leftType==DATA_NODE)||(tree->leftType==DATA_BRACKET)){
			leftValue=evaluateTree(tree->left);
			tree->left=NULL;//evaluate freed tree->left
			if(!leftValue){
				freeTree(tree);
				return NULL;
			}
		}else if(tree->leftType==DATA_BIGINT){
			leftValue=tree->left;
			tree->left=NULL;
		}else if((tree->leftType!=DATA_NONE)&&(tree->leftType!=DATA_NULL)){
			freeTree(tree);
			return NULL;
		}
		BigInt* rightValue=NULL;
		if((tree->rightType==DATA_NODE)||(tree->rightType==DATA_BRACKET)){
			rightValue=evaluateTree(tree->right);
			tree->right=NULL;//evaluate freed tree_>right
			if(!rightValue){
				if(tree->opType==0&&leftValue){
					printBigIntHex(leftValue,true);
					putchar('\n');
				}
				freeTree(tree);
				return NULL;
			}
		}else if(tree->rightType==DATA_BIGINT){
			rightValue=tree->right;
			tree->right=NULL;
		}else if((tree->rightType!=DATA_NONE)&&(tree->rightType!=DATA_NULL)){
			if(tree->opType==0&&leftValue){
				printBigIntHex(leftValue,true);
				putchar('\n');
			}else{
				freeBigInt(leftValue);
			}
			freeTree(tree);
			return NULL;
		}
		switch(tree->opType){
					case 0:
						if(leftValue&&rightValue){
							printBigIntHex(leftValue,true);
							putchar('\n');
							freeTree(tree);
							return rightValue;
						}else if(leftValue){
							freeTree(tree);
							return leftValue;
						}else if(rightValue){
							freeTree(tree);
							return rightValue;
						}
						freeTree(tree);
						return NULL;
					case OPTYPE_SGN:
						if(leftValue){
							freeBigInt(leftValue);
							freeTree(tree);
							return NULL;
						}else if(rightValue){
							freeTree(tree);
							int sgn=bigIntSgn(rightValue);
							free(rightValue);
							return createBigIntInt(sgn);
						}
						break;
					case OPTYPE_PLUS:
						if(leftValue){
							freeBigInt(leftValue);
							freeTree(tree);
							return NULL;
						}else if(rightValue){
							freeTree(tree);
							return rightValue;
						}
						break;
					case OPTYPE_NEG:
						if(leftValue){
							freeBigInt(leftValue);
							freeTree(tree);
							return NULL;
						}else if(rightValue){
							freeTree(tree);
							return negateBigInt(rightValue,true);
						}
						break;
					case OPTYPE_NOT:
						if(leftValue){
							freeBigInt(leftValue);
							freeTree(tree);
							return NULL;
						}else if(rightValue){
							freeTree(tree);
							return notBigInt(rightValue,true);
						}
						break;
					case OPTYPE_ADD:
						if(leftValue&&rightValue){
							freeTree(tree);
							return addBigInt(leftValue,true,rightValue,true);
						}else{
							freeBigInt(leftValue);
							freeBigInt(rightValue);
							freeTree(tree);
							return NULL;
						}
						break;
					case OPTYPE_SUBT:
						if(leftValue&&rightValue){
							freeTree(tree);
							return subtBigInt(leftValue,true,rightValue,true);
						}else{
							freeBigInt(leftValue);
							freeBigInt(rightValue);
							freeTree(tree);
							return NULL;
						}
						break;
					case OPTYPE_MULT:
						if(leftValue&&rightValue){
							freeTree(tree);
							return multBigInt(leftValue,true,rightValue,true);
						}else{
							freeBigInt(leftValue);
							freeBigInt(rightValue);
							freeTree(tree);
							return NULL;
						}
						break;
					case OPTYPE_DIV:
						if(leftValue&&rightValue){
							freeTree(tree);
							return divBigInt(leftValue,true,rightValue,true);
						}else{
							freeBigInt(leftValue);
							freeBigInt(rightValue);
							freeTree(tree);
							return NULL;
						}
						break;
					case OPTYPE_MOD:
						if(leftValue&&rightValue){
							freeTree(tree);
							return modBigInt(leftValue,true,rightValue,true);
						}else{
							freeBigInt(leftValue);
							freeBigInt(rightValue);
							freeTree(tree);
							return NULL;
						}
						break;
					case OPTYPE_AND:
						if(leftValue&&rightValue){
							freeTree(tree);
							return andBigInt(leftValue,true,rightValue,true);
						}else{
							freeBigInt(leftValue);
							freeBigInt(rightValue);
							freeTree(tree);
							return NULL;
						}
						break;
					case OPTYPE_OR:
						if(leftValue&&rightValue){
							freeTree(tree);
							return orBigInt(leftValue,true,rightValue,true);
						}else{
							freeBigInt(leftValue);
							freeBigInt(rightValue);
							freeTree(tree);
							return NULL;
						}
						break;
					case OPTYPE_XOR:
						if(leftValue&&rightValue){
							freeTree(tree);
							return xorBigInt(leftValue,true,rightValue,true);
						}else{
							freeBigInt(leftValue);
							freeBigInt(rightValue);
							freeTree(tree);
							return NULL;
						}
						break;
					case OPTYPE_POW:
						if(leftValue&&rightValue){
							freeTree(tree);
							return powBigInt(leftValue,true,rightValue,true);
						}else{
							freeBigInt(leftValue);
							freeBigInt(rightValue);
							freeTree(tree);
							return NULL;
						}
						break;
					default:
						freeBigInt(leftValue);
						freeBigInt(rightValue);
						freeTree(tree);
						return NULL;
				}
	}
	return NULL;
}

int main(void) {
	for(int i=0;i<16;i++){
		fputs("\n> ",stdout);
		fflush(stdout);
		StringArray *line =getLine();
		if(line){
			if(line->length==1&&(strcmp(line->strings[0].chars,"exit")==0)){
				freeStringArray(line);
				return EXIT_SUCCESS;
			}
			TreeNode *tree = parse(line);
			if(tree){
				fputs("Expression Tree: ",stdout);
				printTree(tree);
				fputs("\nResult: ",stdout);
				fflush(stdout);
				BigInt* tmp=evaluateTree(tree);
				if(tmp){
					printBigInt(tmp,true,stdout,10);
				}else{
					fputs("evaluation error",stdout);
				}
				putchar('\n');
			}else{
				puts("parsing error");
			}
			fflush(stdout);
		}
	}
	fputs("\n> exit\n ",stdout);
	return EXIT_SUCCESS;
}
