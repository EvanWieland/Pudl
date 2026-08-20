# The Pudl Language

A small, statically-typed, C-like language. This is the authoritative
reference for what the language currently supports -- it reflects the
grammar `src/Parser/Parser.cpp` actually implements, not aspirations.

## Hello, Pudl

```pudl
# Comments start with # and run to end of line.
func mast : int {
  print 42
  return 0
}
```

Every program needs exactly one `mast` function -- it's the entry point
(Pudl's `main`), and its return value becomes the process's exit code.
`pudl file.pudl` compiles and immediately runs it; see `DEVELOPING.md` /
`pudl --help` for compiling to an object file or a standalone executable
instead.

## Types

Three types exist: `int`, `float`, `bool` (literals `True`/`False`).
There is no `void` yet -- every function, including `mast`, must declare
and return one of these three. There are no strings, arrays, or structs;
see "Not yet supported" below.

Numeric literals: `42` (int), `3.14` (float). An integer literal too
large to fit an `int` is a parse error, not a silent overflow.

## Variables

```pudl
int x = 5
float y = 1.5
bool flag = True
```

Declaration and assignment share one syntax-level form
(`<type> <name> = <expr>` to declare, `<name> = <expr>` to reassign an
existing variable or parameter); both require an initializer -- there is
no uninitialized declaration. Assigning to an undeclared name, or
redeclaring an already-declared name in the same scope, is a parse
error.

## Operators

| Category   | Operators                | Notes |
|------------|--------------------------|-------|
| Arithmetic | `+` `-` `*` `/`          | `int`/`float` only; mixing promotes to `float` |
| Comparison | `==` `!=` `<` `>` `<=` `>=` | produce `bool` |
| Logical    | `&&` `\|\|` `!`            | `bool` only; `&&`/`\|\|` short-circuit |
| Unary      | `+x` `-x` `!x`           | `+`/`-` on numbers, `!` on `bool` |

`bool` and numeric types don't implicitly convert in arithmetic or
comparisons -- `1 + True` is a parse error, not a truthy-value coercion.

Precedence, loosest to tightest (mirrors the grammar below):
`||` < `&&` < `==`/`!=` < `<`/`>`/`<=`/`>=` < `+`/`-` < `*`/`/` < unary
`+`/`-`/`!`. Parenthesize to override, same as C.

## Functions

```pudl
func fact( int n ) : int {
  if n <= 1 {
    return 1
  }
  return n * fact( n - 1 )
}

func mast : int {
  print fact( 5 )   # 120
  return 0
}
```

Parameters are typed and, unlike in many C-like languages, *can* be
reassigned inside the function body -- `a = a + 1` inside a function
taking `int a` rebinds the parameter, it doesn't shadow it. A function
can call itself (direct recursion, as `fact` does above) but must still
be fully defined before being called from *another* function's body --
there's no forward declaration, so mutual recursion (A calls B, and B
calls A back) doesn't work: A can't call B until B is already defined,
so if A is written first, its own call to B fails even though B itself
can freely call A (already-defined by the time B is parsed). Argument
count and type are checked at the call site.

## Statements

```pudl
if x > 0 {
  print 1
} else if x < 0 {
  print -1
} else {
  print 0
}

while x > 0 {
  x = x - 1
}

do {
  x = x - 1
} while x > 0

print x        # print <expr>
return x       # return <expr> -- required, even in mast
```

Blocks (`{ ... }`) introduce their own variable scope: a variable
declared inside an `if`/`while`/`do` body doesn't leak into the
enclosing scope once the block ends.

Every condition (`if`, `while`, `do`-`while`) is required to be `bool`;
a numeric condition is a parse error, consistent with the "no implicit
bool/numeric conversion" rule above.

## Not yet supported

Deliberately out of scope for the current language (tracked as future
work, not oversights): `void` functions, `for` loops, `break`/`continue`,
strings, arrays, structs, modules/imports, and multiple return values.
`read`/`^` (xor) tokens that once existed in the lexer were removed
outright rather than ever being wired up.

## Grammar reference

Precedence-climbing expression grammar, loosest-binding first; `:=`
reads as "is defined as", `?` means optional, `*` means zero-or-more.

```
function-definition := func <name> [( <function-args> )]? : <type> <statement>
function-args       := [<type> <variable>,]* [<type> <variable>]?

statement   := <block> | <if-stmt> | <while-stmt> | <do-while-stmt>
             | <declaration> | <assignment> | <funcall> | <print> | <return>
block       := { <statement>* }
if-stmt     := If <expression:bool> <statement> (Else <statement>)?
while-stmt  := While <expression:bool> <statement>
do-while    := Do <statement> While <expression:bool>
declaration := <type> <variable> = <expression>
assignment  := <variable> = <expression>
print       := Print <expression>
return      := Return <expression>

expression   := <lor>
lor          := <land> (`||` <lor>)?
land         := <cmpeq> (`&&` <land>)?
cmpeq        := <cmp> (`==`|`!=` <cmpeq>)?
cmp          := <additive> (`<`|`>`|`<=`|`>=` <cmp>)?
additive     := <multiplicative> (`+`|`-` <additive>)?
multiplicative := <unary> (`*`|`/` <multiplicative>)?
unary        := (`+`|`!`)? <factor>
factor       := <constant> | <funcall> | <variable> | ( <expression> )
constant     := <integer> | <float> | <boolean>
funcall      := <name> ( <expression>,* )
```
