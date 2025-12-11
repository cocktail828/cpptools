# URI Parser (Header-only) Project

This project contains a modern C++20 header-only URI parser supporting:
- RFC 3986 semantics
- Percent-encoding/decoding
- User/password decoding
- IPv6 support
- Zero-copy parsing using string_view
- GoogleTest validation suite

## Important Note

**Current Limitation**: The standard Boost.URL and other existing URI parser implementations do not support multi-host mode natively.
However, many microservices like etcd, MongoDB, and other distributed systems require multi-host functionality for connection strings.

**This implementation attempts to provide some basic support for parsing URIs with multiple hosts.**

## About Contributing
Don’t reinvent the wheel!
If you have feature suggestions or improvements, please open a Pull Request — we’d love to collaborate and grow this library together! 

## Build

```bash
cmake -B build/
cmake --build build
./build/uri_parser_tests
```