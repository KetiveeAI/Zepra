// Arithmetic operators
var a = 10 + 5;     // 15
var b = 10 - 3;     // 7
var c = 6 * 7;      // 42
var d = 84 / 2;     // 42
var e = 17 % 5;     // 2
var f = 2 ** 10;    // 1024

// Unary
var g = -42;
var h = -(g);       // 42

// Increment patterns
var i = 0;
i = i + 1;          // 1
i = i + 1;          // 2
i = i + 1;          // 3

// Order of operations
var j = 2 + 3 * 4;  // 14
var k = (2 + 3) * 4; // 20

// Division edge cases
var divByZero = 1 / 0;  // Infinity
var negDivByZero = -1 / 0; // -Infinity
var zeroByZero = 0 / 0; // NaN

// Modulo edge cases
var modResult = -7 % 3; // -1

__result = (a === 15 && b === 7 && c === 42 && d === 42 && e === 2 && f === 1024 && h === 42 && i === 3 && j === 14 && k === 20) ? 1 : 0;
