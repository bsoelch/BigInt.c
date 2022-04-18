/*8
 * BigInt.c
 *
 *  Created on: 28.09.2021
 *  Author: bsoelch
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "BigInt.h"

static const int INT_BITS = 32;
//TODO add int-pointer errorCode parameter to functions

//value for switching from standard multiplication to Karatsuba-algorithm
//(small) experiment points to values between 20 and 40 being the best
static const size_t KARATSUBA_THRESHOLD = 30;

static const int BIGINT_MAX_BASE = 62;

//TODO memory cleanup if one of the arguments is NULL

typedef struct BigIntStruct{
	//sgn data (0 if this number is >=0, UINT32_MAX if number is <0)
	uint32_t sgn;
	//addLater allow capacity>size
	size_t size;
	//unsigned bit-data of this number (in little endian notation)
	uint32_t* data;//addLater? handle BigEndian encodings
}BigInt;

static BigInt BIG_VAL_ZERO;
static BigInt BIG_VAL_ONE;
BigInt* BIG_INT_ZERO=&BIG_VAL_ZERO;
BigInt* BIG_INT_ONE=&BIG_VAL_ONE;
static BigInt BIG_VAL_ZERO=(BigInt){
		.sgn=0,
		.size=0,
		.data=NULL
};
static BigInt BIG_VAL_ONE=(BigInt){
		.sgn=0,
		.size=1,
		.data=(uint32_t[]){1}
};


//creates a BigInt from a 64bit int value
BigInt* createBigIntInt(int64_t value){
	BigInt* create=malloc(sizeof(BigInt));
	if(create!=NULL){
		if(value==0){
			create->size=0;
			create->sgn=0;
			create->data=NULL;
			return create;
		}else{
			if(value<0){
				create->sgn=UINT32_MAX;
				value=-value;
			}else{
				create->sgn=0;
			}
			if(value&0xffffffff00000000ULL){
				create->size=2;
				create->data=malloc(2*sizeof(int32_t));
				if(create->data){
					create->data[0]=value&UINT32_MAX;
					create->data[1] = (value >> INT_BITS) & UINT32_MAX;
				}else{
					freeBigInt(create);
					return NULL;
				}
			}else{
				create->size=1;
				create->data=malloc(sizeof(int32_t));
				if(create->data){
					create->data[0]=value&0xffffffff;
				}else{
					freeBigInt(create);
					return NULL;
				}
			}
		}
	}
	return create;
}
static uint32_t internal_digitFromChar(char c,bool bigBase){
	if(c>='0'&&c<='9'){
		return c-'0';
	}else if(c>='A'&&c<='Z'){
		return c-'A'+10;
	}else if(c>='a'&&c<='z'){
		return c-'a'+(bigBase?36:10);
	}else{
		return UINT32_MAX;
	}
}
/*creates a BigInt from a null-terminated string
 *base should be between 2 and 62*///addLater describe case handling
BigInt* createBigIntCStr(const char* str,int base){
	return createBigIntStr(str,strlen(str),base);
}
/*creates a BigInt from a string
 *base should be between 2 and 62*///addLater describe case handling
BigInt* createBigIntStr(const char* stringValue,size_t stringLen,int base){
	if(base<2||base>BIGINT_MAX_BASE){
		return NULL;
	}
	BigInt* ret=createBigIntInt(0);//addLater shortcut-method for pow2 bases
	BigInt* bigIntBase=createBigIntInt(base);
	bool bigBase=base>36;
	for(size_t i=0;i<stringLen;i++){//addLater? more efficient algorithm
		ret=multBigInt(ret,true,bigIntBase,false);
		if(!ret){
			break;
		}
		uint32_t digit=internal_digitFromChar(stringValue[i],bigBase);
		if(digit<base){
			ret=addBigInt(ret,true,createBigIntInt(digit),true);
			if(!ret){
				break;
			}
		}else{
			freeBigInt(ret);
			freeBigInt(bigIntBase);
			return NULL;
		}
	}
	freeBigInt(bigIntBase);
	return ret;
}
/**creates a BigInt from the given array of integers*/
BigInt* createBigIntBytes(void* bytes,size_t numBytes){
	return NULL;//TODO createBigIntBytes
}
/**creates a BigInt from the given array of integers*/
BigInt* createBigIntInts(uint32_t sgn,uint32_t* value,size_t size){
	BigInt* create=malloc(sizeof(BigInt));
	if(create){
		create->sgn=sgn;
		create->data=value;
		create->size=value==NULL?0:size;
	}
	return create;
}
//returns uninitialized BigInt with the given size
static BigInt* createBigIntSize(size_t size){
	   uint32_t* newData=malloc(size*sizeof(uint32_t));
	   if(newData){
		   BigInt* ret=malloc(sizeof(BigInt));
		   if(ret){
			   ret->size=size;
			   ret->sgn=0;
			   ret->data=newData;
		   }else{
			   free(newData);
			   return NULL;
		   }
		   return ret;
	   }
	return NULL;
}

/**creates a clone of the given BigInteger*/
BigInt* cloneBigInt(BigInt* source){
	if(source){
		BigInt* clone=malloc(sizeof(BigInt));
		if(clone){
			clone->size=source->size;
			clone->sgn=source->sgn;
			clone->data=malloc(source->size*sizeof(uint32_t));
			if(clone->data){
				memcpy(clone->data,source->data,source->size*sizeof(uint32_t));
			}else{
				freeBigInt(clone);
				return NULL;
			}
		}
		return clone;
	}
	return NULL;
}

/**frees the given BigInteger*/
void freeBigInt(BigInt* toFree){
	if(toFree){
		free(toFree->data);
		toFree->data=NULL;
		free(toFree);
	}
}

static BigInt* internal_standardizeBigInt(BigInt* number){
	if(number){
		if(number->size==0){
			number->sgn=0;
			if(number->data){
				free(number->data);
				number->data=NULL;
			}
		}else if(number->data){
			assert((number->size)>0);
			if(number->data[number->size-1]==0){
				while(number->data[number->size-1]==0&&number->size>=2){
					number->size--;
				}
				if(number->size==1&&number->data[0]==0){
					number->size=0;
					free(number->data);
					number->data=NULL;
					return number;
				}
				if(number->size==0){
					free(number->data);
					number->data=NULL;
				}else{
					uint32_t * tmp=realloc(number->data,number->size*sizeof(uint32_t));
					if(tmp){
						number->data=tmp;
					}else{
						freeBigInt(number);
						return NULL;
					}
				}
			}
		}else{
			freeBigInt(number);
			return NULL;
		}
	}
	return number;
}


void printBigIntHex(BigInt* number,bool consume){
	if(number){
		if(number->sgn){
			putchar('-');
		}
		if(number->size==0){
		 fputs("0",stdout);
		}else{
			bool hadNonZero=false;
			int val;
			for(size_t i=number->size-1;i!=SIZE_MAX;i--){//! size_t is always >=0
				for (int shift = INT_BITS - 4; shift >= 0; shift -= 4) {
					val=((number->data[i])>>shift)&0xF;
					if(val<0xa){
						if(val!=0){
							hadNonZero=true;
						}//no else
						if(hadNonZero){
							putchar('0'+val);
						}
					}else{
						putchar('a'+val-10);
					}
				}
				if(i>0)
					putchar('.');
			}
		}
		if(consume){
			freeBigInt(number);
		}
	}else{
		fputs("NULL",stdout);
	}
}

//value of max Power of base that is less than or equal to 2^32
static BigInt* internal_maxPowerInt(int base){
	switch(base){
		case 2:  return createBigIntInt(4294967296);
		case 3:  return createBigIntInt(3486784401);
		case 4:  return createBigIntInt(4294967296);
		case 5:  return createBigIntInt(1220703125);
		case 6:  return createBigIntInt(2176782336);
		case 7:  return createBigIntInt(1977326743);
		case 8:  return createBigIntInt(1073741824);
		case 9:  return createBigIntInt(3486784401);
		case 10: return createBigIntInt(1000000000);
		case 11: return createBigIntInt(2357947691);
		case 12: return createBigIntInt(429981696);
		case 13: return createBigIntInt(815730721);
		case 14: return createBigIntInt(1475789056);
		case 15: return createBigIntInt(2562890625);
		case 16: return createBigIntInt(4294967296);
		case 17: return createBigIntInt(410338673);
		case 18: return createBigIntInt(612220032);
		case 19: return createBigIntInt(893871739);
		case 20: return createBigIntInt(1280000000);
		case 21: return createBigIntInt(1801088541);
		case 22: return createBigIntInt(2494357888);
		case 23: return createBigIntInt(3404825447);
		case 24: return createBigIntInt(191102976);
		case 25: return createBigIntInt(244140625);
		case 26: return createBigIntInt(308915776);
		case 27: return createBigIntInt(387420489);
		case 28: return createBigIntInt(481890304);
		case 29: return createBigIntInt(594823321);
		case 30: return createBigIntInt(729000000);
		case 31: return createBigIntInt(887503681);
		case 32: return createBigIntInt(1073741824);
		case 33: return createBigIntInt(1291467969);
		case 34: return createBigIntInt(1544804416);
		case 35: return createBigIntInt(1838265625);
		case 36: return createBigIntInt(2176782336);
		case 37: return createBigIntInt(2565726409);
		case 38: return createBigIntInt(3010936384);
		case 39: return createBigIntInt(3518743761);
		case 40: return createBigIntInt(4096000000);
		case 41: return createBigIntInt(115856201);
		case 42: return createBigIntInt(130691232);
		case 43: return createBigIntInt(147008443);
		case 44: return createBigIntInt(164916224);
		case 45: return createBigIntInt(184528125);
		case 46: return createBigIntInt(205962976);
		case 47: return createBigIntInt(229345007);
		case 48: return createBigIntInt(254803968);
		case 49: return createBigIntInt(282475249);
		case 50: return createBigIntInt(312500000);
		case 51: return createBigIntInt(345025251);
		case 52: return createBigIntInt(380204032);
		case 53: return createBigIntInt(418195493);
		case 54: return createBigIntInt(459165024);
		case 55: return createBigIntInt(503284375);
		case 56: return createBigIntInt(550731776);
		case 57: return createBigIntInt(601692057);
		case 58: return createBigIntInt(656356768);
		case 59: return createBigIntInt(714924299);
		case 60: return createBigIntInt(777600000);
		case 61: return createBigIntInt(844596301);
		case 62: return createBigIntInt(916132832);
		default: return NULL;
	}
}
//max Power of base that is less than or equal to 2^32
static uint64_t internal_maxPowers(int base){
	switch(base){
	case 2: return 32;
	case 3:  return 20;
	case 4:  return 16;
	case 5:  return 13;
	case 6:  return 12;
	case 7:  return 11;
	case 8:  return 10;
	case 9:  return 10;
	case 10: return 9;
	case 11: return 9;
	case 12: return 8;
	case 13: return 8;
	case 14: return 8;
	case 15: return 8;
	case 16: return 8;
	case 17: return 7;
	case 18: return 7;
	case 19: return 7;
	case 20: return 7;
	case 21: return 7;
	case 22: return 7;
	case 23: return 7;
	case 24: return 6;
	case 25: return 6;
	case 26: return 6;
	case 27: return 6;
	case 28: return 6;
	case 29: return 6;
	case 30: return 6;
	case 31: return 6;
	case 32: return 6;
	case 33: return 6;
	case 34: return 6;
	case 35: return 6;
	case 36: return 6;
	case 37: return 6;
	case 38: return 6;
	case 39: return 6;
	case 40: return 6;
	case 41: return 5;
	case 42: return 5;
	case 43: return 5;
	case 44: return 5;
	case 45: return 5;
	case 46: return 5;
	case 47: return 5;
	case 48: return 5;
	case 49: return 5;
	case 50: return 5;
	case 51: return 5;
	case 52: return 5;
	case 53: return 5;
	case 54: return 5;
	case 55: return 5;
	case 56: return 5;
	case 57: return 5;
	case 58: return 5;
	case 59: return 5;
	case 60: return 5;
	case 61: return 5;
	case 62: return 5;
	default: return -1;
	}
}


static void internal_printInt64(BigInt *toPrint, int base,uint64_t width, FILE *target) {
	uint64_t data =(toPrint->size>0)?toPrint->data[0]:0;
	if(toPrint->size>1){
		data |= (((uint64_t) toPrint->data[1]) << INT_BITS);
	}
	char buff[64];//buffer big enough to hold all digits
	int i = 0;
	while (data != 0) {
		buff[i++] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"[data % base];
		data /= base;
	}
	while(i<width){
		putc('0', target);
		width--;
	}
	while (i > 0) {
		putc(buff[--i], target);
	}
}

int printBigInt(BigInt *toPrint, _Bool consume, FILE *target, int base) {
	if (toPrint) {
		if (base < 2 || base > BIGINT_MAX_BASE) {
			return -1; //addLater error codes
		} //no else
		if (toPrint->size == 0) {
			fputs("0", target);
			return 0;
		} //no else
		if (toPrint->sgn) {
			putc('-', target);
			toPrint = negateBigInt(toPrint, consume);
			consume = true;
		}
		if (toPrint->size <= 2) {
			internal_printInt64(toPrint, base, 0,target);
			if(consume){
				freeBigInt(toPrint);
			}
			return 0;
		}
		BigInt* pows[64]; //more than enough buffer
		int powX = 0;
		int powCount = 1;
		BigInt* stack[64]={0};
		int stackSize = 0;
		pows[0] = internal_maxPowerInt(base);
		if (!pows[0]) {
			if (consume) {
				freeBigInt(toPrint);
			}
			return -1;
		}
		while (2 * pows[powX]->size <= toPrint->size) {
			pows[powX + 1] = multBigInt(pows[powX], false, pows[powX], false);
			if (pows[++powX]) {
				powCount++;
			} else {
				while (powCount > 0) {
					freeBigInt(pows[--powCount]);
				}
				if (consume) {
					freeBigInt(toPrint);
				}
				return -1;
			}
		}

		DivModResult divMod;
		bool first=true;
		while (toPrint) {
			while (toPrint->size > 2) {
				assert(powX>=0);
				divMod = divModBigInt(toPrint, consume, pows[powX--], false, true,true);
				consume=true;
				if (divMod.result && divMod.remainder) {
					toPrint = divMod.result;
					stack[stackSize++] = divMod.remainder;
				} else {
					//division failed
					freeBigInt(divMod.result);
					freeBigInt(divMod.remainder);
					while (powCount > 0) {
						freeBigInt(pows[--powCount]);
					}
					while (stackSize > 0) {
						freeBigInt(stack[--stackSize]);
					}
					return -1;
				}
			}
			if(first){
				internal_printInt64(toPrint, base,0, target);
				first = toPrint->size==0;
			}else{
				internal_printInt64(toPrint, base,internal_maxPowers(base)<<(powX+1), target);
			}
			freeBigInt(toPrint);
			toPrint=NULL;
			if (stackSize > 0) {
				toPrint = stack[--stackSize];
				stack[stackSize]=NULL;
				while (powX+1<powCount && cmpBigInt(pows[powX+1],toPrint)<0) {
					powX++;
				}
			}
		}
		while (powCount > 0) {
			freeBigInt(pows[--powCount]);
		}
		return 0;
	}else{
		fputs("(NULL)",target);
		return 0;
	}
}

//TODO bigIntBytes()

/**returns the sign of a (a<0?-1:a>0?1:0)*/
int bigIntSgn(BigInt* a){
	if(a && (a->size) >0){//a is not NULL or 0
		return (a->sgn) ? -1 : 1;
	}
	return 0;
}


/**Compares |a| and |b|
 * returns negative value if |a|<|b|; 0 if |a|==|b| and positive value if |a|>|b|*/
static int cmpBigIntAbs(BigInt* a,BigInt* b){
	if(a&&b){
		if( a->size > b->size ){
			return 1;//a>b if sgnA=sgnB=1
		}else if( a->size < b->size ){
			return -1;//a<b if sgnA=sgnB=1
		}else{
			//block-wise compare form high to low locks
			for(size_t i=a->size-1;i!=SIZE_MAX;i--){//! >=0 not possible for uint
				if(a->data[i] < b -> data[i]){
					return -1;
				}else if(a->data[i] > b -> data[i]){
					return 1;
				}
			}
		}
	}
	return 0;
}

/**Compares a and b
 * returns negative value if a<b; 0 if a==b and positive value if a>b*/
int cmpBigInt(BigInt* a,BigInt* b){
	if(a==b)
		return 0;
	int sgnA=bigIntSgn(a),sgnB=bigIntSgn(b);
	if(sgnA<sgnB){
		return -1;
	}else if(sgnA>sgnB){
		return 1;
	}else if(sgnA==0){
		return 0;
	}else{//sgnA == sgnB
		return sgnA*cmpBigIntAbs(a,b);
	}
	return 0;//a==0 && b==0
}
/**Compares a BigInt a and an Integer b
 * returns negative value if a<b; 0 if a==b and positive value if a>b*/
int cmpBigIntInt(BigInt* a,int64_t b){
	int sgnA=bigIntSgn(a),sgnB;
	if(b<0){
		b=-b;
		sgnB=-1;
	}else if(b>0){
		sgnB=1;
	}else{
		sgnB=0;
	}
	if(sgnA<sgnB){
		return -1;
	}else if(sgnA>sgnB){
		return 1;
	}else if(sgnA==0){
		return 0;
	}else{//sgnA == sgnB
		assert(a->size>0);
		uint64_t intVal;
		if(a->size>2){
			return sgnA;
		}else if(a->size==2){
			intVal=(((uint64_t)a->data[1])<<INT_BITS)|a->data[0];
		}else if(a->size==1){
			intVal=a->data[0];
		}
		if(intVal<(uint64_t)b){
			return -sgnA;
		}else if(intVal>(uint64_t)b){
			return sgnA;
		}
	}
	return 0;//a==NULL or a==0 && b==0
}

/**flips all bits in a,
 * if mutable is true, the argument a is directly modified*/
BigInt* notBigInt(BigInt* a,bool consumeA){
	if(!consumeA){
		a=cloneBigInt(a);
	}
	if(a){
		if(a->size>0){
			bool searchNZ=true;
			uint64_t buffer=a->data[0]+(a->sgn==0?1:-1);
			a->data[0]=buffer&UINT32_MAX;
			searchNZ=(a->data[0]==0);
			buffer >>= INT_BITS;
			for(size_t i=1;buffer!=0&&i<a->size;i++){
				buffer+=a->data[i];
				a->data[i]=buffer&UINT32_MAX;
				if(searchNZ){
					searchNZ=(a->data[i]==0);
				}
				buffer >>= INT_BITS;
			}
			if(buffer!=0){
				uint32_t* tmp=realloc(a->data,(a->size+1)*sizeof(uint32_t));
				if(tmp==NULL){
					freeBigInt(a);
					return NULL;
				}else{
					a->data=tmp;
					a->data[a->size++]=buffer&UINT32_MAX;
				}
			}
			return negateBigInt(internal_standardizeBigInt(a),true);
		}else{
			freeBigInt(a);
			return createBigIntInt(-1);
		}
	}
	return NULL;
}

//helper for logical operations
static uint32_t internal_2compBlock(BigInt* bigInt,size_t i,bool* hadNZ){
	if(i<bigInt->size){
		if(bigInt->sgn){
			if(*hadNZ){
				return ~bigInt->data[i];
			}else{
				*hadNZ=bigInt->data[i]!=0;
				return -bigInt->data[i];
			}
		}else{
			return bigInt->data[i];
		}
	}else{
		return bigInt->sgn;
	}
}

//assumes that all arguments are not NULL,
//and that target->size == big->size >= small->size
static void internal_unsavelogicalOp(BigInt* target,BigInt* big,BigInt* small,
		uint32_t (*f)(uint32_t,uint32_t),bool isAnd){
	size_t i=0;
	bool hadNZB=false,hadNZS=false;
	for(;i<small->size;i++){
	   target->data[i]=f(internal_2compBlock(big,i,&hadNZB),internal_2compBlock(small,i,&hadNZS));
	}
	uint32_t defVal=small->sgn;
	//TODO replace isAnd with tail0,tail1 [val,op,tail]
	if(isAnd==(defVal==0)){//TODO handle xor differently
		if(defVal==0){
			target->size=i;
			uint32_t* tmp=realloc(target->data,i*sizeof(uint32_t));
			if(tmp){
				target->data=tmp;
			}else{
				target->data=NULL;
				return;
			}
		}else{
			memset(i+(target->data),UINT32_MAX,(big->size - small->size)*sizeof(uint32_t));
			target->size=big->size;
		}
	}else{//addLater shortcut
		for(;i<big->size;i++){
		   target->data[i]=f(internal_2compBlock(big,i,&hadNZB),defVal);
		}
		target->size=big->size;
	}
	target->sgn=f(big->sgn,small->sgn);
	if(target->sgn){//decode 2 complement
		for(i=0;i<target->size;i++){
		   target->data[i]=-target->data[i];
		   if(target->data[i]){
			   i++;
			   break;
		   }
		}//after first non-zero element ~ instead of -
		for(;i<target->size;i++){
		   target->data[i]=~target->data[i];
		}
	}
}

static uint32_t internal_uint32and(uint32_t a,uint32_t b){
	return a&b;
}
static uint32_t internal_uint32or(uint32_t a,uint32_t b){
	return a|b;
}

static uint32_t internal_uint32xor(uint32_t a,uint32_t b){
	return a^b;
}
static BigInt* internal_logicalOp(BigInt* a,bool consumeA,BigInt* b,bool consumeB,
		uint32_t (*op)(uint32_t,uint32_t),bool isAnd){
	if(a&&b){
		if(a==b){//can only consume one
			consumeA&=consumeB;//only consume if both can be consumed
			consumeB=false;
		}
		if(a->size >= b->size){
			if(consumeA){
				internal_unsavelogicalOp(a,a,b,op,isAnd);
				if(consumeB){
					freeBigInt(b);
					b=NULL;
				}
				a=internal_standardizeBigInt(a);
				return a;
			}else if(consumeB){
				uint32_t* tmp=realloc(b->data,(a->size)*sizeof(uint32_t));
				if(tmp){
					b->data=tmp;
					internal_unsavelogicalOp(b,a,b,op,isAnd);
				}else{
					freeBigInt(b);
					return NULL;
				}
				b=internal_standardizeBigInt(b);
				return b;
			}else{
				BigInt* res=createBigIntSize(a->size);
				if(res){
					internal_unsavelogicalOp(res,a,b,op,isAnd);
					return internal_standardizeBigInt(res);
				}
			}
		}else{
			if(consumeB){
				internal_unsavelogicalOp(b,b,a,op,isAnd);
				if(consumeA){
					freeBigInt(a);
					a=NULL;
				}
				b=internal_standardizeBigInt(b);
				return b;
			}else if(consumeA){
				uint32_t* tmp=realloc(a->data,(b->size)*sizeof(uint32_t));
				if(tmp){
					a->data=tmp;
					internal_unsavelogicalOp(a,b,a,op,isAnd);
				}else{
					freeBigInt(a);
					return NULL;
				}
				a=internal_standardizeBigInt(a);
				return a;
			}else{
				BigInt* res=createBigIntSize(b->size);
				if(res){
					internal_unsavelogicalOp(res,b,a,op,isAnd);
					return internal_standardizeBigInt(res);
				}
			}
		}
	}else if(!isAnd){
		if(a){
			return a;
		}else if(b){
			return b;
		}
	}
	return NULL;
}

/*logical and of a and b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* andBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB){
	return internal_logicalOp(a,consumeA,b,consumeB,&internal_uint32and,true);
}
/*logical or of a and b
  arguments marked with consume will be deleted or overwritten by the calculation**/
BigInt* orBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB){
	return internal_logicalOp(a,consumeA,b,consumeB,&internal_uint32or,false);
}
/*logical exclusive or of a and b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* xorBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB){
	return internal_logicalOp(a,consumeA,b,consumeB,&internal_uint32xor,false);
}

/*returns a cop of a that is shifted by the given amount,
 * if amount is positive a is shifted to the left otherwise to the right */
BigInt* shiftBigInt(BigInt* a,bool consumeA,int64_t amount){
	if(a){
		if(amount==0||a->size==0){
			return consumeA?a:cloneBigInt(a);//no shift
		}else{
			if(amount<0){
				bool neg=false;
				if(a->sgn){//(-a)>>b = - ((a-1)>>b +1)
					a=addBigInt(a,consumeA,createBigIntInt(1),true);
					consumeA=true;
					neg=true;
				}
				amount=-amount;
				size_t shiftBlocks=amount>>5;// amount/32
				if(shiftBlocks>=a->size){
					if(consumeA){
						freeBigInt(a);
					}
					return createBigIntInt(neg?-1:0);
				}
				uint8_t shiftBits=amount&0x1f;// amount%32
				if(consumeA){
					memmove(a->data,a->data+shiftBlocks,(a->size-shiftBlocks)*sizeof(uint32_t));
					uint32_t* tmp=realloc(a->data,(a->size-shiftBlocks)*sizeof(uint32_t));
					if(tmp){
						a->data=tmp;
						a->size=a->size-shiftBlocks;
					}else{
						freeBigInt(a);
						return NULL;
					}
				}else{
					uint32_t* tmp=malloc((a->size-shiftBlocks)*sizeof(uint32_t));
					if(tmp){
						memcpy(tmp,a->data+shiftBlocks,(a->size-shiftBlocks)*sizeof(uint32_t));
						a=createBigIntInts(a->sgn,tmp,a->size-shiftBlocks);
						if(!a){
							free(tmp);
							return NULL;
						}
					}else{
						return NULL;
					}
				}
				if(shiftBits!=0){
					shiftBits = INT_BITS - shiftBits;
					uint64_t buffer=0;
					for(size_t i=a->size-1;i!=SIZE_MAX;i--){
						buffer|=((uint64_t)a->data[i])<<shiftBits;
						a->data[i] = (buffer >> INT_BITS) & UINT32_MAX;
						buffer <<= INT_BITS;
					}
				}
				if(neg){//(-a)>>b = - ((a-1)>>b +1)
					a=subtBigInt(a,true,createBigIntInt(1),true);
				}
			}else{
				size_t shiftBlocks=amount>>5;// amount/32
				uint8_t shiftBits=amount&0x1f;// amount%32
				if(consumeA){
					uint32_t* tmp=realloc(a->data,(a->size+shiftBlocks+(shiftBits!=0?1:0))*sizeof(uint32_t));
					if(tmp){
						memmove(tmp+shiftBlocks,tmp,a->size*sizeof(uint32_t));
						a->data=tmp;
						a->size=a->size+shiftBlocks;
					}else{
						freeBigInt(a);
						return NULL;
					}
				}else{
					uint32_t* tmp=malloc((a->size+shiftBlocks+(shiftBits!=0?1:0))*sizeof(uint32_t));
					if(tmp){
						memcpy(tmp+shiftBlocks,a->data,a->size*sizeof(uint32_t));
						a=createBigIntInts(a->sgn,tmp,a->size+shiftBlocks);
						if(!a){
							free(tmp);
							return NULL;
						}
					}else{
						return NULL;
					}
				}
				memset(a->data,0,shiftBlocks*sizeof(uint32_t));
				if(shiftBits!=0){
					uint64_t buffer=0;
					for(size_t i=shiftBlocks;i<a->size;i++){
						buffer|=((uint64_t)a->data[i])<<shiftBits;
						a->data[i]=buffer&UINT32_MAX;
						buffer >>= INT_BITS;
					}
					if(buffer!=0){
						a->data[a->size++]=buffer&UINT32_MAX;
					}
				}
			}
			return internal_standardizeBigInt(a);
		}
	}
	return NULL;
}

/*negates toNegate,
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* negateBigInt(BigInt* toNegate,bool consumeArg){
	if(!consumeArg){
		toNegate=cloneBigInt(toNegate);
	}//no else
	if(toNegate){
		if(toNegate->size>0){//dont negate 0
			toNegate->sgn^=UINT32_MAX;
		}
		return toNegate;
	}
	return NULL;
}

//assumes target,big,small!=NULL, target->capacity >= big->size >= small->size
//the sign values are ignored in the calculation
static void internal_unsaveAdd(BigInt* target,BigInt* big,BigInt* small){
	uint64_t carry=0;
	size_t i=0;
	for(;i<small->size;i++){
		carry+=((uint64_t)big->data[i])+small->data[i];
		target->data[i]=carry&UINT32_MAX;
		carry >>= INT_BITS;
	}
	for(;i<big->size;i++){
		carry+=big->data[i];
		target->data[i]=carry&UINT32_MAX;
		carry >>= INT_BITS;
	}
	if(carry!=0){
		uint32_t* tmp=realloc(target->data,(i+1)*sizeof(uint32_t));
		if(tmp){
			target->data=tmp;
			target->data[i++]=carry&UINT32_MAX;
		}else{
			free(target->data);
			target->data=NULL;
		}
	}
	target->size=i;
}
//assumes target,big,small!=NULL, target->capacity >= big->size, |big| >= |small|
//the sign values are ignored in the calculation
static void internal_unsaveSubt(BigInt* target,BigInt* big,BigInt* small){
	int64_t carry=0;
	size_t i=0;
	for(;i<small->size;i++){
		carry+=big->data[i];
		carry-=small->data[i];
		target->data[i]=carry&UINT32_MAX;
		carry >>= INT_BITS;
		if(carry&0x80000000){
			carry|=0xffffffff00000000;
		}
	}
	for(;i<big->size;i++){
		carry+=big->data[i];
		target->data[i]=carry&UINT32_MAX;
		carry >>= INT_BITS;
		if(carry&0x80000000){
			carry|=0xffffffff00000000;
		}
	}
	target->size=i;
	assert(carry==0);
}

/*adds a to b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* addBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB){
	if(a&&b){
		if(a==b){//can only consume one
			consumeA&=consumeB;//only consume if both can be consumed
			consumeB=false;
		}
		BigInt* tmp;
		if(a->sgn==b->sgn){
			if(a->size < b->size){
				tmp=a;
				a=b;
				b=tmp;
				if(consumeA!=consumeB){
					consumeA=!consumeA;
					consumeB=!consumeB;
				}
			}//len a>=len b
			if(consumeA){
				tmp=a;
			}else{
				if(consumeB){
					uint32_t* tmpP=realloc(b->data,(a->size+1)*sizeof(uint32_t));
					if(tmpP){
						b->data=tmpP;
						tmp=b;
						consumeB=false;//b should not be deleted
					}else{
						freeBigInt(b);
						return NULL;
					}
				}else{
					tmp=createBigIntSize(a->size+1);
					if(tmp){
						tmp->sgn=a->sgn;
					}else{
						return NULL;
					}
				}
			}
			internal_unsaveAdd(tmp,a,b);
			if(consumeB){
				freeBigInt(b);
				b=NULL;
			}
			return internal_standardizeBigInt(tmp);
		}else{
			if(cmpBigIntAbs(a,b)<0){
				tmp=a;
				a=b;
				b=tmp;
				if(consumeA!=consumeB){
					consumeA=!consumeA;
					consumeB=!consumeB;
				}
			}//|a|>=|b|
			if(consumeA){
				tmp=a;
			}else{
				if(consumeB){
					uint32_t* tmpP=realloc(b->data,a->size*sizeof(uint32_t));
					if(tmpP){
						b->data=tmpP;
						tmp=b;
						tmp->sgn=a->sgn;
						consumeB=false;//b should not be deleted
					}else{
						freeBigInt(b);
						tmp=NULL;
					}
				}else{
					tmp=createBigIntSize(a->size);
					if(tmp){
						tmp->sgn=a->sgn;
					}else{
						tmp=NULL;
					}
				}
			}
			if(tmp){
				internal_unsaveSubt(tmp,a,b);
				if(consumeB){
					freeBigInt(b);
					b=NULL;
				}
			}
			return internal_standardizeBigInt(tmp);
		}

	}
	return NULL;
}


/**subtracts b from a
  arguments marked with consume will be deleted or overwritten by the calculation  */
BigInt* subtBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB){
	return addBigInt(a,consumeA,negateBigInt(b,consumeB),true);
}

static void internal_sqOverflow(BigInt *val, size_t target, uint64_t buffer
		,uint64_t overflow) {
	val->data[target++] = buffer & UINT32_MAX;
	buffer >>= INT_BITS;
	while (overflow != 0) {
		buffer += overflow & UINT32_MAX;
		overflow >>= INT_BITS;
		buffer += val->data[target];
		val->data[target++] = buffer & UINT32_MAX;
		buffer >>= INT_BITS;
	}
	while (buffer != 0) {
		buffer += val->data[target];
		val->data[target++] = buffer & UINT32_MAX;
		buffer >>= INT_BITS;
	}
}
static void internal_inPlaceSquare(BigInt* val){
	uint64_t buffer, overflow;
	bool canOverflow;
	size_t N=val->size-1;
	//iterate through all pairs (i,j), 0<=i,j<=N in order of descending i+j
	for(size_t s=0;s<N;s++){
		buffer=overflow=0;
		for(size_t i=0;i<=s;i++){
			canOverflow=buffer>>63;
			buffer+=((uint64_t)val->data[N-i])*val->data[N-(s-i)];
			if(canOverflow&&(buffer>>63==0)){
				overflow++;
			}
		}
		internal_sqOverflow(val, 2*N-s, buffer, overflow);
	}
	for(size_t s=N;s!=SIZE_MAX;s--){
		buffer=overflow=0;
		for(size_t i=0;i<=s;i++){
			canOverflow=buffer>>63;
			buffer+=((uint64_t)val->data[i])*val->data[s-i];
			if(canOverflow&&(buffer>>63==0)){
				overflow++;
			}
		}
		internal_sqOverflow(val, s, buffer, overflow);
	}
	val->size*=2;
}

static void internal_unsaveMult(BigInt* target,BigInt* left,BigInt* right){
	target->data[left->size+right->size-1]=0;
	target->sgn=(left->sgn)^(right->sgn);
	if(right==target){//algorithm only works of right!=target
		if(left==target){
			internal_inPlaceSquare(target);
			return;
		}
		BigInt* tmp=left;
		left=right;
		right=tmp;
	}
	uint64_t buffer,tmp;
	for(size_t l=left->size-1;l!=SIZE_MAX;l--){
		for(size_t r=right->size-1;r!=SIZE_MAX;r--){
			buffer=((uint64_t)left->data[l])*right->data[r];
			tmp=l+r;
			if(r==0||l==(left->size-1)){
				target->data[tmp++]=buffer&UINT32_MAX;
			}else{
				buffer+=target->data[tmp];
				target->data[tmp++]=buffer&UINT32_MAX;
			}
			buffer >>= INT_BITS;
			while(buffer!=0){
				buffer+=target->data[tmp];
				target->data[tmp++]=buffer&UINT32_MAX;
				buffer >>= INT_BITS;
			}
		}
	}
	target->size=left->size+right->size;
}

static void updateSize(BigInt* anInt){
	anInt->size--;//offset by one to remove the -1 in array access
	while((anInt->size!=SIZE_MAX)&&(anInt->data[anInt->size]==0)){
		anInt->size--;
	}
	anInt->size++;//remove offset
}
//Multiplies big and small with the Karatsuba multiplication algorithm,
//neither big nor small is modified in the calculation
//assumptions big,small !=NULL, big>small>0
static BigInt* internal_karatsubaMult(BigInt* big,BigInt* small){
	//printf("Debug: K-mult: %p->%p, %p->%p\n",big,big->data,small,small->data);
	size_t half=big->size/2;
	BigInt* res=NULL;
	BigInt* bigH=malloc(sizeof(BigInt));
	BigInt* bigL=malloc(sizeof(BigInt));
	if(bigH&&bigL){
		bigH->sgn=bigL->sgn=0;
		bigL->data=big->data;
		bigL->size=half;
		updateSize(bigL);
		bigH->data=big->data+half;
		bigH->size=big->size-half;
		if(half<small->size){
			BigInt* smallH=malloc(sizeof(BigInt));
			BigInt* smallL=malloc(sizeof(BigInt));
			if(smallH&&smallL){
				smallH->sgn=smallL->sgn=0;
				smallL->data=small->data;
				smallL->size=half;
				updateSize(smallL);
				smallH->data=small->data+half;
				smallH->size=small->size-half;
				BigInt* high=multBigInt(bigH,false,smallH,false);
				if(high){
					BigInt* low=multBigInt(bigL,false,smallL,false);
					if(low){
						BigInt* mid=multBigInt(addBigInt(bigH,false,bigL,false),true,
								addBigInt(smallH,false,smallL,false),true);
						mid=subtBigInt(mid,true,high,false);
						mid=subtBigInt(mid,true,low,false);
						assert(mid->sgn==0);
						//assert(mid->sgn==0);
						res=createBigIntSize(high->size+2*half+1);
						if(mid&&res){
							memcpy(res->data,low->data,low->size*sizeof(uint32_t));
							assert(low->size<=2*half);
							memset(res->data+low->size,0,(2*half-low->size)*sizeof(uint32_t));
							freeBigInt(low);
							low=NULL;
							memcpy(res->data+2*half,high->data,high->size*sizeof(uint32_t));
							freeBigInt(high);
							high=NULL;
							res->data[res->size-1]=0;//overflow block
							//addLater addWithOffset?
							uint64_t buffer=0;
							size_t i=0;
							for(;i<mid->size;i++){
								buffer+=((uint64_t)mid->data[i])+res->data[i+half];
								res->data[i+half]=buffer&UINT32_MAX;
								buffer >>= INT_BITS;
							}
							freeBigInt(mid);
							mid=NULL;
							if(buffer){
								i+=half;
								for(;i<res->size;i++){
									buffer+=res->data[i];
									res->data[i]=buffer&UINT32_MAX;
									buffer >>= INT_BITS;
									if(!buffer)
										break;
								}
								assert(buffer==0);
							}
						}else{
							freeBigInt(high);
							freeBigInt(low);
							freeBigInt(mid);
							freeBigInt(res);
							high=low=mid=res=NULL;
						}
					}else{
						freeBigInt(high);
						high=NULL;
					}
				}
			}
			smallH->data=smallL->data=NULL;//no need to free ->data since it is still bound to the original arguments
			free(smallH);
			smallH=NULL;
			free(smallL);
			smallL=NULL;
		}else{
			BigInt* high=multBigInt(bigH,false,small,false);
			if(high){
				BigInt* low=multBigInt(bigL,false,small,false);
				res=createBigIntSize(high->size+half+1);
				if(low&&res){
					memcpy(res->data,low->data,low->size*sizeof(uint32_t));
					memset(res->data+low->size,0,(res->size-low->size)*sizeof(uint32_t));
					freeBigInt(low);
					low=NULL;
					res->data[res->size-1]=0;//overflow block
					//addLater addWithOffset?
					uint64_t buffer=0;
					size_t i=0;
					for(;i<high->size;i++){
						buffer+=((uint64_t)high->data[i])+res->data[i+half];
						res->data[i+half]=buffer&UINT32_MAX;
						buffer >>= INT_BITS;
					}
					freeBigInt(high);
					high=NULL;
					if(buffer){
						i+=half;
						buffer+=res->data[i];
						res->data[i]=buffer&UINT32_MAX;
						buffer >>= INT_BITS;
						assert(buffer==0);
					}
				}else{
					freeBigInt(high);
					freeBigInt(low);
					freeBigInt(res);
					high=low=res=NULL;
				}
			}
		}
	}
	bigH->data=bigL->data=NULL;//no need to free ->data since it is still bound to the original arguments
	free(bigH);
	bigH=NULL;
	free(bigL);
	bigL=NULL;
	if(res){
		res->sgn=big->sgn^small->sgn;
	}
	return internal_standardizeBigInt(res);
}

/**multiples a by b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* multBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB){
	if(a&&b){
		if(a==b){//can only consume one
			consumeA&=consumeB;//only consume if both can be consumed
			consumeB=false;
			//addLater call squareBigInt here
		}
		if(a->size==0){//one of the factors is 0
			if(consumeB){
				freeBigInt(b);
			}
			return consumeA?a:createBigIntInt(0);
		}else if(b->size==0){
			if(consumeA){
				freeBigInt(a);
			}
			return consumeB?b:createBigIntInt(0);
		}
		BigInt* tmp;
		if(a->size < b->size){
			tmp=a;
			a=b;
			b=tmp;
			if(consumeA!=consumeB){
				consumeA=!consumeA;
				consumeB=!consumeB;
			}
		}//len a>=len b
		if (a->size > KARATSUBA_THRESHOLD){
			BigInt* ret=internal_karatsubaMult(a,b);
			if(consumeA){
				freeBigInt(a);
			}
			if(consumeB){
				freeBigInt(b);
			}
			return ret;
		}
		if(consumeA){
			uint32_t* tmpP=realloc(a->data,(a->size+b->size)*sizeof(uint32_t));
			if(tmpP){
				a->data=tmpP;
				tmp=a;
			}else{
				freeBigInt(a);
				if(consumeB){
					freeBigInt(b);
				}
				return NULL;
			}
		}else{
			if(consumeB){
				uint32_t* tmpP=realloc(b->data,(a->size+b->size)*sizeof(uint32_t));
				if(tmpP){
					b->data=tmpP;
					tmp=b;
					consumeB=false;//b should not be deleted
				}else{
					freeBigInt(b);
					return NULL;
				}
			}else{
				tmp=createBigIntSize(a->size+b->size);
				if(!tmp){
					return NULL;
				}
			}
		}
		internal_unsaveMult(tmp,a,b);
		if(consumeB){
			freeBigInt(b);
			b=NULL;
		}
		return internal_standardizeBigInt(tmp);
	}
	return NULL;
}

BigInt* squareBigInt(BigInt* a,bool consume){
	//TODO special handling for squaring
	return multBigInt(a,consume,a,consume);
}

static uint32_t internal_divBigIntInt(BigInt* a,uint32_t div){
	uint64_t tmp=0;
	if(div!=1){
		for(size_t i=a->size-1;i!=SIZE_MAX;i--){
			tmp <<= INT_BITS;
			tmp+=a->data[i];
			a->data[i]=tmp/div;
			tmp%=div;
		}
	}
	return tmp&UINT32_MAX;
}

//divides a by b using the Newton-algorithm
//a is consumed by this operation
static BigInt* internal_divideNewton(BigInt* a,BigInt* b){
	size_t k=a->size;
	BigInt* res=shiftBigInt(
			createBigIntInt(0x1000000000LL / b->data[b->size - 1]), true,
			INT_BITS * (a->size - b->size));
	//uses Newton-Method on
	//X_(i+1)=X_i+(X_i(2^k-D*X_i))/2^k
	//to calculate an estimate for 2^k/D
	BigInt* del;
	uint32_t sgnBuff;
	while(1){
		del=multBigInt(b,false,res,false);//D*X_i
		//addLater? more effective creation method for 2^(32k)
		del = subtBigInt(shiftBigInt(createBigIntInt(1), true, (INT_BITS * k) + 4),
				true, del, true);		//2^32k-D*X_i
		del=multBigInt(res,false,del,true);//X_i*(2^(32k)-D*X_i)
		sgnBuff=del->sgn;
		del->sgn=0;//unsigned shift
		del = shiftBigInt(del, true, -((INT_BITS * k) + 4));//X_i*(2^(32k)-D*X_i)/2^(32k)
		del->sgn=sgnBuff;
		if(bigIntSgn(del)==0){
			if(del){
				freeBigInt(del);
				break;
			}else{
				freeBigInt(res);
				freeBigInt(a);
				return NULL;
			}
		}
		res=addBigInt(res,true,del,true);//X_i+X_i*(2^(32k)-D*X_i)/2^(32k)
	}
	return shiftBigInt(multBigInt(a, true, res, true), true, -((INT_BITS * k) + 4));
}

/**divides a by b
  arguments marked with consume will be deleted or overwritten by the calculation*/
DivModResult divModBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB,bool storeDiv,bool storeRem){
	if(a&&b&&(storeDiv||storeRem)){
		if(a==b){//can only consume one
			consumeA&=consumeB;//only consume if both can be consumed
			consumeB=false;
		}
		int sgnA = bigIntSgn(a);
		int sgnB = bigIntSgn(b);
		if(sgnB==0){
			//division by 0
			if(consumeA){
				freeBigInt(a);
			}
			if(consumeB){
				freeBigInt(b);
			}
			return (DivModResult){.result=NULL,.remainder=NULL};
		}//no else
		if (sgnA<0) {
			a=negateBigInt(a,consumeA);
		}else if(!consumeA){
			a=cloneBigInt(a);//replace a with copy
		}//no else
		if(a){
			consumeA=true;//a is copy
		}else{
			//cloning a failed
			if(consumeB){
				freeBigInt(b);
			}
			return (DivModResult){.result=NULL,.remainder=NULL};
		}//no else
		if (sgnB<0) {
			b=negateBigInt(b,consumeB);
			consumeB=true;
			if(b==NULL){
				//negating b failed
				freeBigInt(a);
				return (DivModResult){.result=NULL,.remainder=NULL};
			}
		}
		BigInt* q=NULL;
		if(b->size==1){
			uint32_t rem=internal_divBigIntInt(a,b->data[0]);
			if(storeDiv){
				q=internal_standardizeBigInt(a);//store result in q
			}else{
				free(a);
				a=NULL;
			}
			if(storeRem){
				if(!consumeB){
					b=createBigIntInt(rem);
				}else{
					b->data[0]=rem;
					consumeB=false;
				}
				a=internal_standardizeBigInt(b);//store remainder in a
			}else{
				a=NULL;
			}
		}else if(cmpBigInt(a,b)>=0){
			size_t lnzA=a->size-1,lnzB=b->size-1,delta;
			if(a->data[lnzA]==0)
				lnzA--;
			if(b->data[lnzB]==0)
				lnzB--;
			assert((a->data[lnzA]!=0)&&(b->data[lnzB]!=0));
			delta=lnzA-lnzB;
			if(true/*lnzA>20||delta>10*/){//TODO constants
				a->size=lnzA+1;
				b->size=lnzB+1;
				if(storeRem){
					q=cloneBigInt(a);
				}
				a=internal_divideNewton(a,b);
				//FIXME correct error +-1
				if(storeRem){
					BigInt* tmp=a;
					a=subtBigInt(q,true,multBigInt(a,false,b,consumeB),true);
					consumeB=false;
					q=tmp;
				}else if(storeDiv){
					q=a;
					a=NULL;
				}
			}else{
				uint64_t div,remBuffer=0,divBuffer;
				if(storeDiv){
					q=createBigIntSize(delta+2);
					if(q==NULL||q->data==NULL){
						//creating q failed
						freeBigInt(a);
						if(consumeB){
							freeBigInt(b);
						}
						return (DivModResult){.result=NULL,.remainder=NULL};
					}else{
						//initialize bits of return value to 0
						memset(q->data,0,(delta+2)*sizeof(uint32_t));
					}
				}
				while(lnzA>=lnzB){//>=0 not possible for uints
					div=a->data[lnzA];
					while(div<=b->data[lnzB]){
						if(lnzA>lnzB){
							div <<= INT_BITS;
							delta--;
							div|=a->data[lnzA-1];
						}else{
							delta=SIZE_MAX;
							break;
						}
					}
					if(delta==SIZE_MAX){
						break;
					}
					div/=(b->data[lnzB]+1);
					if(div>0){
						if(storeDiv){
							divBuffer=div;
							divBuffer+=q->data[delta];
							q->data[delta]=divBuffer&UINT32_MAX;
							divBuffer >>= INT_BITS;
							size_t i=1;
							while(divBuffer!=0){
								divBuffer+=q->data[delta+i];
								q->data[delta+i]=divBuffer&UINT32_MAX;
								divBuffer >>= INT_BITS;
								i++;
							}
						}
						divBuffer=0;
						for(size_t i=0;i<=lnzB;i++){
							remBuffer+=a->data[i+delta];
							remBuffer&=UINT32_MAX;
							divBuffer+=div*b->data[i];
							remBuffer-=divBuffer&UINT32_MAX;
							a->data[i+delta]=remBuffer&UINT32_MAX;
							remBuffer >>= INT_BITS;
							divBuffer >>= INT_BITS;
						}
						if((remBuffer!=0||divBuffer!=0)&&lnzA==delta+lnzB+1){
							remBuffer+=a->data[lnzA];
							remBuffer&=UINT32_MAX;
							remBuffer-=divBuffer&UINT32_MAX;
							a->data[lnzA]=remBuffer&UINT32_MAX;
							remBuffer >>= INT_BITS;
							divBuffer >>= INT_BITS;
						}
					}
					assert(remBuffer==0&&divBuffer==0);
					while(lnzA>0&& a->data[lnzA]==0){
						lnzA--;
					}
					delta=lnzA-lnzB;
				}
				if(lnzA==SIZE_MAX){
					a->size=0;
					free(a->data);
					a->data=NULL;
				}else{
					//adjust size for compare
					a->size=lnzA+1;
					if(cmpBigInt(a,b)>=0){
						//TODO? extract common code to function?
						for(size_t i=0;i<=lnzB;i++){
							remBuffer+=a->data[i];
							remBuffer&=UINT32_MAX;
							remBuffer-=b->data[i];
							a->data[i]=remBuffer&UINT32_MAX;
							remBuffer >>= INT_BITS;
						}
						assert(remBuffer==0);
						while(lnzA>0&& a->data[lnzA]==0){
							lnzA--;
						}
						if(storeDiv){
							q->data[0]++;
						}
					}
					//readjusts size
					a->size=lnzA+1;
					if(lnzA==0&&a->data[lnzA]==0){
						a->size=0;
						free(a->data);
						a->data=NULL;
					}else{
						//reduce size of data block
						uint32_t* tmp=realloc(a->data,a->size*sizeof(uint32_t));
						if(tmp){
							a->data=tmp;
						}else{
							//reallocation of a failed
							freeBigInt(a);
							freeBigInt(q);
							return (DivModResult){.result=NULL,.remainder=NULL};
						}
					}
					q=internal_standardizeBigInt(q);
				}
			}
		}else{
			if(storeDiv){
				q=createBigIntInt(0);
			}
		}
		if(consumeB){
			freeBigInt(b);
			b=NULL;
		}
		DivModResult ret={.result=NULL,.remainder=NULL};
		//sign handling of result a=q*b+r
		// (q*b+r)/b => q R B
		// (q*b+r)/-b => -q R r
		// (-(q*b+r))/b => -q R -r
		// (-(q*b+r))/-b => q R -r
		if(sgnA<0){//sgnA->sgnR, sgnB->sgnQ
			if(sgnB<0){
				sgnB=1;
			}else{
				sgnB=-1;
			}
		}
		if(storeDiv){
			if(storeRem){
				ret.result=sgnB<0?negateBigInt(q,true):q;
				ret.remainder=sgnA<0?negateBigInt(a,true):a;
			}else{
				ret.result=sgnB<0?negateBigInt(q,true):q;
				freeBigInt(a);
			}
		}else{
			ret.remainder=sgnA<0?negateBigInt(a,true):a;
			freeBigInt(q);
		}
		return ret;
	}
	//invalid input arguments
	if(consumeA){
		freeBigInt(a);
	}
	if(consumeB){
		freeBigInt(b);
	}
	return (DivModResult){.result=NULL,.remainder=NULL};
}
/**divides a by b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* divBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB){
	return divModBigInt(a,consumeA,b,consumeB,true,false).result;
}
/**remainder of a divided by b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* modBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB){
	return divModBigInt(a,consumeA,b,consumeB,false,true).remainder;
}

BigInt* bigIntGCD(BigInt* a,bool consumeA,BigInt* b,bool consumeB){
	if(a&&b){
		if(a==b){//can only consume one
			consumeA&=consumeB;//only consume if both can be consumed
			consumeB=false;
		}
		if(a->sgn){
			a=negateBigInt(a,consumeA);
			consumeA=true;
		}else if(a->size==0){
			if(consumeB){
				if(!consumeA&&b->size==0){
					return b;
				}
				freeBigInt(b);
			}
			return consumeA?a:createBigIntInt(0);
		}else if(!consumeA){
			a=cloneBigInt(a);
			if(!a){
				if(consumeB){
					freeBigInt(b);
				}
				return NULL;
			}
		}
		if(b->sgn){
			b=negateBigInt(b,consumeB);
			consumeB=true;
		}else if(b->size==0){
			if(consumeA){
				freeBigInt(a);
			}
			return consumeB?b:createBigIntInt(0);
		}else if(!consumeB){
			b=cloneBigInt(b);
			if(!b){
				if(consumeA){
					freeBigInt(a);
				}
				return NULL;
			}
		}
		BigInt* tmp;
		if(cmpBigInt(a,b)<0){
			tmp=a;
			a=b;
			b=tmp;
		}
		//0<b<a, consumeA=consumeB=true
		while(bigIntSgn(b)!=0){
			tmp=modBigInt(a,true,b,false);
			a=b;
			b=tmp;
		}
		if(!b){
			freeBigInt(a);
			return NULL;
		}
		freeBigInt(b);
		return a;
	}
	return NULL;
}

/**returns a to the power of b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* powBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB){
	if(a&&b){
		if(a==b){//can only consume one
			consumeA&=consumeB;//only consume if both can be consumed
			consumeB=false;
		}
		uint64_t pow;
		if((b->sgn)||(b->size>2)){
			if(consumeA){
				freeBigInt(a);
			}//no else
			if(consumeB){
				freeBigInt(b);
			}
			return NULL;
		}else{
			if(b->size==0){
				if(consumeA){
					freeBigInt(a);
				}//no else
				if(consumeB){
					freeBigInt(b);
				}
				return createBigIntInt(1);
			}else{
				pow=b->data[0];
				if(b->size==2){
					pow |= ((uint64_t) b->data[1]) << INT_BITS;
				}
			}
			if(consumeB){
				freeBigInt(b);
				b=NULL;
			}
		}
		BigInt* res=createBigIntInt(1);
		if(!consumeA){
			a=cloneBigInt(a);
		}
		if(res){
			while(pow!=0){
				if(pow&1){
					res=multBigInt(res,true,a,false);
					if(!res){
						freeBigInt(a);
						return NULL;
					}
				}
				pow>>=1;
				a=squareBigInt(a,true);
				if(!a){
					freeBigInt(res);
					return NULL;
				}
			}
		}
		return res;
	}
	return NULL;
}
