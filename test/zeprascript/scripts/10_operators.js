// typeof operator
var typeNum = typeof 42;         // 'number'
var typeStr = typeof 'hello';    // 'string'
var typeBool = typeof true;      // 'boolean'
var typeUndef = typeof undefined; // 'undefined'
var typeNull = typeof null;      // 'object' (JS spec)
var typeObj = typeof {};          // 'object'
var typeFunc = typeof function() {}; // 'function'

// Logical operators
var andResult = (true && 42);     // 42
var orResult = (false || 'fallback'); // 'fallback'
var notResult = !false;           // true

// Ternary
var ternary = (10 > 5) ? 'yes' : 'no';

// Short-circuit evaluation
var shortA = false && undefined;  // false
var shortB = true || undefined;   // true

// Comparison coercion
var looseEq = (1 == true);       // true (JS loose equality)
var strictNeq = (1 !== '1');     // true

// Bitwise
var bitAnd = 0xFF & 0x0F;       // 15
var bitOr = 0xF0 | 0x0F;        // 255
var bitXor = 0xFF ^ 0xFF;       // 0
var bitNot = ~0;                 // -1
var bitLsh = 1 << 8;            // 256
var bitRsh = 256 >> 4;          // 16

__result = (typeNum === 'number' && typeStr === 'string' && typeBool === 'boolean' && typeUndef === 'undefined' && ternary === 'yes' && notResult === true && bitAnd === 15 && bitOr === 255 && bitXor === 0 && bitLsh === 256 && bitRsh === 16) ? 1 : 0;
