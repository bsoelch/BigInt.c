/*
 * BigInt.h
 *
 *  Created on: 28.09.2021
 *  Author: bsoelch
 */

#ifndef BIGINT_H_
#define BIGINT_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


typedef struct BigIntStruct BigInt;

//constants for zero and one
//!!! do not mark this values as consumable!!!
BigInt* BIG_INT_ZERO;
BigInt* BIG_INT_ONE;


//creates a BigInt from a 64bit int value
BigInt* createBigIntInt(int64_t value);
/*creates a BigInt from a null-terminated string
 *base should be between 2 and 62*///XXX describe case handling
BigInt* createBigIntCStr(const char* str,int base);
/*creates a BigInt from a string
 *base should be between 2 and 62*///XXX describe case handling
BigInt* createBigIntStr(const char* stringValue,size_t stringLen,int base);
/**creates a BigInt from the given array of bytes*/
BigInt* createBigIntBytes(void* bytes,size_t numBytes);
/**creates a clone of the given BigInteger*/
BigInt* cloneBigInt(BigInt* source);
/**frees the given BigInteger*/
void freeBigInt(BigInt* toFree);

int printBigInt(BigInt *toPrint, _Bool consume, FILE *target, int base);
void printBigIntHex(BigInt* number,bool consume);


/**returns the sign of a (a<0?-1:a>0?1:0)*/
int bigIntSgn(BigInt* a);

//TODO bitLength

/**Compares two BigIntegers a and b
 * returns negative value if a<b; 0 if a==b and positive value if a>b*/
int cmpBigInt(BigInt* a,BigInt* b);
/**Compares a BigInt a and an Integer b
 * returns negative value if a<b; 0 if a==b and positive value if a>b*/
int cmpBigIntInt(BigInt* a,int64_t b);

/**flips all bits in a,
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* notBigInt(BigInt* a,bool consumeA);
/*logical and of a and b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* andBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB);
/*logical or of a and b
  arguments marked with consume will be deleted or overwritten by the calculation**/
BigInt* orBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB);
/*logical exclusive or of a and b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* xorBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB);
/**returns a cop of a that is shifted by the given amount,
 * if amount is positive a is shifted to the left otherwise to the right
  arguments marked with consume will be deleted or overwritten by the calculation */
BigInt* shiftBigInt(BigInt* a,bool consumeA,int64_t amount);

/**negates toNegate
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* negateBigInt(BigInt* toNegate,bool consumeArg);

/**adds a to b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* addBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB);

/**subtracts b from a
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* subtBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB);

/**multiples a by b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* multBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB);
/**squares a
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* squareBigInt(BigInt* a,bool consume);
/**result and remainder of a divided by b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt** divModBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB,bool storeDiv,bool storeRem);

//helper for the parts of divMod
/**divides a by b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* divBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB);
/**remainder of a divided by b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* modBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB);
/**greatest common divisor of a and b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* bigIntGCD(BigInt* a,bool consumeA,BigInt* b,bool consumeB);
/**returns a to the power of b
  arguments marked with consume will be deleted or overwritten by the calculation*/
BigInt* powBigInt(BigInt* a,bool consumeA,BigInt* b,bool consumeB);

#endif /* BIGINT_H_ */
