// Fibonacci benchmark - CPU bound test
function fib(n) {
    if (n <= 1) return n
    return fib(n - 1) + fib(n - 2)
}

var start = Date.now()
var result = fib(25)
var elapsed = Date.now() - start

console.log("fib(25) = " + result)
console.log("Time: " + elapsed + "ms")
