#pragma once

#include <filesystem>
#include <system_error>

namespace cpptools {
namespace utilities {

/**
 * @brief Recursively creates a directory and all its parent directories if they don't exist
 * c++17 is required
 *
 * @details This function creates the specified directory path, including all necessary parent
 * directories. It uses the C++17 std::filesystem::create_directories function internally, which is
 * an atomic operation that either creates all directories successfully or fails completely without
 * leaving partial directories.
 *
 * @param dir The directory path to create (can be absolute or relative)
 * @return bool True if the directory was created successfully or already exists, false otherwise
 * @note
 * 1. If the directory already exists, the function returns true immediately
 * 2. The function does not throw exceptions; instead, it returns a boolean result
 * 3. This function is marked inline to avoid linker errors when included in multiple translation
 * units
 */
inline bool mkdirall(const std::string& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return !ec;
}

}  // namespace utilities
}  // namespace cpptools