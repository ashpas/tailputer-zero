/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>
#include <string>

namespace model {

enum class AppPage {
    Connect = 0,
    QrLogin = 1,
    Logs = 2,
    Devices = 3,
    NetCheck = 4,
};

/* Mirrors platform::tailscale::ConnState but kept free of any platform
 * include so the model stays dependency-light. */
enum class TailscaleConnState {
    Unknown = 0,
    NotInstalled,
    Stopped,
    NeedsLogin,
    Connecting,
    Connected,
    Error,
};

struct TailscaleState {
    TailscaleConnState conn_state{TailscaleConnState::Unknown};
    std::string self_ip;
    std::string self_hostname;
    int32_t peer_count{0};
    int32_t peer_online_count{0};
    std::string login_url;
    std::string last_error;
    bool busy{false};
};

class BaseModel {
public:
    const char* app_title() const;

    bool dark_mode() const;
    void set_dark_mode(bool enabled);
    void toggle_dark_mode();

    AppPage current_page() const;
    void set_current_page(AppPage page);

    const TailscaleState& tailscale_state() const;
    void set_tailscale_state(TailscaleState state);

private:
    bool dark_mode_ = false;
    AppPage current_page_ = AppPage::Connect;
    TailscaleState tailscale_state_{};
};

} // namespace model
