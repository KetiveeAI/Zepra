#pragma once
#include <string>
#include <functional>

namespace zepra {

class AttentionManager {
public:
    AttentionManager();
    ~AttentionManager();

    void requestAttention(const std::string& tabId);
    void clearAttention(const std::string& tabId);
    void setAttentionCallback(std::function<void(const std::string&)> cb);
    void notifyUser(const std::string& message);

private:
    std::function<void(const std::string&)> attentionCallback;
};

} // namespace zepra 