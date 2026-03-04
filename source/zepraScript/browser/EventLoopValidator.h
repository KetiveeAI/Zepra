/**
 * @file EventLoopValidator.h
 * @brief Event Loop and Microtask Correctness Validation
 * 
 * Implements:
 * - Microtask vs macrotask ordering verification
 * - Promise resolution timing checks
 * - Module loading order validation
 * - Worker message ordering
 */

#pragma once

#include <vector>
#include <queue>
#include <string>
#include <functional>
#include <chrono>
#include <optional>

namespace Zepra::Browser {

// =============================================================================
// Task Types
// =============================================================================

enum class TaskType {
    Script,         // Script execution
    Microtask,      // Promise callbacks, queueMicrotask
    Macrotask,      // setTimeout, setInterval, I/O
    Render,         // requestAnimationFrame
    Idle            // requestIdleCallback
};

/**
 * @brief Task record for validation
 */
struct TaskRecord {
    uint64_t id;
    TaskType type;
    std::chrono::steady_clock::time_point scheduledAt;
    std::chrono::steady_clock::time_point executedAt;
    std::string source;  // Where task was scheduled from
    bool completed = false;
};

// =============================================================================
// Event Loop Validator
// =============================================================================

/**
 * @brief Validates event loop execution order
 */
class EventLoopValidator {
public:
    // Record task scheduling
    uint64_t recordSchedule(TaskType type, const std::string& source) {
        TaskRecord record;
        record.id = nextId_++;
        record.type = type;
        record.scheduledAt = std::chrono::steady_clock::now();
        record.source = source;
        
        pendingTasks_.push_back(record);
        return record.id;
    }
    
    // Record task execution
    void recordExecution(uint64_t taskId) {
        for (auto& task : pendingTasks_) {
            if (task.id == taskId) {
                task.executedAt = std::chrono::steady_clock::now();
                task.completed = true;
                executionOrder_.push_back(taskId);
                break;
            }
        }
    }
    
    // Validate microtask ordering
    // Rule: All microtasks must complete before next macrotask
    bool validateMicrotaskOrdering() const {
        bool inMicrotasks = false;
        for (uint64_t id : executionOrder_) {
            auto task = findTask(id);
            if (!task) continue;
            
            if (task->type == TaskType::Microtask) {
                inMicrotasks = true;
            } else if (inMicrotasks && task->type == TaskType::Macrotask) {
                // Macrotask after microtask - check all microtasks done
                for (const auto& pending : pendingTasks_) {
                    if (pending.type == TaskType::Microtask && !pending.completed) {
                        return false;  // Microtask still pending
                    }
                }
                inMicrotasks = false;
            }
        }
        return true;
    }
    
    // Validate Promise resolution timing
    // Promise.resolve().then() must run before setTimeout(0)
    bool validatePromiseTiming() const {
        std::optional<size_t> firstPromiseThen;
        std::optional<size_t> firstSetTimeout;
        
        for (size_t i = 0; i < executionOrder_.size(); i++) {
            auto task = findTask(executionOrder_[i]);
            if (!task) continue;
            
            if (task->type == TaskType::Microtask && 
                task->source.find("Promise") != std::string::npos) {
                if (!firstPromiseThen) firstPromiseThen = i;
            }
            if (task->type == TaskType::Macrotask &&
                task->source.find("setTimeout") != std::string::npos) {
                if (!firstSetTimeout) firstSetTimeout = i;
            }
        }
        
        // Promise.then should execute before setTimeout
        if (firstPromiseThen && firstSetTimeout) {
            return *firstPromiseThen < *firstSetTimeout;
        }
        return true;  // No conflict to check
    }
    
    // Clear validation state
    void reset() {
        pendingTasks_.clear();
        executionOrder_.clear();
        nextId_ = 1;
    }
    
private:
    const TaskRecord* findTask(uint64_t id) const {
        for (const auto& task : pendingTasks_) {
            if (task.id == id) return &task;
        }
        return nullptr;
    }
    
    std::vector<TaskRecord> pendingTasks_;
    std::vector<uint64_t> executionOrder_;
    uint64_t nextId_ = 1;
};

// =============================================================================
// Module Loading Order Validator
// =============================================================================

/**
 * @brief Validates ES module loading order
 */
class ModuleLoadValidator {
public:
    struct ModuleRecord {
        std::string url;
        std::vector<std::string> imports;  // Dependencies
        enum class State { Requested, Fetched, Parsed, Linked, Evaluated };
        State state = State::Requested;
    };
    
    // Record module request
    void recordRequest(const std::string& url, const std::vector<std::string>& deps) {
        modules_[url] = {url, deps, ModuleRecord::State::Requested};
    }
    
    // Record state transition
    void recordStateChange(const std::string& url, ModuleRecord::State newState) {
        auto it = modules_.find(url);
        if (it != modules_.end()) {
            it->second.state = newState;
            stateTransitions_.push_back({url, newState});
        }
    }
    
    // Validate: Dependencies must be evaluated before dependents
    bool validateEvaluationOrder() const {
        for (const auto& [url, record] : modules_) {
            if (record.state == ModuleRecord::State::Evaluated) {
                // Check all deps were evaluated first
                for (const auto& dep : record.imports) {
                    auto depIt = modules_.find(dep);
                    if (depIt == modules_.end()) continue;
                    
                    // Find evaluation positions
                    int myPos = findEvaluationPosition(url);
                    int depPos = findEvaluationPosition(dep);
                    
                    if (myPos < depPos) {
                        return false;  // Evaluated before dependency
                    }
                }
            }
        }
        return true;
    }
    
    // Validate: Circular deps are allowed but must not cause infinite loop
    bool validateCircularHandling() const {
        // Check for cycles in dependency graph
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> stack;
        
        for (const auto& [url, _] : modules_) {
            if (hasCycle(url, visited, stack)) {
                // Cycle detected - verify it's handled (not infinite loop)
                // Check that all modules eventually got evaluated
                for (const auto& [u, r] : modules_) {
                    if (r.state != ModuleRecord::State::Evaluated) {
                        return false;  // Module stuck
                    }
                }
            }
        }
        return true;
    }
    
private:
    int findEvaluationPosition(const std::string& url) const {
        int pos = 0;
        for (const auto& [u, state] : stateTransitions_) {
            if (state == ModuleRecord::State::Evaluated) {
                if (u == url) return pos;
                pos++;
            }
        }
        return -1;
    }
    
    bool hasCycle(const std::string& url, 
                  std::unordered_set<std::string>& visited,
                  std::unordered_set<std::string>& stack) const {
        if (stack.count(url)) return true;
        if (visited.count(url)) return false;
        
        visited.insert(url);
        stack.insert(url);
        
        auto it = modules_.find(url);
        if (it != modules_.end()) {
            for (const auto& dep : it->second.imports) {
                if (hasCycle(dep, visited, stack)) return true;
            }
        }
        
        stack.erase(url);
        return false;
    }
    
    std::unordered_map<std::string, ModuleRecord> modules_;
    std::vector<std::pair<std::string, ModuleRecord::State>> stateTransitions_;
};

// =============================================================================
// Worker Message Ordering
// =============================================================================

/**
 * @brief Validates worker message ordering
 */
class WorkerMessageValidator {
public:
    struct Message {
        uint64_t id;
        std::string fromThread;
        std::string toThread;
        std::chrono::steady_clock::time_point sent;
        std::chrono::steady_clock::time_point received;
        bool delivered = false;
    };
    
    // Record message send
    uint64_t recordSend(const std::string& from, const std::string& to) {
        Message msg;
        msg.id = nextId_++;
        msg.fromThread = from;
        msg.toThread = to;
        msg.sent = std::chrono::steady_clock::now();
        messages_.push_back(msg);
        return msg.id;
    }
    
    // Record message receive
    void recordReceive(uint64_t msgId) {
        for (auto& msg : messages_) {
            if (msg.id == msgId) {
                msg.received = std::chrono::steady_clock::now();
                msg.delivered = true;
                break;
            }
        }
    }
    
    // Validate: Messages between same pair of threads are FIFO
    bool validateFIFO() const {
        std::unordered_map<std::string, std::vector<const Message*>> byPair;
        
        for (const auto& msg : messages_) {
            std::string key = msg.fromThread + "->" + msg.toThread;
            byPair[key].push_back(&msg);
        }
        
        for (const auto& [pair, msgs] : byPair) {
            for (size_t i = 1; i < msgs.size(); i++) {
                // Messages sent earlier should be received earlier
                if (msgs[i-1]->delivered && msgs[i]->delivered) {
                    if (msgs[i]->received < msgs[i-1]->received) {
                        return false;  // FIFO violation
                    }
                }
            }
        }
        return true;
    }
    
    // Validate: All messages eventually delivered
    bool validateDelivery() const {
        for (const auto& msg : messages_) {
            if (!msg.delivered) {
                return false;  // Message lost
            }
        }
        return true;
    }
    
private:
    std::vector<Message> messages_;
    uint64_t nextId_ = 1;
};

} // namespace Zepra::Browser
