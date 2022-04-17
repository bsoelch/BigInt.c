# BigInt
implementation of arbitrary size Integers

## Calculator

The file Calculator.c contains a simple console calculator
 based on this big-integer libary.

The program evaluates arithmetic expressions consisting of brackets ( `(` `)` ) numbers (decimal) and operators (explained below). 
The program prints the expression tree (with numbers in hexadecimal representation) and the result of the expression (in decimal).
as well as the syntax tree of the evaluated expression.

The caluclator automatically terminates after 16 operations,
the program can be terminated earlier by typing `exit` in the input field.

### Binary operators

The supported arithmetic operations are

- `+` addition
- `-` subtraction
- `*` multipliction
- `%` modulo
- `^` exponentiation

- `&` bitwise logical AND
- `|` bitwise logical OR
- `#` bitwise logical XOR

The arithmetic operatos have the usual precedence 
( `^` before `*` `/` `%` before `+` `-` ), logical operators 
have lower precedence than all arithmetic operations.
Operations with the same precedence are evaluated from left to rigth, 
with the exception that exponentiation is evaluated from rigth to left.

Example expressions:

```
> 1+2*3^4
Expression Tree: (1+(2*(3^4)))
Result: 163

>  2/3
Expression Tree: (2/3)
Result: 0

> 5^100 % 10
Expression Tree: ((5^64)%a)
Result: 5

> 1+2-3+4
Expression Tree: (((1+2)-3)+4)
Result: 4

> 2^2^3
Expression Tree: (2^(2^3))
Result: 256

> 1#3&2|4
Expression Tree: (((1#3)&2)|4)
Result: 6
```


### Unary operators

- `?` sign of the argument (`-1` `0` or `1`)
- `-` negation
- `~` bitwise logical complement

Unary operations are evaluated before any binary operation.
If a number has multiple unary operators the operators will be executed
from rigth to left,a number can have at most 2 unary operators.

Example expressions:

```
> -1--2
Expression Tree: ((-1)-(-2))
Result: 1

> ?-10
Expression Tree: (?(-a))
Result: -1

> ~1
Expression Tree: (~1)
Result: -2

```

