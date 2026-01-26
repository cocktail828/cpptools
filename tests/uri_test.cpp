#include <chrono>
#include <iostream>
#include <string>

#include <gtest/gtest.h>
#include "uri.h"

TEST(UriTest, BasicParsing1) {
    auto check = [](const std::string& uri) { EXPECT_EQ(cpptools::net::URI(uri).to_string(), uri); };

    check("http://example.com");
    check("http://user@example.com");
    check("http://:pass@example.com");
    check("http://user:pass@example.com");
    check("http://@example.com");
    check("http://:@example.com");
    check("http://user:@example.com");
    check("http://user%40domain:pass%23123@example.com");
    check("http://example.com");
    check("http://example.com:8080");
    check("http://192.168.1.1");
    check("http://192.168.1.1:9000");
    check("http://[2001:db8::1]");
    check("http://[2001:db8::1]:8080");
    check("redis://host1:6379,host2:6380");
    check("etcd://node1:2379,node2:2380,node3:2381");
    check("custom://host1,host2,host3");
    check("http://example.com");
    check("http://example.com/");
    check("http://example.com/path/to/resource");
    check("http://example.com/path%20with%20spaces");
    check("http://example.com/path/with/special/chars!@#$%^&*()");
    check("http://example.com/path/with/unicode/%E2%9C%93");
    check("http://example.com/a/b/c/d/e/f/g/h/i/j/k");
    check("http://example.com");
    check("http://example.com?");
    check("http://example.com?query=value");
    check("http://example.com?query1=value1&query2=value2");
    check("http://example.com?name=boost&version=1.83&lang=c%2B%2B");
    check("http://example.com?tags=cpp,redis,cluster");
    check("http://example.com?search=hello%20world&sort=asc&limit=100");
    check("http://example.com?a=1&b=2&c=3&d=4&e=5");
    check("http://example.com");
    check("http://example.com#section1");
    check("http://example.com#section%20with%20spaces");
    check("http://example.com#complex-fragment_123!@#$%^&*()");
    check("http://example.com/path?query=value#frag");
    check("http://user@example.com:8080/path?query=value#frag");
    check("https://:pass@example.com/path/to/resource?tags=cpp,boost#section1");
    check("ftp://user:password@192.168.1.1:21/files?sort=asc&limit=100#top");
    check("mongodb://user%40domain:pass%23123@[2001:db8::1]:27017/db?authSource=admin");
    check("custom://user:pass@host1:8080,host2:8081,host3:8082/path?query=cluster#status");
    check("redis://:secretpass@localhost:6379/0?client_setname=myapp");
    check("http://example.com/path%2Fwith%2Fslashes?query%3Dwith%3Dequals#frag%26with%26amps");
    check("https://admin:secret123@multi.host.com:9000/api/v1/users?filter=active&sort=name%2Casc#results");
    check("http://user%21%40%23:pass%24%25%5E@example.com");
    check("http://example.com/path%2Fwith%2Fslashes");
    check("http://example.com?query=hello%20world%21");
    check("http://example.com#frag%26with%26amps");
    check("http://user%21%40%23:pass%24%25%5E@example.com/path%2Fwith%3Fquery%23frag");
}

// Basic parsing test
TEST(UriTest, BasicParsing2) {
    auto u = cpptools::net::URI("http://user:pass@host1:80,host2:81/path/to/%E4%BD%A0%E5%A5%BD?x=1&y=%20#frag");
    EXPECT_EQ(u.to_string(), u.uri());
    EXPECT_EQ(u.scheme(), "http");

    auto up = u.User();
    EXPECT_TRUE(up.has_value());
    EXPECT_EQ(up->first, "user");
    EXPECT_EQ(up->second, "pass");

    EXPECT_EQ(u.hosts().size(), 2);
    EXPECT_EQ(u.hosts()[0].host, "host1");
    EXPECT_EQ(u.hosts()[0].port, 80);
    EXPECT_EQ(u.hosts()[1].host, "host2");
    EXPECT_EQ(u.hosts()[1].port, 81);
    EXPECT_EQ(u.path(), "/path/to/你好");

    auto q = u.query();
    EXPECT_TRUE(q.has_value());
    EXPECT_EQ(*q, "x=1&y= ");
    EXPECT_TRUE(q->find(' ') != std::string::npos);
}

// IPv6 address parsing test
TEST(UriTest, Ipv6Parsing) {
    auto u = cpptools::net::URI("https://[2001:db8::1]:443/abc");
    EXPECT_EQ(u.to_string(), u.uri());
    EXPECT_EQ(u.hosts().size(), 1);
    EXPECT_EQ(u.hosts()[0].host, "2001:db8::1");
    EXPECT_EQ(u.hosts()[0].port, 443);

    // Test without square brackets
    auto u2 = cpptools::net::URI("tcp://2001:db8::1/path");
    EXPECT_EQ(u2.hosts().size(), 1);
    EXPECT_EQ(u2.hosts()[0].host, "2001:db8::1");
    EXPECT_FALSE(u2.hosts()[0].port.has_value());
}

// File URI parsing test
TEST(UriTest, FileUriParsing) {
    auto u = cpptools::net::URI("file:///C:/Windows/System32");
    EXPECT_EQ(u.to_string(), u.uri());
    EXPECT_EQ(u.scheme(), "file");
    EXPECT_EQ(u.path(), "/C:/Windows/System32");
}

// Error handling test
TEST(UriTest, ErrorHandling) {
    // Missing scheme
    EXPECT_THROW(cpptools::net::URI("://no-scheme"), cpptools::net::parse_error);

    // Unclosed IPv6 square brackets
    EXPECT_THROW(cpptools::net::URI("http://[::1"), cpptools::net::parse_error);

    // Port out of range
    EXPECT_THROW(cpptools::net::URI("http://host:99999/"), cpptools::net::parse_error);
}

// Performance test (not a unit test, but a separate function)
TEST(UriTest, DISABLED_Performance) {
    const int   N      = 20000;
    std::string sample = "http://user:pass@host1:80,host2:81/path/to/resource?query=1#frag";
    auto        t0     = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        auto u = cpptools::net::URI(sample);
        (void)u;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "Parsed " << N << " URIs in " << ms << " ms" << std::endl;
}
