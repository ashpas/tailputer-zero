/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#include "base_viewmodel.h"

#include "applog.h"
#include "logger.h"
#include "tailscale_client.h"

#include <cstdio>
#include <string>
#include <thread>
#include <utility>

namespace viewmodel {
namespace {

int page_to_int(model::AppPage page) {
    return static_cast<int>(page);
}

model::TailscaleConnState from_platform(platform::tailscale::ConnState state) {
    using platform::tailscale::ConnState;
    switch (state) {
        case ConnState::NotInstalled: return model::TailscaleConnState::NotInstalled;
        case ConnState::Stopped:      return model::TailscaleConnState::Stopped;
        case ConnState::NeedsLogin:   return model::TailscaleConnState::NeedsLogin;
        case ConnState::Connecting:   return model::TailscaleConnState::Connecting;
        case ConnState::Connected:    return model::TailscaleConnState::Connected;
        case ConnState::Error:        return model::TailscaleConnState::Error;
        case ConnState::Unknown:
        default:                      return model::TailscaleConnState::Unknown;
    }
}

model::TailscaleState from_platform_status(const platform::tailscale::Status& status) {
    model::TailscaleState state;
    state.conn_state = from_platform(status.state);
    state.self_ip = status.self_ip;
    state.self_hostname = status.self_hostname;
    state.peer_count = status.peer_count;
    state.peer_online_count = status.peer_online_count;
    state.last_error = status.error;
    state.login_url = status.login_url;
    return state;
}

/* Heap-allocated payload for the lv_async_call bridge. Freed by the
 * callback that consumes it. */
struct AsyncResult {
    BaseViewModel* view_model;
    std::shared_ptr<std::atomic_bool> alive;
    model::TailscaleState state;
    bool navigate_to_qr_login;
};

void apply_async_result(void* user_data) {
    auto* result = static_cast<AsyncResult*>(user_data);
    if (result->alive->load()) {
        result->view_model->apply_tailscale_state_public(std::move(result->state),
                                                          result->navigate_to_qr_login);
    }
    delete result;
}

void post_result(BaseViewModel* view_model,
                 std::shared_ptr<std::atomic_bool> alive,
                 model::TailscaleState state,
                 bool navigate_to_qr_login) {
    auto* payload = new AsyncResult{view_model, std::move(alive), std::move(state), navigate_to_qr_login};
    if (lv_async_call(apply_async_result, payload) != LV_RESULT_OK) {
        delete payload;
    }
}

struct AsyncTextResult {
    BaseViewModel* view_model;
    std::shared_ptr<std::atomic_bool> alive;
    std::string text;
    bool is_netcheck;
};

void apply_async_text_result(void* user_data) {
    auto* result = static_cast<AsyncTextResult*>(user_data);
    if (result->alive->load()) {
        if (result->is_netcheck) {
            result->view_model->apply_netcheck_text_public(std::move(result->text));
        }
        else {
            result->view_model->apply_devices_text_public(std::move(result->text));
        }
    }
    delete result;
}

void post_text_result(BaseViewModel* view_model,
                      std::shared_ptr<std::atomic_bool> alive,
                      std::string text,
                      bool is_netcheck) {
    auto* payload = new AsyncTextResult{view_model, std::move(alive), std::move(text), is_netcheck};
    if (lv_async_call(apply_async_text_result, payload) != LV_RESULT_OK) {
        delete payload;
    }
}

} // namespace

BaseViewModel::BaseViewModel()
    : title_subject_(model_.app_title()),
      dark_mode_subject_(model_.dark_mode()),
      current_page_subject_(page_to_int(model_.current_page())),
      quit_requested_subject_(false),
      status_headline_subject_("Checking..."),
      status_detail_subject_(""),
      button_label_subject_("Connect"),
      button_enabled_subject_(false),
      busy_subject_(true),
      login_url_subject_(""),
      log_text_subject_(""),
      devices_text_subject_(""),
      netcheck_text_subject_(""),
      alive_(std::make_shared<std::atomic_bool>(true)) {
    refresh_status();
}

BaseViewModel::~BaseViewModel() {
    alive_->store(false);
}

lv_subject_t* BaseViewModel::title_subject() {
    return title_subject_.native();
}

lv_subject_t* BaseViewModel::dark_mode_subject() {
    return dark_mode_subject_.native();
}

lv_subject_t* BaseViewModel::current_page_subject() {
    return current_page_subject_.native();
}

lv_subject_t* BaseViewModel::quit_requested_subject() {
    return quit_requested_subject_.native();
}

lv_subject_t* BaseViewModel::status_headline_subject() {
    return status_headline_subject_.native();
}

lv_subject_t* BaseViewModel::status_detail_subject() {
    return status_detail_subject_.native();
}

lv_subject_t* BaseViewModel::button_label_subject() {
    return button_label_subject_.native();
}

lv_subject_t* BaseViewModel::button_enabled_subject() {
    return button_enabled_subject_.native();
}

lv_subject_t* BaseViewModel::busy_subject() {
    return busy_subject_.native();
}

lv_subject_t* BaseViewModel::login_url_subject() {
    return login_url_subject_.native();
}

lv_subject_t* BaseViewModel::log_text_subject() {
    return log_text_subject_.native();
}

lv_subject_t* BaseViewModel::devices_text_subject() {
    return devices_text_subject_.native();
}

lv_subject_t* BaseViewModel::netcheck_text_subject() {
    return netcheck_text_subject_.native();
}

bool BaseViewModel::is_dark_mode() const {
    return model_.dark_mode();
}

void BaseViewModel::set_dark_mode(bool enabled) {
    model_.set_dark_mode(enabled);
    publish_all();
}

void BaseViewModel::toggle_dark_mode() {
    model_.toggle_dark_mode();
    publish_all();
}

model::AppPage BaseViewModel::current_page() const {
    return model_.current_page();
}

void BaseViewModel::show_connect_page() {
    model_.set_current_page(model::AppPage::Connect);
    publish_all();
}

void BaseViewModel::show_qr_login_page() {
    model_.set_current_page(model::AppPage::QrLogin);
    publish_all();
}

void BaseViewModel::show_logs_page() {
    /* LogScreen's content area scrolls (F/X keys), so this isn't capped to
     * one screen's worth -- just to the subject's buffer size. */
    const auto text = platform::applog::read_tail(3900);
    log_text_subject_.set(text.c_str());
    model_.set_current_page(model::AppPage::Logs);
    publish_all();
}

void BaseViewModel::show_devices_page() {
    devices_text_subject_.set("Loading...");
    model_.set_current_page(model::AppPage::Devices);
    publish_all();

    auto* self = this;
    auto alive = alive_;
    std::thread([self, alive]() {
        const auto peers = platform::tailscale::query_peers();
        std::string text;
        if (peers.empty()) {
            text = "No peers found.\n\nEither you're alone on this tailnet, or\nTailscale is not connected.";
        }
        else {
            for (const auto& peer : peers) {
                text += (peer.online ? "● " : "○ ");
                text += peer.hostname.empty() ? peer.ip : peer.hostname;
                text += "\n    ";
                text += peer.ip;
                text += "  ";
                text += peer.detail.empty() ? (peer.online ? "idle" : "offline") : peer.detail;
                text += "\n\n";
            }
        }
        post_text_result(self, alive, std::move(text), false);
    }).detach();
}

void BaseViewModel::show_netcheck_page() {
    netcheck_text_subject_.set("Checking network...\nThis can take a few seconds.");
    model_.set_current_page(model::AppPage::NetCheck);
    publish_all();

    auto* self = this;
    auto alive = alive_;
    std::thread([self, alive]() {
        const auto result = platform::tailscale::netcheck();
        post_text_result(self, alive, result.summary, true);
    }).detach();
}

void BaseViewModel::refresh_status() {
    if (refresh_in_flight_) {
        return;
    }
    refresh_in_flight_ = true;

    auto* self = this;
    auto alive = alive_;
    std::thread([self, alive]() {
        auto status = platform::tailscale::query_status();
        LOG_DEBUG("status refresh: state={} ip={} peers={}/{}",
                 static_cast<int>(status.state),
                 status.self_ip.empty() ? "-" : status.self_ip,
                 status.peer_online_count,
                 status.peer_count);
        post_result(self, alive, from_platform_status(status), false);
    }).detach();
}

void BaseViewModel::toggle_connect() {
    const auto current_state = model_.tailscale_state().conn_state;
    const bool currently_connected = current_state == model::TailscaleConnState::Connected;
    /* Read on the UI thread, captured by value below -- last_known_login_url_
     * must never be touched from the worker thread itself. */
    const std::string cached_login_url = last_known_login_url_;
    /* Only skip bring_up()/`tailscale up` when we already got a URL from an
     * earlier attempt THIS session -- that's the specific case confirmed to
     * hang on a repeat call (see the comment below). A fresh logout with no
     * prior successful attempt has nothing pending yet at all (confirmed
     * via on-device log: `status` just says "Logged out." with no URL,
     * neither plain-text nor AuthURL, until `tailscale up` is actually
     * called at least once) -- bring_up() must still run in that case, or
     * nothing ever initiates a login. */
    const bool already_needs_login = current_state == model::TailscaleConnState::NeedsLogin &&
                                     !cached_login_url.empty();

    LOG_INFO("user requested {}", currently_connected ? "disconnect" : "connect");
    begin_busy();

    auto* self = this;
    auto alive = alive_;

    if (already_needs_login) {
        /* Confirmed via on-device log: calling `tailscale up` again while a
         * login is already pending (i.e. we already have a URL from an
         * earlier call) does not return promptly like the first call does
         * -- it hangs indefinitely (no corresponding "exec ... up" log line
         * ever appears, no matter how long you wait), so the worker thread
         * calling bring_up() here would never complete and the user would
         * be stuck forever. Skip bring_up()/`tailscale up` entirely in that
         * specific case; query_status() alone is fast, non-blocking, and
         * (confirmed in the same log) reliably returns a fresh login URL
         * each time while genuinely logged out with a login already
         * pending. */
        std::thread([self, alive, cached_login_url]() {
            const auto status = platform::tailscale::query_status();
            auto state = from_platform_status(status);
            bool navigate_to_qr_login = false;
            if (status.state == platform::tailscale::ConnState::NeedsLogin) {
                if (!status.login_url.empty()) {
                    navigate_to_qr_login = true;
                }
                else if (!cached_login_url.empty()) {
                    state.login_url = cached_login_url;
                    navigate_to_qr_login = true;
                }
            }
            post_result(self, alive, std::move(state), navigate_to_qr_login);
        }).detach();
        return;
    }

    if (currently_connected) {
        std::thread([self, alive]() {
            std::string error;
            const bool ok = platform::tailscale::bring_down(error);
            if (ok) {
                LOG_INFO("disconnect succeeded");
            }
            else {
                LOG_ERROR("disconnect failed: {}", error);
            }
            model::TailscaleState state;
            state.conn_state = ok ? model::TailscaleConnState::Stopped : model::TailscaleConnState::Error;
            state.last_error = error;
            post_result(self, alive, std::move(state), false);
        }).detach();
        return;
    }

    std::thread([self, alive, cached_login_url]() {
        const auto result = platform::tailscale::bring_up();
        model::TailscaleState state;
        bool navigate_to_qr_login = false;

        if (result.needs_login) {
            LOG_INFO("connect requires login: {}", result.login_url);
            state.conn_state = model::TailscaleConnState::NeedsLogin;
            state.login_url = result.login_url;
            navigate_to_qr_login = true;
        }
        else if (result.ok) {
            LOG_INFO("connect succeeded");
            const auto status = platform::tailscale::query_status();
            state = from_platform_status(status);
            /* bring_up() reported success without extracting a login URL
             * from `tailscale up`'s own output (result.needs_login was
             * false), but the follow-up status query can still reveal a
             * pending interactive login with its own URL -- route to the
             * QR screen here too, rather than only ever doing so from
             * bring_up()'s direct extraction. */
            if (status.state == platform::tailscale::ConnState::NeedsLogin) {
                if (!status.login_url.empty()) {
                    LOG_INFO("status query revealed pending login: {}", status.login_url);
                    navigate_to_qr_login = true;
                }
                else if (!cached_login_url.empty()) {
                    /* Neither `up` nor `status` printed a URL this time --
                     * both appear to only do so while a login is first
                     * becoming pending, not on every subsequent check
                     * while one is already pending. Reuse the last one we
                     * actually got rather than leaving the user stuck on
                     * "Needs Login" with no QR to scan. */
                    LOG_INFO("no fresh login URL; reusing last known one");
                    state.login_url = cached_login_url;
                    navigate_to_qr_login = true;
                }
            }
        }
        else {
            LOG_ERROR("connect failed: {}", result.error);
            state.conn_state = model::TailscaleConnState::Error;
            state.last_error = result.error;
        }

        post_result(self, alive, std::move(state), navigate_to_qr_login);
    }).detach();
}

void BaseViewModel::dismiss_login_prompt() {
    show_connect_page();
    refresh_status();
}

void BaseViewModel::request_quit() {
    quit_requested_subject_.set(true);
}

void BaseViewModel::publish_all() {
    title_subject_.set(model_.app_title());
    dark_mode_subject_.set(model_.dark_mode());
    current_page_subject_.set(page_to_int(model_.current_page()));
}

void BaseViewModel::begin_busy() {
    busy_subject_.set(true);
    button_enabled_subject_.set(false);
}

void BaseViewModel::apply_tailscale_state(model::TailscaleState state, bool navigate_to_qr_login) {
    refresh_in_flight_ = false;
    model_.set_tailscale_state(state);

    char detail[96] = {0};
    switch (state.conn_state) {
        case model::TailscaleConnState::Unknown:
            status_headline_subject_.set("Checking...");
            status_detail_subject_.set("");
            button_label_subject_.set("Connect");
            button_enabled_subject_.set(false);
            break;
        case model::TailscaleConnState::NotInstalled:
            status_headline_subject_.set("Not Installed");
            status_detail_subject_.set("Install Tailscale via SSH first");
            button_label_subject_.set("Retry");
            button_enabled_subject_.set(true);
            break;
        case model::TailscaleConnState::Stopped:
            status_headline_subject_.set("Not Connected");
            status_detail_subject_.set("Tap Connect to join your tailnet");
            button_label_subject_.set("Connect");
            button_enabled_subject_.set(true);
            break;
        case model::TailscaleConnState::NeedsLogin:
            /* This is shown on the Connect page itself -- there is no QR
             * code visible here, only once Connect is actually pressed and
             * the app navigates to QrLoginScreen. Saying "scan the QR code"
             * on a page with no QR code confused a real user into thinking
             * the app was broken. */
            status_headline_subject_.set("Needs Login");
            status_detail_subject_.set("Tap Connect to continue");
            button_label_subject_.set("Connect");
            button_enabled_subject_.set(true);
            break;
        case model::TailscaleConnState::Connecting:
            status_headline_subject_.set("Connecting...");
            status_detail_subject_.set("");
            button_label_subject_.set("Connecting...");
            button_enabled_subject_.set(false);
            break;
        case model::TailscaleConnState::Connected:
            status_headline_subject_.set("Connected");
            std::snprintf(detail, sizeof(detail), "%s  -  %d/%d peers online",
                         state.self_ip.empty() ? "no IP" : state.self_ip.c_str(),
                         state.peer_online_count,
                         state.peer_count);
            status_detail_subject_.set(detail);
            button_label_subject_.set("Disconnect");
            button_enabled_subject_.set(true);
            break;
        case model::TailscaleConnState::Error:
            status_headline_subject_.set("Error");
            status_detail_subject_.set(state.last_error.empty() ? "Unknown error" : state.last_error.c_str());
            button_label_subject_.set("Retry");
            button_enabled_subject_.set(true);
            break;
    }

    busy_subject_.set(false);
    if (!state.login_url.empty()) {
        last_known_login_url_ = state.login_url;
    }
    login_url_subject_.set(state.login_url.c_str());

    if (navigate_to_qr_login) {
        show_qr_login_page();
    }
    else if (state.conn_state == model::TailscaleConnState::Connected &&
             model_.current_page() == model::AppPage::QrLogin) {
        /* The periodic background status poll can detect a successful
         * login (e.g. approved from a phone) while the user is still
         * looking at the now-stale QR code. Confirmed by a real user
         * report: without this, they had to manually press ESC to leave
         * it even though the login had already succeeded. */
        LOG_INFO("login completed while QR screen was open; returning to Connect");
        show_connect_page();
    }
}

/* Thin public trampoline so the free-function async bridge above (which
 * cannot be a friend without extra ceremony) can reach the private
 * state-apply logic. */
void BaseViewModel::apply_tailscale_state_public(model::TailscaleState state, bool navigate_to_qr_login) {
    apply_tailscale_state(std::move(state), navigate_to_qr_login);
}

void BaseViewModel::apply_devices_text_public(std::string text) {
    devices_text_subject_.set(text.c_str());
}

void BaseViewModel::apply_netcheck_text_public(std::string text) {
    netcheck_text_subject_.set(text.c_str());
}

} // namespace viewmodel
