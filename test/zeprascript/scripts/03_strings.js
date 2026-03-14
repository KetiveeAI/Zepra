// String operations
var hello = 'hello';
var world = 'world';
var greeting = hello + ' ' + world;

// String + number concatenation
var strNum = 'value: ' + 42;

// String length (via property)
var len = greeting.length;

// Comparison
var same = ('abc' === 'abc');
var diff = ('abc' === 'def');

// Empty string
var empty = '';
var notEmpty = 'x';

// String truthiness
var emptyIsFalsy = !empty;      // true
var nonEmptyIsTruthy = !notEmpty; // false (double negation: notEmpty is truthy, !truthy = false)

__result = (greeting === 'hello world' && strNum === 'value: 42' && same === true && diff === false && emptyIsFalsy === true && nonEmptyIsTruthy === false) ? 1 : 0;
