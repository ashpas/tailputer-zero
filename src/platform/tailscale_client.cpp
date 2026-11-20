/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#include "tailscale_client.h"

#include "logger.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifndef APP_TAILSCALE_HELPER_PATH
#define APP_TAILSCALE_HELPER_PATH "/usr/local/bin/cardputerzero-tailscale-helper"
#endif

namespace platform::tailscale {
namespace {

/* The whole point of the helper script + sudoers setup in
 * docs/TAILSCALE_SETUP.md is that these calls run through a narrowly
 * scoped `sudo -n` -- without this prefix the helper runs as the plain
 * logged-in user and `tailscale up`/`down` are correctly refused by
 * Tailscale itself ("Access denied: prefs write access denied"). `-n`
 * (non-interactive) means a missing/misconfigured sudoers rule fails fast
 * with a clear error instead of hanging on a password prompt with no tty. */
std::string sudo_helper_command(const std::string& args) {
    return std::string("sudo -n ") + APP_TAILSCALE_HELPER_PATH + " " + args;
}

/* Runs a fixed command line (no untrusted interpolation) and returns
 * (exit_ok, combined_stdout_stderr). Blocking; callers must be off the LVGL
 * thread. */
std::pair<bool, std::string> run_capture(const std::string& command) {
    std::string full_command = command + " 2>&1";
    std::array<char, 512> buffer{};
    std::string output;

    FILE* pipe = popen(full_command.c_str(), "r");
    if (!pipe) {
        return {false, "failed to start helper process"};
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }

    const int rc = pclose(pipe);
    LOG_DEBUG("exec: {} -> rc={} output={}", command, rc, output.empty() ? "<empty>" : output);
    return {rc == 0, output};
}

bool command_exists(const char* path_or_name) {
    const std::string probe = std::string("command -v '") + path_or_name + "' >/dev/null 2>&1";
    return std::system(probe.c_str()) == 0;
}

/* `tailscale status` (plain text) lines look like:
 *   100.64.1.2   my-device   user@example.com   linux   -
 *   100.64.1.3   nas0        user@example.com   linux   idle, tx 123B rx 456B
 * The very first non-empty line is Self; everything after is a peer. A
 * logged-out/stopped daemon instead prints a one-line message such as
 * "Tailscale is stopped." or "Logged out." with no IP-shaped first token. */
std::string extract_login_url(const std::string& text) {
    static const std::regex url_regex(R"(https://login\.tailscale\.com/a/\S+)");
    std::smatch match;
    if (std::regex_search(text, match, url_regex)) {
        return match.str();
    }
    return {};
}

/* `tailscale up`/`status` (plain text) only seem to print "Log in at:
 * <url>" at the moment a login first becomes pending, not on every
 * subsequent check while one is already pending -- which leaves nothing
 * to extract on a retry. `tailscale status --json`'s top-level "AuthURL"
 * field is documented to stay populated for the full duration a login is
 * pending (it's how Tailscale's own official GUI clients implement this
 * exact "show the pending login link" UX reliably), so use that
 * specifically as a second attempt when the plain-text path comes up
 * empty. Deliberately not a general JSON parser -- just this one
 * well-known top-level string field. Read-only, no sudo needed, same as
 * netcheck(). */
std::string query_pending_auth_url() {
    const auto [ok, output] = run_capture("tailscale status --json");
    (void)ok; // the command can exit non-zero while logged out; AuthURL may still be present
    static const std::regex auth_url_regex(R"RGX("AuthURL"\s*:\s*"([^"]*)")RGX");
    std::smatch match;
    if (std::regex_search(output, match, auth_url_regex) && match.size() > 1 && !match[1].str().empty()) {
        return match[1].str();
    }
    return {};
}

Status parse_status_text(const std::string& text) {
    Status status;
    std::istringstream stream(text);
    std::string line;
    bool first_line = true;
    const std::regex ip_regex(R"(^(\d{1,3}\.){3}\d{1,3}$)");

    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream fields(line);
        std::string first_token;
        fields >> first_token;

        if (first_line) {
            first_line = false;
            if (!std::regex_match(first_token, ip_regex)) {
                /* Not a peer table at all: stopped/logged-out/needs-login message. */
                if (line.find("Logged out") != std::string::npos ||
                    line.find("NeedsLogin") != std::string::npos) {
                    status.state = ConnState::NeedsLogin;
                    /* `tailscale status` on a logged-out device with a
                     * pending interactive login prints a second line like
                     * "Log in at: https://login.tailscale.com/a/...". Search
                     * the whole text (not just this line), since it comes
                     * after the one we're looking at. */
                    status.login_url = extract_login_url(text);
                }
                else if (line.find("stopped") != std::string::npos) {
                    status.state = ConnState::Stopped;
                }
                else {
                    status.state = ConnState::Error;
                    status.error = line;
                }
                return status;
            }

            status.self_ip = first_token;
            fields >> status.self_hostname;
            status.state = ConnState::Connected;
            continue;
        }

        if (!std::regex_match(first_token, ip_regex)) {
            continue;
        }
        status.peer_count += 1;

        std::string rest((std::istreambuf_iterator<char>(fields)), std::istreambuf_iterator<char>());
        if (rest.find("offline") == std::string::npos) {
            status.peer_online_count += 1;
        }
    }

    return status;
}

/* Peer lines look like:
 *   100.64.1.3   nas0        user@example.com   linux   idle, tx 123B rx 456B
 *   100.64.1.4   laptop      user@example.com   linux   offline
 * Mirrors parse_status_text()'s line-skipping rules but collects every
 * non-self row instead of just counting them. */
std::vector<PeerInfo> parse_peers_text(const std::string& text) {
    std::vector<PeerInfo> peers;
    std::istringstream stream(text);
    std::string line;
    bool first_line = true;
    const std::regex ip_regex(R"(^(\d{1,3}\.){3}\d{1,3}$)");

    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream fields(line);
        std::string ip;
        fields >> ip;

        if (!std::regex_match(ip, ip_regex)) {
            continue;
        }
        if (first_line) {
            first_line = false;
            continue; // self
        }

        PeerInfo peer;
        peer.ip = ip;
        fields >> peer.hostname;
        std::string user;
        std::string os;
        fields >> user >> os;
        std::string rest((std::istreambuf_iterator<char>(fields)), std::istreambuf_iterator<char>());
        while (!rest.empty() && rest.front() == ' ') {
            rest.erase(rest.begin());
        }
        peer.detail = rest;
        peer.online = rest.find("offline") == std::string::npos;
        peers.push_back(std::move(peer));
    }

    return peers;
}

} // namespace

bool is_available() {
    return command_exists("tailscale") && command_exists(APP_TAILSCALE_HELPER_PATH);
}

Status query_status() {
    if (!command_exists("tailscale")) {
        return Status{ConnState::NotInstalled, {}, {}, 0, 0, "tailscale is not installed"};
    }

    const auto [ok, output] = run_capture(sudo_helper_command("status"));
    if (output.empty()) {
        Status status;
        status.state = ConnState::Error;
        status.error = "status helper failed";
        return status;
    }

    /* `tailscale status` exits non-zero when logged out or stopped -- that
     * is not itself evidence of a real error, just of a state
     * parse_status_text() already recognizes (NeedsLogin/Stopped) from the
     * message text. Parse first; only treat a non-zero exit as a hard
     * error when parsing couldn't explain it, otherwise a legitimate
     * "logged out, here's your login URL" status gets misreported as a
     * generic error with the URL buried in unparsed text instead of
     * routing to the QR login screen. */
    Status status = parse_status_text(output);
    if (!ok && status.state != ConnState::NeedsLogin && status.state != ConnState::Stopped) {
        status.state = ConnState::Error;
        status.error = output;
    }
    if (status.state == ConnState::NeedsLogin && status.login_url.empty()) {
        status.login_url = query_pending_auth_url();
    }
    return status;
}

UpResult bring_up() {
    UpResult result;
    if (!is_available()) {
        result.error = "tailscale or the helper script is not installed";
        return result;
    }

    const auto [ok, output] = run_capture(sudo_helper_command("up"));
    std::string login_url = extract_login_url(output);
    if (login_url.empty()) {
        /* `tailscale up`'s own text output doesn't always carry the URL
         * (see query_pending_auth_url()'s comment) -- check the reliable
         * JSON field before giving up. */
        login_url = query_pending_auth_url();
    }
    if (!login_url.empty()) {
        result.needs_login = true;
        result.login_url = login_url;
        result.ok = true;
        return result;
    }

    result.ok = ok;
    if (!ok) {
        result.error = output;
    }
    return result;
}

UpResult bring_up_with_key(const std::string& auth_key) {
    UpResult result;
    if (!is_available()) {
        result.error = "tailscale or the helper script is not installed";
        return result;
    }
    if (auth_key.empty()) {
        result.error = "empty auth key";
        return result;
    }

    /* The key is passed as a single argv token to a fixed-verb helper script
     * that only accepts "up-with-key <key>" — it never reaches a shell, so
     * there is no injection surface. It is still visible to other local
     * users via /proc/<pid>/cmdline for the life of the helper process;
     * piping it over stdin instead is a documented follow-up hardening
     * step (see docs/TAILSCALE_SETUP.md). */
    const std::string command = sudo_helper_command("up-with-key '" + auth_key + "'");
    const auto [ok, output] = run_capture(command);
    result.ok = ok;
    if (!ok) {
        result.error = output;
    }
    return result;
}

std::vector<PeerInfo> query_peers() {
    if (!command_exists("tailscale")) {
        return {};
    }
    const auto [ok, output] = run_capture(sudo_helper_command("status"));
    if (!ok && output.find("Tailscale") == std::string::npos) {
        return {};
    }
    return parse_peers_text(output);
}

NetCheckResult netcheck() {
    NetCheckResult result;
    if (!command_exists("tailscale")) {
        result.summary = "tailscale is not installed";
        return result;
    }

    /* Read-only diagnostic, no sudo/helper needed -- see the declaration
     * in tailscale_client.h. */
    const auto [ok, output] = run_capture("tailscale netcheck");
    result.ok = ok;
    result.summary = output.empty() ? "no output" : output;
    return result;
}

bool bring_down(std::string& error) {
    if (!is_available()) {
        error = "tailscale or the helper script is not installed";
        return false;
    }

    const auto [ok, output] = run_capture(sudo_helper_command("down"));
    if (!ok) {
        error = output;
    }
    return ok;
}

} // namespace platform::tailscale
