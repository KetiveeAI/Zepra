// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once
#include <string>
#include <vector>
#include <functional>

namespace zepra {

struct AutoFillEntry {
    std::string field;
    std::string value;
    std::string type; // e.g., email, password, address
};

class AutoFillManager {
public:
    AutoFillManager();
    ~AutoFillManager();

    void addEntry(const AutoFillEntry& entry);
    std::vector<AutoFillEntry> getSuggestions(const std::string& field, const std::string& context) const;
    void setSuggestionCallback(std::function<void(const std::vector<AutoFillEntry>&)> cb);
    void clearEntries();

private:
    std::vector<AutoFillEntry> entries;
    std::function<void(const std::vector<AutoFillEntry>&)> suggestionCallback;
};

} // namespace zepra 