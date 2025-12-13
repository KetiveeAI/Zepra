// Test262 Compliance Tests for ZepraScript
// Based on ECMAScript test262 harness

// ============================================================
// LITERALS
// ============================================================

// Numeric literals
var num1 = 42;
var num2 = 3.14;
var num3 = 0;
console.log("LITERALS: " + (num1 === 42 && num2 > 3 && num3 === 0 ? "PASS" : "FAIL"));

// String literals
var str1 = "hello";
var str2 = 'world';
console.log("STRINGS: " + (str1 === "hello" && str2 === "world" ? "PASS" : "FAIL"));

// Boolean literals
var t = true;
var f = false;
console.log("BOOLEANS: " + (t === true && f === false ? "PASS" : "FAIL"));

// ============================================================
// OPERATORS
// ============================================================

// Arithmetic
console.log("ARITHMETIC: " + (5 + 3 === 8 && 10 - 4 === 6 && 6 * 7 === 42 && 20 / 4 === 5 ? "PASS" : "FAIL"));

// Comparison
console.log("COMPARISON: " + (5 < 10 && 10 > 5 && 5 <= 5 && 5 >= 5 && 5 === 5 && 5 !== 6 ? "PASS" : "FAIL"));

// Logical
console.log("LOGICAL: " + (true && true && !(false) && (true || false) ? "PASS" : "FAIL"));

// ============================================================
// CONTROL FLOW
// ============================================================

// If statement
var ifResult = 0;
if (true) ifResult = 1;
console.log("IF: " + (ifResult === 1 ? "PASS" : "FAIL"));

// If-else
var elseResult = 0;
if (false) elseResult = 1; else elseResult = 2;
console.log("IF-ELSE: " + (elseResult === 2 ? "PASS" : "FAIL"));

// While loop
var whileSum = 0;
var i = 0;
while (i < 5) { whileSum = whileSum + i; i = i + 1; }
console.log("WHILE: " + (whileSum === 10 ? "PASS" : "FAIL"));

// For loop
var forSum = 0;
for (var j = 0; j < 5; j = j + 1) { forSum = forSum + j; }
console.log("FOR: " + (forSum === 10 ? "PASS" : "FAIL"));

// ============================================================
// FUNCTIONS
// ============================================================

function add(a, b) { return a + b; }
console.log("FUNCTION: " + (add(3, 4) === 7 ? "PASS" : "FAIL"));

function factorial(n) { if (n <= 1) return 1; return n * factorial(n - 1); }
console.log("RECURSION: " + (factorial(5) === 120 ? "PASS" : "FAIL"));

// ============================================================
// OBJECTS
// ============================================================

var obj = { x: 10, y: 20 };
console.log("OBJECT: " + (obj.x === 10 && obj.y === 20 ? "PASS" : "FAIL"));

// ============================================================
// ARRAYS
// ============================================================

var arr = [1, 2, 3];
console.log("ARRAY: " + (arr[0] === 1 && arr.length === 3 ? "PASS" : "FAIL"));

arr.push(4);
console.log("ARRAY_PUSH: " + (arr.length === 4 && arr[3] === 4 ? "PASS" : "FAIL"));

// ============================================================
// STRING METHODS
// ============================================================

console.log("STRING_UPPER: " + ("hello".toUpperCase() === "HELLO" ? "PASS" : "FAIL"));
console.log("STRING_LOWER: " + ("HELLO".toLowerCase() === "hello" ? "PASS" : "FAIL"));

// ============================================================
// TRY/CATCH
// ============================================================

var caught = false;
try { throw "error"; } catch (e) { caught = true; }
console.log("TRY_CATCH: " + (caught === true ? "PASS" : "FAIL"));

// ============================================================
// SUMMARY
// ============================================================

console.log("--- TEST262 BASIC COMPLIANCE COMPLETE ---");
