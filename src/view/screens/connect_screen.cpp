/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#include "connect_screen.h"

#include "asset_manager.h"
#include "bindings.h"

namespace screen {
namespace {

void connect_button_clicked(lv_event_t* event) {
    auto* view_model = static_cast<viewmodel::BaseViewModel*>(lv_event_get_user_data(event));
    if (view_model) {
        view_model->toggle_connect();
    }
}

void refresh_timer_cb(lv_timer_t* timer) {
    auto* view_model = static_cast<viewmodel::BaseViewModel*>(lv_timer_get_user_data(timer));
    if (view_model) {
        view_model->refresh_status();
    }
}

} // namespace

ConnectScreen::ConnectScreen(viewmodel::BaseViewModel& view_model, app::AssetManager& assets)
    : BaseScreen(view_model, assets) {
    init();
}

ConnectScreen::~ConnectScreen() {
    if (refresh_timer_) {
        lv_timer_delete(refresh_timer_);
        refresh_timer_ = nullptr;
    }
}

void ConnectScreen::build_content(lv_obj_t* content) {
    auto* group = lv_obj_create(content);
    lv_obj_remove_style_all(group);
    lv_obj_set_size(group, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(group, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(group, 6, 0);
    lv_obj_clear_flag(group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(group);

    auto* headline = lv_label_create(group);
    lv_label_bind_text(headline, view_model().status_headline_subject(), nullptr);
    auto* headline_font = assets().load_standard_font(22);
    lv_obj_set_style_text_font(headline, headline_font ? headline_font : &lv_font_montserrat_20, 0);
    reactive::bind_theme(headline, view_model().dark_mode_subject(), reactive::ThemeRole::Text);

    auto* detail = lv_label_create(group);
    lv_label_bind_text(detail, view_model().status_detail_subject(), nullptr);
    lv_obj_set_width(detail, 260);
    lv_label_set_long_mode(detail, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_align(detail, LV_TEXT_ALIGN_CENTER, 0);
    auto* detail_font = assets().load_standard_font(12, app::StandardFontWeight::Regular);
    lv_obj_set_style_text_font(detail, detail_font ? detail_font : &lv_font_montserrat_12, 0);
    reactive::bind_theme(detail, view_model().dark_mode_subject(), reactive::ThemeRole::Text);

    auto* spinner = lv_spinner_create(group);
    lv_obj_set_size(spinner, 24, 24);
    reactive::bind_flag(spinner, view_model().busy_subject(), LV_OBJ_FLAG_HIDDEN, 0, reactive::Compare::Equal);

    auto* button = lv_button_create(group);
    lv_obj_set_size(button, 150, 48);
    lv_obj_set_style_radius(button, 24, 0);
    lv_obj_add_event_cb(button, connect_button_clicked, LV_EVENT_CLICKED, &view_model());
    reactive::bind_state(button, view_model().button_enabled_subject(), LV_STATE_DISABLED, 0, reactive::Compare::Equal);

    auto* button_label = lv_label_create(button);
    lv_label_bind_text(button_label, view_model().button_label_subject(), nullptr);
    lv_obj_center(button_label);
    auto* button_font = assets().load_standard_font(16);
    lv_obj_set_style_text_font(button_label, button_font ? button_font : &lv_font_montserrat_16, 0);

    refresh_timer_ = lv_timer_create(refresh_timer_cb, 4000, &view_model());

    view_model().refresh_status();

    auto* version_label = lv_label_create(content);
    lv_label_set_text(version_label, "v" APP_VERSION);
    auto* version_font = assets().load_standard_font(10, app::StandardFontWeight::Regular);
    lv_obj_set_style_text_font(version_label, version_font ? version_font : &lv_font_montserrat_10, 0);
    lv_obj_align(version_label, LV_ALIGN_BOTTOM_RIGHT, -4, -2);
    reactive::bind_theme(version_label, view_model().dark_mode_subject(), reactive::ThemeRole::Text);
    lv_obj_set_style_opa(version_label, LV_OPA_50, 0);
}

} // namespace screen
