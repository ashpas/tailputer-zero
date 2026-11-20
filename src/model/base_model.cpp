/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#include "base_model.h"

#include <utility>

namespace model {

const char* BaseModel::app_title() const {
    return "Tailputer Zero";
}

bool BaseModel::dark_mode() const {
    return dark_mode_;
}

void BaseModel::set_dark_mode(bool enabled) {
    dark_mode_ = enabled;
}

void BaseModel::toggle_dark_mode() {
    dark_mode_ = !dark_mode_;
}

AppPage BaseModel::current_page() const {
    return current_page_;
}

void BaseModel::set_current_page(AppPage page) {
    current_page_ = page;
}

const TailscaleState& BaseModel::tailscale_state() const {
    return tailscale_state_;
}

void BaseModel::set_tailscale_state(TailscaleState state) {
    tailscale_state_ = std::move(state);
}

} // namespace model
