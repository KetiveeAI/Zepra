/**
 * @file url_tests.cpp
 * @brief Unit tests for URL and URLSearchParams classes
 */

#include <gtest/gtest.h>
#include "zeprascript/browser/url.hpp"

using namespace Zepra::Browser;

// =============================================================================
// URL Parsing Tests
// =============================================================================

TEST(URLTests, BasicParsing) {
    URL url("https://example.com/path?query=1#hash");
    
    EXPECT_TRUE(url.isValid());
    EXPECT_EQ(url.protocol(), "https:");
    EXPECT_EQ(url.hostname(), "example.com");
    EXPECT_EQ(url.pathname(), "/path");
    EXPECT_EQ(url.search(), "?query=1");
    EXPECT_EQ(url.hash(), "#hash");
}

TEST(URLTests, WithPort) {
    URL url("http://localhost:8080/api");
    
    EXPECT_TRUE(url.isValid());
    EXPECT_EQ(url.hostname(), "localhost");
    EXPECT_EQ(url.port(), "8080");
    EXPECT_EQ(url.host(), "localhost:8080");
}

TEST(URLTests, WithUserInfo) {
    URL url("https://user:pass@example.com/");
    
    EXPECT_TRUE(url.isValid());
    EXPECT_EQ(url.username(), "user");
    EXPECT_EQ(url.password(), "pass");
    EXPECT_EQ(url.hostname(), "example.com");
}

TEST(URLTests, Origin) {
    URL url("https://example.com:443/path");
    
    EXPECT_EQ(url.origin(), "https://example.com:443");
}

TEST(URLTests, Href) {
    URL url("https://example.com/path?q=1#sec");
    
    std::string href = url.href();
    EXPECT_NE(href.find("https://"), std::string::npos);
    EXPECT_NE(href.find("example.com"), std::string::npos);
    EXPECT_NE(href.find("/path"), std::string::npos);
}

TEST(URLTests, InvalidURL) {
    URL url("not-a-url");
    EXPECT_FALSE(url.isValid());
}

TEST(URLTests, CanParse) {
    EXPECT_TRUE(URL::canParse("https://example.com"));
    EXPECT_FALSE(URL::canParse("invalid"));
}

// =============================================================================
// URL Mutation Tests
// =============================================================================

TEST(URLTests, SetPathname) {
    URL url("https://example.com/old");
    url.setPathname("/new/path");
    
    EXPECT_EQ(url.pathname(), "/new/path");
}

TEST(URLTests, SetSearch) {
    URL url("https://example.com/");
    url.setSearch("key=value");
    
    EXPECT_EQ(url.search(), "?key=value");
}

TEST(URLTests, SetHash) {
    URL url("https://example.com/");
    url.setHash("section1");
    
    EXPECT_EQ(url.hash(), "#section1");
}

// =============================================================================
// URLSearchParams Tests
// =============================================================================

TEST(URLSearchParamsTests, ParseQueryString) {
    URLSearchParams params("foo=1&bar=2");
    
    auto foo = params.getParam("foo");
    auto bar = params.getParam("bar");
    
    EXPECT_TRUE(foo.has_value());
    EXPECT_TRUE(bar.has_value());
    EXPECT_EQ(foo.value(), "1");
    EXPECT_EQ(bar.value(), "2");
}

TEST(URLSearchParamsTests, ParseWithLeadingQuestion) {
    URLSearchParams params("?name=test");
    
    auto name = params.getParam("name");
    EXPECT_TRUE(name.has_value());
    EXPECT_EQ(name.value(), "test");
}

TEST(URLSearchParamsTests, Append) {
    URLSearchParams params;
    params.append("key", "value1");
    params.append("key", "value2");
    
    auto all = params.getAll("key");
    EXPECT_EQ(all.size(), 2);
    EXPECT_EQ(all[0], "value1");
    EXPECT_EQ(all[1], "value2");
}

TEST(URLSearchParamsTests, Set) {
    URLSearchParams params("key=old");
    params.set("key", "new");
    
    auto val = params.getParam("key");
    EXPECT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "new");
}

TEST(URLSearchParamsTests, Delete) {
    URLSearchParams params("a=1&b=2&c=3");
    params.deleteParam("b");
    
    EXPECT_FALSE(params.has("b"));
    EXPECT_TRUE(params.has("a"));
    EXPECT_TRUE(params.has("c"));
}

TEST(URLSearchParamsTests, Has) {
    URLSearchParams params("exists=yes");
    
    EXPECT_TRUE(params.has("exists"));
    EXPECT_FALSE(params.has("missing"));
}

TEST(URLSearchParamsTests, ToString) {
    URLSearchParams params;
    params.append("a", "1");
    params.append("b", "2");
    
    std::string str = params.toString();
    EXPECT_NE(str.find("a=1"), std::string::npos);
    EXPECT_NE(str.find("b=2"), std::string::npos);
}

TEST(URLSearchParamsTests, Sort) {
    URLSearchParams params("z=3&a=1&m=2");
    params.sort();
    
    auto entries = params.entries();
    EXPECT_EQ(entries[0].first, "a");
    EXPECT_EQ(entries[1].first, "m");
    EXPECT_EQ(entries[2].first, "z");
}

// =============================================================================
// Percent Encoding Tests
// =============================================================================

TEST(URLTests, PercentEncode) {
    std::string encoded = URL::percentEncode("hello world");
    EXPECT_NE(encoded.find("%20"), std::string::npos);
}

TEST(URLTests, PercentDecode) {
    std::string decoded = URL::percentDecode("hello%20world");
    EXPECT_EQ(decoded, "hello world");
}

TEST(URLTests, PercentDecodeSpecialChars) {
    std::string decoded = URL::percentDecode("key%3Dvalue%26other");
    EXPECT_EQ(decoded, "key=value&other");
}
