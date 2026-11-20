/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef>
#include <string>

namespace platform::applog {

/* $XDG_STATE_HOME/tailputer-zero/tailputer-zero.log, falling back to
 * ~/.local/state/tailputer-zero/tailputer-zero.log. */
std::string log_file_path();

#if !USE_DESKTOP
/* Redirects stdout/stderr (and therefore every Logger:: call) to
 * log_file_path(), truncating the file first if it has grown past a fixed
 * cap. Device builds only -- desktop keeps console output for dev
 * convenience. Call once, early in Application::run(). */
void setup_persistent_log();
#endif

/* Reads up to the last max_bytes of the log file for display in-app (the
 * LogScreen). Returns a short human-readable message instead of file
 * contents if the log doesn't exist yet or can't be opened -- this is shown
 * directly in the UI, not just logged, so it must read as a real message. */
std::string read_tail(std::size_t max_bytes);

} // namespace platform::applog
