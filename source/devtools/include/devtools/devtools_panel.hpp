/**
 * @file devtools_panel.hpp
 * @brief Base DevTools Panel interface
 * 
 * Provides abstract base for all DevTools panels.
 * Individual panels (elements, console, network, etc.) extend this.
 */

#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace Zepra::DevTools {

/**
 * @class DevToolsPanel
 * @brief Abstract base class for DevTools panels
 */
class DevToolsPanel {
public:
    explicit DevToolsPanel(const std::string& name) : name_(name) {}
    virtual ~DevToolsPanel() = default;
    
    // Panel identity
    const std::string& name() const { return name_; }
    
    // Lifecycle
    virtual void render() = 0;
    virtual void refresh() = 0;
    
    // Visibility
    bool isVisible() const { return visible_; }
    void setVisible(bool v) { visible_ = v; }
    
protected:
    std::string name_;
    bool visible_ = true;
};

/**
 * @brief Network request entry for Network panel
 */
struct NetworkEntry {
    int id;
    std::string url;
    std::string method;
    int status;
    std::string statusText;
    std::string type;
    size_t responseSize;
    double startTime;
    double endTime;
    std::vector<std::pair<std::string, std::string>> requestHeaders;
    std::vector<std::pair<std::string, std::string>> responseHeaders;
    std::string requestBody;
    std::string responseBody;
    
    bool isError() const { return status >= 400; }
};

} // namespace Zepra::DevTools
