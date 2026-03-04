/**
 * @file fetch_tests.cpp
 * @brief Integration tests for Fetch API with real HTTP requests
 */

#include <gtest/gtest.h>
#include "browser/fetch.hpp"
#include "browser/url.hpp"
#include "runtime/async/promise.hpp"

using namespace Zepra::Browser;
using namespace Zepra::Runtime;

// =============================================================================
// URL Integration with Fetch
// =============================================================================

TEST(FetchIntegrationTests, URLParsing) {
    URL url("https://jsonplaceholder.typicode.com/todos/1");
    
    EXPECT_TRUE(url.isValid());
    EXPECT_EQ(url.protocol(), "https:");
    EXPECT_EQ(url.hostname(), "jsonplaceholder.typicode.com");
    EXPECT_EQ(url.pathname(), "/todos/1");
}

// =============================================================================
// Headers Tests
// =============================================================================

TEST(FetchIntegrationTests, HeadersBasic) {
    Headers headers;
    
    headers.setHeader("Content-Type", "application/json");
    headers.setHeader("Authorization", "Bearer token123");
    
    EXPECT_EQ(headers.getHeader("content-type"), "application/json");
    EXPECT_EQ(headers.getHeader("authorization"), "Bearer token123");
    EXPECT_TRUE(headers.has("Content-Type"));
}

TEST(FetchIntegrationTests, HeadersAppend) {
    Headers headers;
    
    headers.append("Accept", "text/html");
    headers.append("Accept", "application/json");
    
    std::string accept = headers.getHeader("accept");
    EXPECT_NE(accept.find("text/html"), std::string::npos);
    EXPECT_NE(accept.find("application/json"), std::string::npos);
}

TEST(FetchIntegrationTests, HeadersRemove) {
    Headers headers;
    
    headers.setHeader("X-Custom", "value");
    EXPECT_TRUE(headers.has("x-custom"));
    
    headers.remove("X-Custom");
    EXPECT_FALSE(headers.has("x-custom"));
}

// =============================================================================
// Request Tests
// =============================================================================

TEST(FetchIntegrationTests, RequestBasic) {
    Request req("https://api.example.com/data", "POST");
    
    EXPECT_EQ(req.url(), "https://api.example.com/data");
    EXPECT_EQ(req.method(), "POST");
}

TEST(FetchIntegrationTests, RequestWithBody) {
    Request req("https://api.example.com/data", "POST");
    req.setBody("{\"key\": \"value\"}");
    
    EXPECT_EQ(req.body(), "{\"key\": \"value\"}");
}

TEST(FetchIntegrationTests, RequestWithHeaders) {
    Request req("https://api.example.com/data");
    req.setHeader("Accept", "application/json");
    
    EXPECT_EQ(req.headers()->getHeader("accept"), "application/json");
}

// =============================================================================
// Response Tests
// =============================================================================

TEST(FetchIntegrationTests, ResponseConstruction) {
    Response resp(200, "OK");
    
    EXPECT_EQ(resp.status(), 200);
    EXPECT_EQ(resp.statusText(), "OK");
    EXPECT_TRUE(resp.ok());
}

TEST(FetchIntegrationTests, ResponseNotOk) {
    Response resp(404, "Not Found");
    
    EXPECT_EQ(resp.status(), 404);
    EXPECT_FALSE(resp.ok());
}

TEST(FetchIntegrationTests, ResponseWithBody) {
    Response resp(200);
    resp.setBody("Hello World");
    
    EXPECT_EQ(resp.body(), "Hello World");
    EXPECT_FALSE(resp.bodyUsed());
}

TEST(FetchIntegrationTests, ResponseHeaders) {
    Response resp(200);
    resp.headers()->setHeader("Content-Type", "text/plain");
    
    EXPECT_EQ(resp.headers()->getHeader("content-type"), "text/plain");
}

// =============================================================================
// Live HTTP Tests (require network)
// =============================================================================

// Note: These tests require network access. They use jsonplaceholder.typicode.com
// which is a free fake API for testing.

TEST(FetchLiveTests, DISABLED_GetRequest) {
    // Disabled by default - enable when testing with network
    Promise* promise = FetchAPI::fetch("https://jsonplaceholder.typicode.com/todos/1");
    
    EXPECT_NE(promise, nullptr);
    // In a real async test, we'd wait for resolution
}

TEST(FetchLiveTests, DISABLED_PostRequest) {
    Request* req = new Request("https://jsonplaceholder.typicode.com/posts", "POST");
    req->setBody("{\"title\":\"test\",\"body\":\"content\",\"userId\":1}");
    req->setHeader("Content-Type", "application/json");
    
    Promise* promise = FetchAPI::fetch(req->url(), req);
    EXPECT_NE(promise, nullptr);
    
    delete req;
}
