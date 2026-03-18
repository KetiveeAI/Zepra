// Functions

// Declaration
function add(a, b) {
    return a + b;
}
var addResult = add(3, 4); // 7

// Expression
var multiply = function(a, b) {
    return a * b;
};
var mulResult = multiply(6, 7); // 42

// Recursion — factorial
function factorial(n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
var fact5 = factorial(5); // 120

// Closure
function makeCounter() {
    var count = 0;
    return function() {
        count = count + 1;
        return count;
    };
}
var counter = makeCounter();
var c1 = counter(); // 1
var c2 = counter(); // 2
var c3 = counter(); // 3

// Nested closure
function outer(x) {
    return function(y) {
        return x + y;
    };
}
var addFive = outer(5);
var closureResult = addFive(10); // 15

// Default return (undefined)
function noReturn() {
    var x = 1;
}
var noReturnResult = noReturn();

// Multiple returns
function abs(n) {
    if (n < 0) return -n;
    return n;
}
var absResult = abs(-42); // 42

__result = (addResult === 7 && mulResult === 42 && fact5 === 120 && c1 === 1 && c2 === 2 && c3 === 3 && closureResult === 15 && noReturnResult === undefined && absResult === 42) ? 1 : 0;
