// Array operations benchmark
var arr = []

// Push test
for (var i = 0; i < 1000; i++) {
    arr.push(i)
}
console.log("Array length after push: " + arr.length)

// Pop test
var sum = 0
while (arr.length > 0) {
    sum = sum + arr.pop()
}
console.log("Sum of popped: " + sum)

// Rebuild and map test
arr = [1, 2, 3, 4, 5]
var doubled = arr.map(function (x) { return x * 2 })
console.log("Mapped: " + doubled.join(", "))

// Filter test
var evens = arr.filter(function (x) { return x % 2 === 0 })
console.log("Evens: " + evens.join(", "))
