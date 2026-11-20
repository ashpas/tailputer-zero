/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#include "applog.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace platform::applog {

std::string log_file_path() {
    if (const char* xdg_state_home = std::getenv("XDG_STATE_HOME")) {
        const std::filesystem::path root(xdg_state_home);
        if (!root.empty() && root.is_absolute()) {
            return (root / "tailputer-zero" / "tailputer-zero.log").string();
        }
    }
    if (const char* home = std::getenv("HOME")) {
        const std::filesystem::path root(home);
        if (!root.empty() && root.is_absolute()) {
            return (root / ".local" / "state" / "tailputer-zero" / "tailputer-zero.log").string();
        }
    }
    return "tailputer-zero.log";
}

#if !USE_DESKTOP
void setup_persistent_log() {
    const std::string path = log_file_path();
    std::error_code ec;
    const std::filesystem::path log_path(path);
    if (log_path.has_parent_path()) {
        std::filesystem::create_directories(log_path.parent_path(), ec);
    }

    constexpr std::uintmax_t kMaxLogBytes = 1 * 1024 * 1024;
    if (std::filesystem::exists(log_path, ec) &&
        std::filesystem::file_size(log_path, ec) > kMaxLogBytes) {
        std::filesystem::remove(log_path, ec);
    }

    if (!std::freopen(path.c_str(), "a", stdout)) {
        return;
    }
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    if (std::FILE* err = std::freopen(path.c_str(), "a", stderr)) {
        std::setvbuf(err, nullptr, _IOLBF, 0);
    }
}
#endif

std::string read_tail(std::size_t max_bytes) {
    const std::string path = log_file_path();
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return "No log entries yet.";
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return "Could not open log file: " + path;
    }

    const auto size = static_cast<std::size_t>(file.tellg());
    const std::size_t to_read = size < max_bytes ? size : max_bytes;
    file.seekg(static_cast<std::streamoff>(size - to_read));

    std::string content(to_read, '\0');
    file.read(content.data(), static_cast<std::streamsize>(to_read));

    if (size > max_bytes) {
        const auto first_newline = content.find('\n');
        if (first_newline != std::string::npos) {
            content.erase(0, first_newline + 1);
        }
        content = "...(truncated, showing most recent entries)...\n" + content;
    }

    if (content.empty()) {
        return "Log file is empty.";
    }
    return content;
}

} // namespace platform::applog
