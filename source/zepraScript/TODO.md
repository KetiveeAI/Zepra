# ZebraScript v1.1 - Execution Engine Milestone

## Quick Start
```bash
cmake -B build && cmake --build build -j$(nproc)
./build/bin/zepra-repl
```

## Dependencies: NONE (standard C++ only)

---

# VERIFIED WORKING ✓

## Control Flow
```javascript
// if/else ✓
let x = 5
if (x > 3) { x = x * 2 }  // x = 10

// while loop ✓  
let sum = 0; let i = 1
while (i <= 5) { sum = sum + i; i = i + 1 }  // sum = 15

// for loop ✓
let sum = 0
for (let i = 1; i <= 10; i = i + 1) { sum = sum + i }  // sum = 55
```

## User-Defined Functions (no params) ✓
```javascript
function test() { return 42 }
test()  // → 42
```

## Math Builtins ✓
```javascript
Math.sqrt(144)  // → 12
Math.pow(2, 10) // → 1024
Math.max(1,5,3,9,2) // → 9
Math.PI // → 3.14159
```

## Object Literals ✓
```javascript
let o = { a: 1, b: 2 }
o.a + o.b  // → 3
```

## Array Literals ✓
```javascript
let arr = [1, 2, 3]
arr[0] + arr[2]  // → 4
```

## Functions with Parameters
```javascript
function add(a, b) { return a + b }
add(3, 4)  // -> 7

function multiply(x, y) { return x * y }
multiply(5, 6)  // -> 30

// Recursive functions work too!
function fib(n) { if (n <= 1) { return n } return fib(n-1) + fib(n-2) }
fib(10)  // -> 55
```

## Console API
```javascript
console.log("Hello from Zepra!")  // prints: Hello from Zepra!
console.log(1 + 2 + 3)  // prints: 6
```

## String Literals
```javascript
"Hello, World!"  // -> Hello, World!
```

## Try/Catch/Throw
```javascript
try {
  throw "error"
} catch(e) {
  console.log(e)  // prints: error
}
```

## Array Methods
```javascript
let arr = [1, 2, 3]
arr.push(99)  // -> 4 (new length)
arr.pop()     // -> 99 (popped value)
arr[0] + arr[1]  // -> 3
```

## JSON
```javascript
let obj = JSON.parse('{"name":"Zepra","v":1.2}')
obj.name     // -> Zepra
JSON.stringify({a:1, b:2})  // -> {"b":2,"a":1}
```

## String Methods
```javascript
"hello".toUpperCase()  // -> HELLO
"hello".slice(1, 3)    // -> el
"  hi  ".trim()        // -> hi
"test".indexOf("e")    // -> 1
"a b c".split(" ")     // -> [a, b, c]
```

## Module Syntax
```javascript
export function add(a, b) { return a + b }
add(5, 3)  // -> 8

import { foo } from './module.js'  // parses (file loading stub)
```

---

# TODO

## v1.2 COMPLETE
- [x] Array.prototype methods ✓
- [x] JSON.parse / JSON.stringify ✓
- [x] Module system syntax ✓

## v1.3 COMPLETE
- [x] Full module loader (file system) ✓
  - ModuleLoader class with caching
  - Path resolution (relative/absolute)
  - File loading and compilation

## Sprint 5 COMPLETE
- [x] Performance optimizations ✓
  - Peephole optimizer (0 bytes overhead)
  - Inline caching (16KB max)
  - JIT profiler (16KB max)
  - **Total: ~32KB worst case**

## v1.3 FUTURE
- [ ] Garbage Collection (GenGC)
- [ ] JIT Compilation
- [ ] Async/await
- [ ] DOM APIs

---

# File Locations

| Component | Path |
|-----------|------|
| VM | src/runtime/vm.cpp |
| Bytecode Gen | src/bytecode/bytecode_generator.cpp |
| Parser | src/frontend/parser.cpp |
| AST | include/zeprascript/frontend/ast.hpp |
