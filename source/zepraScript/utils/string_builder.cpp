/**
 * @file string_builder.cpp
 * @brief Efficient string concatenation for hot paths
 * 
 * Uses a chunked rope approach: accumulates fragments and joins on demand.
 * Avoids O(n²) reallocation from repeated std::string += in loops.
 */

#include "config.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>

namespace Zepra::Utils {

class StringBuilder {
public:
    StringBuilder() = default;

    explicit StringBuilder(size_t reserveHint) {
        chunks_.reserve(reserveHint > 16 ? 16 : reserveHint);
    }

    void append(const std::string& s) {
        if (!s.empty()) {
            chunks_.push_back(s);
            totalLen_ += s.size();
        }
    }

    void append(const char* s, size_t len) {
        if (len > 0) {
            chunks_.emplace_back(s, len);
            totalLen_ += len;
        }
    }

    void append(char c) {
        chunks_.emplace_back(1, c);
        totalLen_ += 1;
    }

    void appendInt(int64_t value) {
        append(std::to_string(value));
    }

    void appendDouble(double value) {
        // Match JS number-to-string behavior
        if (value == 0.0) {
            append("0");
            return;
        }
        append(std::to_string(value));
    }

    std::string toString() const {
        std::string result;
        result.reserve(totalLen_);
        for (const auto& chunk : chunks_) {
            result.append(chunk);
        }
        return result;
    }

    size_t length() const { return totalLen_; }
    bool empty() const { return totalLen_ == 0; }

    void clear() {
        chunks_.clear();
        totalLen_ = 0;
    }

private:
    std::vector<std::string> chunks_;
    size_t totalLen_ = 0;
};

} // namespace Zepra::Utils
