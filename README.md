# Inn Language

Basic procedural programming language

## Syntax

Builtin types are `int`, `float` and `string`.

Arrays are declared by prefixing with brackets and element count `[3]int`, and they're constructed with brackets `[1,4,7]`. Arrays are zero-indexed, elements can be accesed by indexing with brackets in suffix notation `arr[0]`.

Traditional arithmetic, comparison and logical operators are supported.

```py
a + b
a - b
a * b
a / b
a ^ b # Exponentiation
a and b
a or b
not a
a == b
a >= b
a > b
a <= b
a < b
```

Variables can be declared using the `var` keyword. Types are written after the name.

```go
var age int
var name string = "Pancake"
```

Assignments can be done with `=`

```go
age = 17
```

Pointers can be declared similar to arrays `[]int`. They are also dereferenced similar to array indexing `ptr[]`. You can take a pointer to a variable by prefixing with `&`.

```go
var foo int = 3
var bar []int = &foo
bar[] = 7
```

Functions are declared with the `func` keyword. Blocks begin with the `do` keyword and end with the `end` keyword. Functions can return with `return` keyword.

```rb
func add(a int, b int) int do
    return a + b
end
```

`if` can be used to declare branches in code. Multiple exclusive branches can be chained with `else if`, and a fall-through case can be written with `else`.

```rb
if x < 5 do
    printf("x < 5\n")
else if x < 10 do
    printf("5 <= x < 10\n")
else do
    printf("x >= 10\n")
end
```

`while` can be used to create loops. You can break out of a loop early with `break`.

```rb
var x int = 0
while x < 10 do
    if x == 7 do
        break
    end
    printf("%d\n", x)
    x = x + 1
end
```
