// Minimal diagnostic — isolate which object feature fails

// 1. Object literal + property access
var obj = { x: 10, y: 20 };
var t1 = (obj.x === 10 && obj.y === 20) ? 1 : 0;

// 2. Property assignment
obj.z = 30;
var t2 = (obj.z === 30) ? 1 : 0;

// 3. Nested object
var nested = { inner: { value: 42 } };
var t3 = (nested.inner.value === 42) ? 1 : 0;

// 4. Bracket access
var key = 'x';
var t4 = (obj[key] === 10) ? 1 : 0;

// Encode results
__result = t1 * 1 + t2 * 2 + t3 * 4 + t4 * 8;
// Full pass = 15
