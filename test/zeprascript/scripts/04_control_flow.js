// Control flow

// if/else
var x = 10;
var ifResult;
if (x > 5) {
    ifResult = 'big';
} else {
    ifResult = 'small';
}

// Nested if
var nested;
if (x > 20) {
    nested = 'huge';
} else if (x > 5) {
    nested = 'medium';
} else {
    nested = 'tiny';
}

// for loop
var forSum = 0;
for (var i = 0; i < 10; i = i + 1) {
    forSum = forSum + i;
}

// while loop
var whileSum = 0;
var j = 0;
while (j < 10) {
    whileSum = whileSum + j;
    j = j + 1;
}

// do-while
var doCount = 0;
var k = 0;
do {
    doCount = doCount + 1;
    k = k + 1;
} while (k < 5);

// break
var breakVal = 0;
for (var m = 0; m < 100; m = m + 1) {
    if (m === 10) {
        break;
    }
    breakVal = breakVal + 1;
}

// continue
var contSum = 0;
for (var n = 0; n < 10; n = n + 1) {
    if (n % 2 === 0) {
        continue;
    }
    contSum = contSum + n; // 1+3+5+7+9 = 25
}

__result = (ifResult === 'big' && nested === 'medium' && forSum === 45 && whileSum === 45 && doCount === 5 && breakVal === 10 && contSum === 25) ? 1 : 0;
