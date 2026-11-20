/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "screen_manager.h"

#include "connect_screen.h"
#include "devices_screen.h"
#include "log_screen.h"
#include "netcheck_screen.h"
#include "qr_login_screen.h"

namespace app {
namespace {

void flush_requested_page_async(void* user_data) {
    auto* manager = static_cast<ScreenManager*>(user_data);
    if (manager) {
        manager->flush_requested_page();
    }
}

void current_page_observer(lv_observer_t* observer, lv_subject_t* subject) {
    auto* manager = static_cast<ScreenManager*>(lv_observer_get_target(observer));
    if (!manager) {
        return;
    }

    manager->request_page(static_cast<model::AppPage>(lv_subject_get_int(subject)));
}

} // namespace

ScreenManager::ScreenManager(viewmodel::BaseViewModel& view_model, AssetManager& assets)
    : view_model_(view_model), assets_(assets) {}

ScreenManager::~ScreenManager() {
    lv_async_call_cancel(flush_requested_page_async, this);

    if (page_observer_) {
        lv_observer_remove(page_observer_);
        page_observer_ = nullptr;
    }
}

void ScreenManager::start() {
    if (page_observer_) {
        return;
    }

    page_observer_ = lv_subject_add_observer_with_target(view_model_.current_page_subject(),
                                                         current_page_observer,
                                                         this,
                                                         nullptr);
}

void ScreenManager::show_connect_screen() {
    load_screen(std::make_unique<screen::ConnectScreen>(view_model_, assets_));
    loaded_page_ = model::AppPage::Connect;
    has_loaded_page_ = true;
}

void ScreenManager::show_qr_login_screen() {
    load_screen(std::make_unique<screen::QrLoginScreen>(view_model_, assets_));
    loaded_page_ = model::AppPage::QrLogin;
    has_loaded_page_ = true;
}

void ScreenManager::show_logs_screen() {
    load_screen(std::make_unique<screen::LogScreen>(view_model_, assets_));
    loaded_page_ = model::AppPage::Logs;
    has_loaded_page_ = true;
}

void ScreenManager::show_devices_screen() {
    load_screen(std::make_unique<screen::DevicesScreen>(view_model_, assets_));
    loaded_page_ = model::AppPage::Devices;
    has_loaded_page_ = true;
}

void ScreenManager::show_netcheck_screen() {
    load_screen(std::make_unique<screen::NetCheckScreen>(view_model_, assets_));
    loaded_page_ = model::AppPage::NetCheck;
    has_loaded_page_ = true;
}

void ScreenManager::request_page(model::AppPage page) {
    requested_page_ = page;

    if (page_switch_scheduled_) {
        return;
    }

    page_switch_scheduled_ = lv_async_call(flush_requested_page_async, this) == LV_RESULT_OK;
}

void ScreenManager::flush_requested_page() {
    page_switch_scheduled_ = false;

    if (has_loaded_page_ && requested_page_ == loaded_page_) {
        return;
    }

    if (requested_page_ == model::AppPage::QrLogin) {
        show_qr_login_screen();
        return;
    }
    if (requested_page_ == model::AppPage::Logs) {
        show_logs_screen();
        return;
    }
    if (requested_page_ == model::AppPage::Devices) {
        show_devices_screen();
        return;
    }
    if (requested_page_ == model::AppPage::NetCheck) {
        show_netcheck_screen();
        return;
    }

    show_connect_screen();
}

lv_obj_t* ScreenManager::current_screen() const {
    return current_screen_ ? current_screen_->root() : nullptr;
}

void ScreenManager::scroll_current_screen(int32_t steps) {
    if (current_screen_) {
        current_screen_->handle_vertical_scroll(steps);
    }
}

void ScreenManager::load_screen(std::unique_ptr<screen::BaseScreen> next_screen) {
    if (!next_screen || !next_screen->root()) {
        return;
    }

    lv_screen_load(next_screen->root());
    current_screen_ = std::move(next_screen);
}

} // namespace app
