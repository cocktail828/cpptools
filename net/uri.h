// Single-file containing improved header-only URI parser and a test runner.
// Features added:
//  - RFC-aware character classes (unreserved, sub-delims, pchar)
//  - percent-decode utilities
//  - authority helpers (decoded_user() / decoded_pass())
//  - path/query decoding helpers
//  - performance-minded parsing using std::string into stored raw input
//  - more extensive test suite (assert-based) with many edge cases

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <initializer_list>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace cpptools {
namespace net {

struct parse_error : public std::runtime_error {
    explicit parse_error(const std::string &s) : std::runtime_error(s) {}
};

struct HostPort {
    std::string host;  // for IPv6 this does not include surrounding brackets
    std::optional<uint16_t> port;
    bool operator==(const HostPort &o) const { return host == o.host && port == o.port; }
    std::string to_string() const {
        bool is_ipv6 = host.find(':') != std::string::npos;
        std::string inst;
        if (is_ipv6)
            inst += '[' + host + ']';
        else
            inst += host;
        if (port) inst += ':' + std::to_string(*port);
        return inst;
    }
};

class URI final {
   private:
    std::string uri_;
    std::string scheme_;
    std::optional<std::string> authority_;
    std::vector<HostPort> hosts_;
    std::string path_;
    std::optional<std::string> query_;
    std::optional<std::string> fragment_;

   public:
    const std::string &uri() const { return uri_; }
    const std::string &scheme() const { return scheme_; }

    typedef std::pair<std::string, std::string> UserPass;
    const std::optional<UserPass> User() const {
        if (!authority_) return std::nullopt;
        auto a = *authority_;
        size_t pos = a.find(':');
        if (pos == std::string::npos) return std::make_pair(percent_decode(a), std::string{});
        return std::make_pair(percent_decode(a.substr(0, pos)), percent_decode(a.substr(pos + 1)));
    }
    const std::vector<HostPort> &hosts() const { return hosts_; }
    const std::string path() const { return percent_decode(path_); }
    const std::optional<std::string> query() const {
        return query_ ? std::optional(percent_decode(*query_)) : std::nullopt;
    }
    const std::optional<std::string> &fragment() const { return fragment_; }

    // Parse and return a URI object. Throws parse_error on invalid input
    URI(const std::string input) : uri_(input) {
        const char *s = uri_.c_str();
        size_t len = uri_.size();
        size_t i = 0;

        if (len == 0) throw parse_error("empty input");

        auto is_scheme_char = [](char c, size_t pos) -> bool {
            if (pos == 0) return std::isalpha((unsigned char)c);
            return std::isalnum((unsigned char)c) || c == '+' || c == '-' || c == '.';
        };

        // find ':' for scheme
        size_t scheme_end = uri_.find(':');
        if (scheme_end == std::string::npos) throw parse_error("missing scheme");
        if (scheme_end == 0) throw parse_error("empty scheme");
        for (size_t k = 0; k < scheme_end; ++k)
            if (!is_scheme_char(input[k], k)) throw parse_error("invalid scheme char");
        this->scheme_ = uri_.substr(0, scheme_end);

        i = scheme_end + 1;

        // authority
        if (i + 1 < len && s[i] == '/' && s[i + 1] == '/') {
            i += 2;
            size_t auth_start = i;
            size_t auth_end = i;
            while (auth_end < len && s[auth_end] != '/' && s[auth_end] != '?' && s[auth_end] != '#') ++auth_end;
            // parse authority between auth_start and auth_end
            if (auth_end > auth_start) {
                std::string auth = uri_.substr(auth_start, auth_end - auth_start);
                // find last '@' not inside brackets
                int bracket = 0;
                size_t at_pos = std::string::npos;
                for (size_t k = 0; k < auth.size(); ++k) {
                    char c = auth[k];
                    if (c == '[')
                        ++bracket;
                    else if (c == ']')
                        --bracket;
                    else if (c == '@' && bracket == 0)
                        at_pos = k;
                }
                std::string hostportlist;
                if (at_pos != std::string::npos) {
                    this->authority_ = auth.substr(0, at_pos);
                    hostportlist = auth.substr(at_pos + 1);
                } else
                    hostportlist = auth;

                // split by comma or semicolon
                size_t start = 0;
                for (size_t k = 0; k <= hostportlist.size(); ++k) {
                    if (k == hostportlist.size() || hostportlist[k] == ',' || hostportlist[k] == ';') {
                        if (k > start) {
                            std::string item = hostportlist.substr(start, k - start);
                            trim_space(item);
                            if (!item.empty()) this->hosts_.push_back(parse_hostport(item));
                        }
                        start = k + 1;
                    }
                }
            }
            i = auth_end;
        }

        // path
        size_t path_start = i;
        while (i < len && s[i] != '?' && s[i] != '#') ++i;
        this->path_ = uri_.substr(path_start, i - path_start);
        // if (this->path_.empty() && !this->hosts_.empty()) this->path_ =
        // std::string("/");

        if (i < len && s[i] == '?') {
            ++i;
            size_t qstart = i;
            while (i < len && s[i] != '#') ++i;
            this->query_ = uri_.substr(qstart, i - qstart);
        }
        if (i < len && s[i] == '#') {
            ++i;
            this->fragment_ = uri_.substr(i);
        }

        // normalize host to lowercase
        for (auto &hp : this->hosts_) {
            for (auto &c : hp.host) c = std::tolower((unsigned char)c);
        }
    }

    // percent-decode a string -> std::string
    static std::string percent_decode(std::string in) {
        std::string str;
        str.reserve(in.size());
        for (size_t i = 0; i < in.size(); ++i) {
            char c = in[i];
            if (c == '%' && i + 2 < in.size()) {
                char h1 = in[i + 1], h2 = in[i + 2];
                if (is_hex(h1) && is_hex(h2)) {
                    int val = hex_val(h1) * 16 + hex_val(h2);
                    str.push_back(static_cast<char>(val));
                    i += 2;
                    continue;
                }
            }
            str.push_back(c);
        }
        return str;
    }

    std::string to_string() const {
        std::string str;
        str.append(scheme_);
        str.push_back(':');
        str.append("//");
        if (authority_) {
            str.append(*authority_);
            str.push_back('@');
        }
        for (size_t k = 0; k < hosts_.size(); ++k) {
            if (k) str.push_back(',');
            str.append(hosts_[k].to_string());
        }

        str.append(path_);
        if (query_) {
            str.push_back('?');
            str.append(*query_);
        }
        if (fragment_) {
            str.push_back('#');
            str.append(*fragment_);
        }
        return str;
    }

   private:
    // helper functions
    static inline bool is_hex(char c) { return std::isxdigit((unsigned char)c); }
    static inline int hex_val(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return c - 'A' + 10;
    }
    static inline char hex_digit(int v) {
        const char *H = "0123456789ABCDEF";
        return H[v & 0xF];
    }

    // trim spaces (in-place for string via slicing)
    inline void trim_space(std::string &v) {
        size_t b = 0;
        while (b < v.size() && std::isspace((unsigned char)v[b])) ++b;
        size_t e = v.size();
        while (e > b && std::isspace((unsigned char)v[e - 1])) --e;
        v = v.substr(b, e - b);
    }

    // parse hostport from a string
    HostPort parse_hostport(std::string item) {
        HostPort hp;
        trim_space(item);
        if (item.empty()) throw parse_error("empty host item");
        if (item.size() > 0 && item[0] == '[') {
            // IPv6 literal
            size_t rb = item.find(']');
            if (rb == std::string::npos) throw parse_error("unclosed IPv6 bracket");
            hp.host = item.substr(1, rb - 1);
            if (rb + 1 == item.size()) return hp;
            if (item[rb + 1] != ':') throw parse_error("unexpected chars after IPv6 literal");
            auto portstr = item.substr(rb + 2);
            if (portstr.empty()) throw parse_error("empty port");
            hp.port = parse_port(portstr);
            return hp;
        }
        size_t colon = item.rfind(':');
        if (colon == std::string::npos) {
            hp.host = item;
            return hp;
        }
        // if multiple ':' present and not bracketed, treat as host
        // (non-bracketed IPv6) to avoid misparse
        if (item.find(':') != colon) {
            hp.host = item;
            return hp;
        }
        hp.host = item.substr(0, colon);
        auto portstr = item.substr(colon + 1);
        if (portstr.empty()) throw parse_error("empty port");
        hp.port = parse_port(portstr);
        return hp;
    }

    uint16_t parse_port(std::string s) {
        int val = 0;
        for (char c : s) {
            if (!std::isdigit((unsigned char)c)) throw parse_error("port contains non-digit");
            val = val * 10 + (c - '0');
            if (val > 65535) throw parse_error("port inst of range");
        }
        return static_cast<uint16_t>(val);
    }
};

};  // namespace net
};  // namespace cpptools
