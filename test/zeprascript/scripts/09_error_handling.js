// Error handling

// Basic try/catch
var caught = false;
try {
    throw 'test error';
} catch (e) {
    caught = true;
}

// try/catch/finally
var finallyRan = false;
var catchVal = '';
try {
    throw 'oops';
} catch (e) {
    catchVal = e;
} finally {
    finallyRan = true;
}

// Nested try/catch
var innerCatch = '';
var outerCatch = '';
try {
    try {
        throw 'inner';
    } catch (e) {
        innerCatch = e;
        throw 'outer';
    }
} catch (e) {
    outerCatch = e;
}

// throw in loop with catch outside
var loopCatch = 0;
for (var i = 0; i < 10; i = i + 1) {
    try {
        if (i === 5) {
            throw 'stop';
        }
    } catch (e) {
        loopCatch = i;
    }
}

// finally always executes
var finallyCount = 0;
for (var j = 0; j < 3; j = j + 1) {
    try {
        if (j === 1) throw 'skip';
    } catch (e) {
        // caught
    } finally {
        finallyCount = finallyCount + 1;
    }
}

__result = (caught === true && catchVal === 'oops' && finallyRan === true && innerCatch === 'inner' && outerCatch === 'outer' && loopCatch === 5 && finallyCount === 3) ? 1 : 0;
