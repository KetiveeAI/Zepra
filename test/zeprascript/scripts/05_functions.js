// Minimal diagnostic — isolate which function feature fails
// Test each feature separately and store pass/fail flags

// 1. Basic function declaration + call
function add(a, b) { return a + b; }
var t1 = (add(3, 4) === 7) ? 1 : 0;

// 2. Function expression
var mul = function(a, b) { return a * b; };
var t2 = (mul(6, 7) === 42) ? 1 : 0;

// 3. Recursion
function factorial(n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
var t3 = (factorial(5) === 120) ? 1 : 0;

// 4. Closure
function makeCounter() {
    var count = 0;
    return function() {
        count = count + 1;
        return count;
    };
}
var counter = makeCounter();
var c1 = counter();
var c2 = counter();
var t4 = (c1 === 1 && c2 === 2) ? 1 : 0;

// 5. Nested closure
function outer(x) {
    return function(y) { return x + y; };
}
var t5 = (outer(5)(10) === 15) ? 1 : 0;

// 6. Default return
function noReturn() { var x = 1; }
var t6 = (noReturn() === undefined) ? 1 : 0;

// Encode which tests pass as a bitmask
__result = t1 * 1 + t2 * 2 + t3 * 4 + t4 * 8 + t5 * 16 + t6 * 32;
// Full pass = 63 (0b111111)
