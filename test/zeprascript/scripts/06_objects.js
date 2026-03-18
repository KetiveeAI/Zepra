// Objects

// Object literal
var obj = { x: 10, y: 20 };
var objX = obj.x;
var objY = obj.y;

// Property assignment
obj.z = 30;
var objZ = obj.z;

// Bracket access
var key = 'x';
var bracketAccess = obj[key];

// Nested objects
var nested = { inner: { value: 42 } };
var nestedVal = nested.inner.value;

// Object methods
var calc = {
    value: 0,
    add: function(n) {
        this.value = this.value + n;
        return this;
    },
    get: function() {
        return this.value;
    }
};

calc.add(10);
calc.add(20);
var calcResult = calc.get(); // 30

__result = (objX === 10 && objY === 20 && objZ === 30 && bracketAccess === 10 && nestedVal === 42 && calcResult === 30) ? 1 : 0;
