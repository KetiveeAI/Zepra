// Test262 Extended Compliance Tests for ZepraScript
// ECMAScript 2024 Specification Compliance

// ============================================================
// SCOPE AND VARIABLES (Critical)
// ============================================================

// --- TDZ (Temporal Dead Zone) ---
var tdzPassed = false;
try {
  // Access before declaration should throw ReferenceError
  console.log("TDZ_LET: FAIL (no error thrown)");
} catch (e) {
  tdzPassed = true;
  console.log("TDZ_LET: PASS");
}
let tdzVar = 1;

// --- Block Scoping ---
var outerX = 1;
{
  let outerX = 2;
  var blockScopeTest = outerX;
}
console.log(
  "BLOCK_SCOPE: " + (outerX === 1 && blockScopeTest === 2 ? "PASS" : "FAIL"),
);

// --- Const Immutability ---
const constVar = 42;
var constTest = false;
try {
  // Assignment to const should throw
  constTest = false;
  console.log("CONST_IMMUTABLE: FAIL (no error thrown)");
} catch (e) {
  constTest = true;
  console.log("CONST_IMMUTABLE: PASS");
}

// ============================================================
// CLOSURES (Critical)
// ============================================================

// --- Basic Closure ---
function makeCounter() {
  var count = 0;
  return function () {
    return ++count;
  };
}
var counter = makeCounter();
console.log(
  "CLOSURE_BASIC: " + (counter() === 1 && counter() === 2 ? "PASS" : "FAIL"),
);

// --- Closure in Loop (let vs var) ---
var closureResults = [];
for (let i = 0; i < 3; i++) {
  closureResults.push(function () {
    return i;
  });
}
console.log(
  "CLOSURE_LOOP_LET: " +
    (closureResults[0]() === 0 &&
    closureResults[1]() === 1 &&
    closureResults[2]() === 2
      ? "PASS"
      : "FAIL"),
);

// ============================================================
// THIS BINDING (Critical)
// ============================================================

// --- Method this ---
var thisObj = {
  value: 100,
  getValue: function () {
    return this.value;
  },
};
console.log("THIS_METHOD: " + (thisObj.getValue() === 100 ? "PASS" : "FAIL"));

// --- Arrow function preserves outer this ---
var arrowThis = {
  value: 200,
  getArrow: function () {
    var arrow = () => this.value;
    return arrow();
  },
};
console.log("THIS_ARROW: " + (arrowThis.getArrow() === 200 ? "PASS" : "FAIL"));

// --- Explicit this binding with call ---
function getThisValue() {
  return this.val;
}
var explicitThis = { val: 300 };
console.log(
  "THIS_CALL: " + (getThisValue.call(explicitThis) === 300 ? "PASS" : "FAIL"),
);

// ============================================================
// COERCIONS (Edge Cases)
// ============================================================

// --- ToPrimitive ---
console.log("TO_NUMBER_NULL: " + (Number(null) === 0 ? "PASS" : "FAIL"));
console.log(
  "TO_NUMBER_UNDEFINED: " + (isNaN(Number(undefined)) ? "PASS" : "FAIL"),
);
console.log("TO_NUMBER_TRUE: " + (Number(true) === 1 ? "PASS" : "FAIL"));
console.log("TO_NUMBER_FALSE: " + (Number(false) === 0 ? "PASS" : "FAIL"));

// --- ToString ---
console.log("TO_STRING_NULL: " + (String(null) === "null" ? "PASS" : "FAIL"));
console.log(
  "TO_STRING_UNDEFINED: " +
    (String(undefined) === "undefined" ? "PASS" : "FAIL"),
);

// --- ToBoolean ---
console.log("FALSY_0: " + (!0 ? "PASS" : "FAIL"));
console.log("FALSY_EMPTY: " + (!"" ? "PASS" : "FAIL"));
console.log("FALSY_NAN: " + (!NaN ? "PASS" : "FAIL"));
console.log("FALSY_NULL: " + (!null ? "PASS" : "FAIL"));
console.log("FALSY_UNDEFINED: " + (!undefined ? "PASS" : "FAIL"));

// ============================================================
// EQUALITY (Critical)
// ============================================================

// --- Abstract Equality ---
console.log("EQ_NULL_UNDEF: " + (null == undefined ? "PASS" : "FAIL"));
console.log("EQ_NUM_STR: " + ("42" == 42 ? "PASS" : "FAIL"));
console.log("EQ_BOOL_NUM: " + (true == 1 ? "PASS" : "FAIL"));

// --- Strict Equality ---
console.log("SEQ_TYPE_MISMATCH: " + ("42" !== 42 ? "PASS" : "FAIL"));
console.log("SEQ_NAN: " + (NaN !== NaN ? "PASS" : "FAIL"));

// --- Object Identity ---
var objA = {};
var objB = {};
var objC = objA;
console.log(
  "SEQ_OBJ_IDENTITY: " + (objA === objC && objA !== objB ? "PASS" : "FAIL"),
);

// ============================================================
// OPERATORS
// ============================================================

// --- Addition with Coercion ---
console.log("ADD_STRING_NUM: " + ("3" + 4 === "34" ? "PASS" : "FAIL"));
console.log("ADD_NUM_STRING: " + (3 + "4" === "34" ? "PASS" : "FAIL"));

// --- Bitwise ---
console.log("BITWISE_OR: " + ((5 | 3) === 7 ? "PASS" : "FAIL"));
console.log("BITWISE_AND: " + ((5 & 3) === 1 ? "PASS" : "FAIL"));
console.log("BITWISE_XOR: " + ((5 ^ 3) === 6 ? "PASS" : "FAIL"));
console.log("BITWISE_NOT: " + (~5 === -6 ? "PASS" : "FAIL"));
console.log("LEFT_SHIFT: " + (1 << 4 === 16 ? "PASS" : "FAIL"));
console.log("RIGHT_SHIFT: " + (16 >> 2 === 4 ? "PASS" : "FAIL"));
console.log(
  "UNSIGNED_RIGHT_SHIFT: " + (-1 >>> 0 === 4294967295 ? "PASS" : "FAIL"),
);

// ============================================================
// TYPEOF
// ============================================================

console.log(
  "TYPEOF_UNDEFINED: " + (typeof undefined === "undefined" ? "PASS" : "FAIL"),
);
console.log("TYPEOF_NULL: " + (typeof null === "object" ? "PASS" : "FAIL")); // Historical bug
console.log("TYPEOF_BOOL: " + (typeof true === "boolean" ? "PASS" : "FAIL"));
console.log("TYPEOF_NUM: " + (typeof 42 === "number" ? "PASS" : "FAIL"));
console.log("TYPEOF_STR: " + (typeof "hello" === "string" ? "PASS" : "FAIL"));
console.log(
  "TYPEOF_FUNC: " + (typeof function () {} === "function" ? "PASS" : "FAIL"),
);
console.log("TYPEOF_OBJ: " + (typeof {} === "object" ? "PASS" : "FAIL"));

// ============================================================
// INSTANCEOF
// ============================================================

function Animal(name) {
  this.name = name;
}
var dog = new Animal("Rex");
console.log("INSTANCEOF_BASIC: " + (dog instanceof Animal ? "PASS" : "FAIL"));
console.log("INSTANCEOF_ARRAY: " + ([] instanceof Array ? "PASS" : "FAIL"));
console.log("INSTANCEOF_OBJECT: " + ({} instanceof Object ? "PASS" : "FAIL"));

// ============================================================
// ERROR HANDLING
// ============================================================

// --- Finally always runs ---
var finallyRan = false;
try {
  throw "test";
} catch (e) {
  // caught
} finally {
  finallyRan = true;
}
console.log("FINALLY_RUNS: " + (finallyRan ? "PASS" : "FAIL"));

// --- Nested try/catch ---
var nestedResult = "";
try {
  try {
    throw "inner";
  } catch (e) {
    nestedResult += "caught-inner;";
    throw "outer";
  }
} catch (e) {
  nestedResult += "caught-outer";
}
console.log(
  "NESTED_CATCH: " +
    (nestedResult === "caught-inner;caught-outer" ? "PASS" : "FAIL"),
);

// ============================================================
// SUMMARY
// ============================================================

console.log("--- TEST262 EXTENDED COMPLIANCE COMPLETE ---");
