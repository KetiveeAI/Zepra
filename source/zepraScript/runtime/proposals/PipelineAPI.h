/**
 * @file PipelineAPI.h
 * @brief Pipeline Operator Simulation
 */

#pragma once

#include <functional>
#include <tuple>
#include <utility>

namespace Zepra::Runtime {

// =============================================================================
// Pipe
// =============================================================================

template<typename T>
class Pipe {
public:
    explicit Pipe(T value) : value_(std::move(value)) {}
    
    template<typename F>
    auto operator|(F&& fn) const -> Pipe<decltype(fn(std::declval<T>()))> {
        return Pipe<decltype(fn(value_))>(fn(value_));
    }
    
    T value() const { return value_; }
    operator T() const { return value_; }

private:
    T value_;
};

template<typename T>
Pipe<T> pipe(T value) {
    return Pipe<T>(std::move(value));
}

// =============================================================================
// Function Composition
// =============================================================================

template<typename F, typename G>
auto compose(F&& f, G&& g) {
    return [f = std::forward<F>(f), g = std::forward<G>(g)](auto&&... args) {
        return f(g(std::forward<decltype(args)>(args)...));
    };
}

template<typename F, typename G, typename... Rest>
auto compose(F&& f, G&& g, Rest&&... rest) {
    return compose(std::forward<F>(f), compose(std::forward<G>(g), std::forward<Rest>(rest)...));
}

template<typename F, typename G>
auto flow(F&& f, G&& g) {
    return [f = std::forward<F>(f), g = std::forward<G>(g)](auto&&... args) {
        return g(f(std::forward<decltype(args)>(args)...));
    };
}

template<typename F, typename G, typename... Rest>
auto flow(F&& f, G&& g, Rest&&... rest) {
    return flow(flow(std::forward<F>(f), std::forward<G>(g)), std::forward<Rest>(rest)...);
}

// =============================================================================
// Partial Application
// =============================================================================

template<typename F, typename... BoundArgs>
auto partial(F&& f, BoundArgs&&... boundArgs) {
    return [f = std::forward<F>(f), 
            bound = std::make_tuple(std::forward<BoundArgs>(boundArgs)...)]
           (auto&&... args) mutable {
        return std::apply([&](auto&&... b) {
            return f(std::forward<decltype(b)>(b)..., std::forward<decltype(args)>(args)...);
        }, bound);
    };
}

template<typename F, typename... BoundArgs>
auto partialRight(F&& f, BoundArgs&&... boundArgs) {
    return [f = std::forward<F>(f),
            bound = std::make_tuple(std::forward<BoundArgs>(boundArgs)...)]
           (auto&&... args) mutable {
        return std::apply([&](auto&&... b) {
            return f(std::forward<decltype(args)>(args)..., std::forward<decltype(b)>(b)...);
        }, bound);
    };
}

// =============================================================================
// Curry
// =============================================================================

template<typename F>
auto curry(F&& f) {
    return [f = std::forward<F>(f)](auto&& arg) {
        return [f, arg = std::forward<decltype(arg)>(arg)](auto&&... args) {
            return f(arg, std::forward<decltype(args)>(args)...);
        };
    };
}

// =============================================================================
// Flip
// =============================================================================

template<typename F>
auto flip(F&& f) {
    return [f = std::forward<F>(f)](auto&& a, auto&& b, auto&&... rest) {
        return f(std::forward<decltype(b)>(b), 
                 std::forward<decltype(a)>(a),
                 std::forward<decltype(rest)>(rest)...);
    };
}

// =============================================================================
// Identity & Constant
// =============================================================================

inline auto identity = [](auto&& x) -> decltype(auto) { 
    return std::forward<decltype(x)>(x); 
};

template<typename T>
auto constant(T value) {
    return [value = std::move(value)](auto&&...) { return value; };
}

// =============================================================================
// Tap (Side Effect)
// =============================================================================

template<typename F>
auto tap(F&& sideEffect) {
    return [fn = std::forward<F>(sideEffect)](auto&& value) -> decltype(auto) {
        fn(value);
        return std::forward<decltype(value)>(value);
    };
}

} // namespace Zepra::Runtime
