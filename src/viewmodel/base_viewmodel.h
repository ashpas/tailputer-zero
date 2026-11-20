/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "subjects.h"
#include "base_model.h"

#include "lvgl.h"

#include <atomic>
#include <memory>
#include <string>

namespace viewmodel {

class BaseViewModel {
public:
    BaseViewModel();
    ~BaseViewModel();

    BaseViewModel(const BaseViewModel&) = delete;
    BaseViewModel& operator=(const BaseViewModel&) = delete;

    lv_subject_t* title_subject();
    lv_subject_t* dark_mode_subject();
    lv_subject_t* current_page_subject();
    lv_subject_t* quit_requested_subject();

    /* Tailscale-facing subjects consumed by ConnectScreen / QrLoginScreen. */
    lv_subject_t* status_headline_subject();  // "Connected" / "Not Connected" / "Needs Login" / ...
    lv_subject_t* status_detail_subject();    // IP + peer summary, or an error line
    lv_subject_t* button_label_subject();     // "Connect" / "Disconnect" / "Retry"
    lv_subject_t* button_enabled_subject();
    lv_subject_t* busy_subject();
    lv_subject_t* login_url_subject();
    lv_subject_t* log_text_subject();
    lv_subject_t* devices_text_subject();
    lv_subject_t* netcheck_text_subject();

    bool is_dark_mode() const;
    void set_dark_mode(bool enabled);
    void toggle_dark_mode();

    model::AppPage current_page() const;
    void show_connect_page();
    void show_qr_login_page();

    /* Reads the on-device log file's tail into log_text_subject() and
     * switches to the Logs page. Synchronous (a few KB local file read is
     * fast enough not to need the worker-thread/lv_async_call machinery
     * used for Tailscale calls). */
    void show_logs_page();

    /* Queries tailnet peers on a worker thread and switches to the Devices
     * page once the result is back; shows "Loading..." in the meantime.
     * Safe to call repeatedly -- requests are not coalesced like
     * refresh_status() since this is user-navigation-triggered, not a
     * background poll. */
    void show_devices_page();

    /* Runs `tailscale netcheck` on a worker thread (it can take several
     * seconds) and switches to the NetCheck page once the result is back. */
    void show_netcheck_page();

    /* Starts an async status refresh; safe to call repeatedly, calls are
     * coalesced while one is already in flight. */
    void refresh_status();

    /* Primary one-tap action: connects if disconnected, disconnects if
     * connected. This is the only action ConnectScreen's main button needs
     * to know about. */
    void toggle_connect();

    /* Called from QrLoginScreen when the user dismisses the QR code without
     * completing login (the underlying `tailscale up` process is left to
     * finish or time out on its own -- see known limitations in
     * docs/TAILSCALE_SETUP.md). */
    void dismiss_login_prompt();

    void request_quit();

    /* Public trampoline used only by the internal async-result bridge in
     * base_viewmodel.cpp to reach apply_tailscale_state() from a
     * free function. Not part of the intended external API. */
    void apply_tailscale_state_public(model::TailscaleState state, bool navigate_to_qr_login);

    /* Same kind of trampoline as apply_tailscale_state_public(), for the
     * Devices/NetCheck worker-thread results. */
    void apply_devices_text_public(std::string text);
    void apply_netcheck_text_public(std::string text);

private:
    void publish_all();
    void apply_tailscale_state(model::TailscaleState state, bool navigate_to_qr_login);
    void begin_busy();

    model::BaseModel model_;
    reactive::StringSubject<32> title_subject_;
    reactive::BoolSubject dark_mode_subject_;
    reactive::IntSubject current_page_subject_;
    reactive::BoolSubject quit_requested_subject_;

    reactive::StringSubject<32> status_headline_subject_;
    reactive::StringSubject<96> status_detail_subject_;
    reactive::StringSubject<16> button_label_subject_;
    reactive::BoolSubject button_enabled_subject_;
    reactive::BoolSubject busy_subject_;
    reactive::StringSubject<320> login_url_subject_;
    reactive::StringSubject<4000> log_text_subject_;
    reactive::StringSubject<2000> devices_text_subject_;
    reactive::StringSubject<3000> netcheck_text_subject_;

    /* Shared with any in-flight worker thread so it can detect that this
     * view model (and the LVGL instance it talks to) has been destroyed
     * before it tries to marshal a result back via lv_async_call. */
    std::shared_ptr<std::atomic_bool> alive_;
    bool refresh_in_flight_{false};

    /* The most recent non-empty login URL we've shown. `tailscale up`/
     * `status` only seem to print "Log in at: <url>" while a login is
     * first becoming pending -- a retry while one is already pending (e.g.
     * the user hit Connect again after missing the first scan) can come
     * back without the URL at all. Reusing the cached one beats silently
     * showing "Needs Login" with no QR to actually scan. */
    std::string last_known_login_url_;
};

} // namespace viewmodel
