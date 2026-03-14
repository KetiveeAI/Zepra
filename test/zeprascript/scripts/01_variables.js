// Variables and scoping
var x = 10;
var y = 20;
var sum = x + y;

// Reassignment
x = 100;
var product = x * y;

// Multiple declarations
var a = 1, b = 2, c = 3;
var abc_sum = a + b + c;

// Undefined default
var undef_var;

// Boolean variables
var flag = true;
var not_flag = !flag;

// Null
var nothing = null;

// Nested scope
var outer = 1;
if (true) {
    var inner = 2;
    outer = outer + inner;
}

// Result: sum=30, product=2000, abc_sum=6, outer=3
__result = (sum === 30 && product === 2000 && abc_sum === 6 && outer === 3 && flag === true && not_flag === false && nothing === null) ? 1 : 0;
