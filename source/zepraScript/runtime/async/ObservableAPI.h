/**
 * @file ObservableAPI.h
 * @brief Observable Pattern Implementation
 */

#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

namespace Zepra::Runtime {

// =============================================================================
// Subscription
// =============================================================================

class Subscription {
public:
    virtual ~Subscription() = default;
    virtual void unsubscribe() = 0;
    virtual bool closed() const = 0;
};

class BasicSubscription : public Subscription {
public:
    using Cleanup = std::function<void()>;
    
    explicit BasicSubscription(Cleanup cleanup = nullptr) : cleanup_(std::move(cleanup)) {}
    
    void unsubscribe() override {
        if (closed_.exchange(true)) return;
        if (cleanup_) cleanup_();
    }
    
    bool closed() const override { return closed_; }

private:
    Cleanup cleanup_;
    std::atomic<bool> closed_{false};
};

// =============================================================================
// Observer
// =============================================================================

template<typename T>
struct Observer {
    std::function<void(const T&)> next;
    std::function<void(std::exception_ptr)> error;
    std::function<void()> complete;
};

// =============================================================================
// Observable
// =============================================================================

template<typename T>
class Observable {
public:
    using SubscribeHandler = std::function<std::shared_ptr<Subscription>(Observer<T>)>;
    
    explicit Observable(SubscribeHandler handler) : handler_(std::move(handler)) {}
    
    std::shared_ptr<Subscription> subscribe(Observer<T> observer) const {
        return handler_(std::move(observer));
    }
    
    std::shared_ptr<Subscription> subscribe(
        std::function<void(const T&)> onNext,
        std::function<void(std::exception_ptr)> onError = nullptr,
        std::function<void()> onComplete = nullptr) const {
        return subscribe({std::move(onNext), std::move(onError), std::move(onComplete)});
    }
    
    // Operators
    template<typename U>
    Observable<U> map(std::function<U(const T&)> fn) const {
        auto source = handler_;
        return Observable<U>([source, fn](Observer<U> observer) {
            return source({
                [observer, fn](const T& value) {
                    if (observer.next) observer.next(fn(value));
                },
                observer.error,
                observer.complete
            });
        });
    }
    
    Observable filter(std::function<bool(const T&)> predicate) const {
        auto source = handler_;
        return Observable([source, predicate](Observer<T> observer) {
            return source({
                [observer, predicate](const T& value) {
                    if (predicate(value) && observer.next) observer.next(value);
                },
                observer.error,
                observer.complete
            });
        });
    }
    
    Observable take(size_t count) const {
        auto source = handler_;
        return Observable([source, count](Observer<T> observer) mutable {
            auto remaining = std::make_shared<size_t>(count);
            auto sub = std::make_shared<std::shared_ptr<Subscription>>();
            
            *sub = source({
                [observer, remaining, sub](const T& value) {
                    if (*remaining > 0) {
                        --(*remaining);
                        if (observer.next) observer.next(value);
                        if (*remaining == 0) {
                            if (observer.complete) observer.complete();
                            if (*sub) (*sub)->unsubscribe();
                        }
                    }
                },
                observer.error,
                observer.complete
            });
            return *sub;
        });
    }
    
    Observable skip(size_t count) const {
        auto source = handler_;
        return Observable([source, count](Observer<T> observer) mutable {
            auto skipped = std::make_shared<size_t>(0);
            return source({
                [observer, skipped, count](const T& value) {
                    if (*skipped < count) {
                        ++(*skipped);
                    } else if (observer.next) {
                        observer.next(value);
                    }
                },
                observer.error,
                observer.complete
            });
        });
    }
    
    // Static creators
    static Observable from(const std::vector<T>& values) {
        return Observable([values](Observer<T> observer) {
            for (const auto& v : values) {
                if (observer.next) observer.next(v);
            }
            if (observer.complete) observer.complete();
            return std::make_shared<BasicSubscription>();
        });
    }
    
    static Observable of(std::initializer_list<T> values) {
        return from(std::vector<T>(values));
    }
    
    static Observable empty() {
        return Observable([](Observer<T> observer) {
            if (observer.complete) observer.complete();
            return std::make_shared<BasicSubscription>();
        });
    }
    
    static Observable never() {
        return Observable([](Observer<T>) {
            return std::make_shared<BasicSubscription>();
        });
    }

private:
    SubscribeHandler handler_;
};

// =============================================================================
// Subject (Observable + Observer)
// =============================================================================

template<typename T>
class Subject {
public:
    void next(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& obs : observers_) {
            if (obs.next) obs.next(value);
        }
    }
    
    void error(std::exception_ptr err) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& obs : observers_) {
            if (obs.error) obs.error(err);
        }
    }
    
    void complete() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& obs : observers_) {
            if (obs.complete) obs.complete();
        }
    }
    
    Observable<T> asObservable() const {
        auto self = const_cast<Subject*>(this);
        return Observable<T>([self](Observer<T> observer) {
            return self->subscribe(std::move(observer));
        });
    }
    
    std::shared_ptr<Subscription> subscribe(Observer<T> observer) {
        std::lock_guard<std::mutex> lock(mutex_);
        observers_.push_back(observer);
        
        auto self = this;
        auto idx = observers_.size() - 1;
        return std::make_shared<BasicSubscription>([self, idx]() {
            std::lock_guard<std::mutex> lock(self->mutex_);
            if (idx < self->observers_.size()) {
                self->observers_.erase(self->observers_.begin() + idx);
            }
        });
    }

private:
    std::vector<Observer<T>> observers_;
    mutable std::mutex mutex_;
};

} // namespace Zepra::Runtime
