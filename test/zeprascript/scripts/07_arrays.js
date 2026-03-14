// Arrays

// Creation
var arr = [1, 2, 3, 4, 5];
var len = arr.length;

// Index access
var first = arr[0];
var last = arr[4];

// Push
arr.push(6);
var lenAfterPush = arr.length;

// Pop
var popped = arr.pop();
var lenAfterPop = arr.length;

// Sum via loop
var sum = 0;
for (var i = 0; i < arr.length; i = i + 1) {
    sum = sum + arr[i];
}

// Nested arrays
var matrix = [[1, 2], [3, 4]];
var m00 = matrix[0][0];
var m11 = matrix[1][1];

// Array of objects
var people = [
    { name: 'Alice', age: 30 },
    { name: 'Bob', age: 25 }
];
var aliceName = people[0].name;
var bobAge = people[1].age;

__result = (len === 5 && first === 1 && last === 5 && lenAfterPush === 6 && popped === 6 && lenAfterPop === 5 && sum === 15 && m00 === 1 && m11 === 4 && aliceName === 'Alice' && bobAge === 25) ? 1 : 0;
