#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>

namespace Zepra::Networking {

struct ResourceResponse {
    bool success = false;
    int statusCode = 0;
    std::string data;
    std::string contentType;
    std::string error;
};

class ResourceLoader {
public:
    ResourceLoader();
    ~ResourceLoader();

    // Check if URL is supported (http/https/file)
    bool isSupportedScheme(const std::string& url);

    // Load resource synchronously
    ResourceResponse loadUrl(const std::string& url);
};

} // namespace Zepra::Networking
