#include "networking/resource_loader.hpp"
#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Zepra::Networking {

// Helpers
static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

ResourceLoader::ResourceLoader() {
    curl_global_init(CURL_GLOBAL_ALL);
}

ResourceLoader::~ResourceLoader() {
    curl_global_cleanup();
}

bool ResourceLoader::isSupportedScheme(const std::string& url) {
    return url.find("http://") == 0 || 
           url.find("https://") == 0 ||
           url.find("file://") == 0;
}

ResourceResponse ResourceLoader::loadUrl(const std::string& url) {
    ResourceResponse response;
    
    // Native file handling
    if (url.find("file://") == 0) {
        std::string path = url.substr(7);
        // Handle file:/// -> /
        // If url is file:///tmp, path is /tmp. 
        // Logic: url.substr(7) gives /tmp. Correct.
        
        std::ifstream file(path);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            response.data = buffer.str();
            response.success = true;
            response.statusCode = 200;
            return response;
        } else {
            response.success = false;
            response.error = "File not found: " + path;
            response.statusCode = 404;
            return response;
        }
    }
    
    CURL* curl = curl_easy_init();
    
    if (!curl) {
        response.success = false;
        response.error = "Failed to initialize libcurl";
        return response;
    }

    std::string readBuffer;
    std::string errorBuffer;
    errorBuffer.resize(CURL_ERROR_SIZE);
    
    // Set options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ZepraBrowser/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // For dev ease (disable in prod!)
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);       // 10s timeout
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, &errorBuffer[0]);
    
    // Perform request
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        response.success = false;
        response.error = std::string(errorBuffer.c_str());
        if (response.error.empty()) response.error = curl_easy_strerror(res);
    } else {
        response.success = true;
        
        // Get status code
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        response.statusCode = static_cast<int>(response_code);
        
        // Get content type
        char* ct;
        if (curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct) == CURLE_OK && ct) {
            response.contentType = std::string(ct);
        }
        
        response.data = std::move(readBuffer);
    }
    
    curl_easy_cleanup(curl);
    return response;
}

} // namespace Zepra::Networking
