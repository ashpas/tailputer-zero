/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace platform::tailscale {

/* High-level connection state surfaced to the UI. Kept coarse on purpose: the
 * UI only needs enough detail to drive a single Connect/Disconnect button
 * plus a status line, not the full `tailscale status --json` shape. */
enum class ConnState {
    Unknown = 0,
    NotInstalled,
    Stopped,
    NeedsLogin,
    Connecting,
    Connected,
    Error,
};

struct Status {
    ConnState state{ConnState::Unknown};
    std::string self_ip;
    std::string self_hostname;
    int32_t peer_count{0};
    int32_t peer_online_count{0};
    std::string error;
    /* Set only when state == NeedsLogin and `tailscale status` printed a
     * "Log in at: <url>" line -- i.e. a pending interactive login that
     * bring_up()'s own URL extraction may have missed (see tailscale_client
     * .cpp's query_status()/parse_status_text() for why this path exists). */
    std::string login_url;
};

struct UpResult {
    bool ok{false};
    bool needs_login{false};
    std::string login_url;
    std::string error;
};

/* All functions below shell out to a small fixed-verb helper script
 * (see scripts/cardputerzero-tailscale-helper.sh) via a narrowly scoped
 * sudoers NOPASSWD rule. They block on the subprocess and must be called
 * from a worker thread, never from the LVGL UI thread. See
 * docs/TAILSCALE_SETUP.md for the one-time setup this depends on. */

Status query_status();

/* Brings the tailnet connection up. If Tailscale needs interactive
 * re-authentication, UpResult.needs_login is set and login_url carries the
 * URL to render as a QR code. */
UpResult bring_up();

/* Same as bring_up(), but passes a pre-generated auth key so no interactive
 * browser approval is needed (e.g. a key scanned via the camera). */
UpResult bring_up_with_key(const std::string& auth_key);

bool bring_down(std::string& error);

/* True when the `tailscale` CLI and the helper script are both present on
 * PATH / at their fixed install locations. Does not imply the daemon is
 * running or authenticated. */
bool is_available();

struct PeerInfo {
    std::string hostname;
    std::string ip;
    bool online{false};
    /* Free-text status tail from `tailscale status`, e.g. "idle, tx 1.2KB
     * rx 3.4KB" or "active; direct 192.168.1.5:41641" or "offline". Shown
     * as-is rather than re-parsed into a path/relay enum -- good enough for
     * a read-only dashboard, and avoids reproducing tailscaled's own
     * relay-vs-direct logic. */
    std::string detail;
};

/* Lists tailnet peers (excludes self). Read-only; goes through the same
 * sudo helper as query_status() purely for consistency, since both read
 * from the same `tailscale status` text. */
std::vector<PeerInfo> query_peers();

struct NetCheckResult {
    bool ok{false};
    std::string summary;
};

/* Runs `tailscale netcheck` directly (no sudo/helper): it is a read-only
 * diagnostic that does not touch connection state, so it needs neither
 * root nor the operator grant that bring_up()/bring_down() require. Can
 * take several seconds (it probes STUN/DERP) -- call from a worker thread. */
NetCheckResult netcheck();

} // namespace platform::tailscale
