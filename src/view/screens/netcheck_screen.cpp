/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#include "netcheck_screen.h"

#include "asset_manager.h"
#include "bindings.h"

namespace screen {

NetCheckScreen::NetCheckScreen(viewmodel::BaseViewModel& view_model, app::AssetManager& assets)
    : BaseScreen(view_model, assets) {
    init();
}

void NetCheckScreen::build_content(lv_obj_t* content) {
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    auto* title = lv_label_create(content);
    lv_label_set_text(title, "Network Check");
    auto* title_font = assets().load_standard_font(13);
    lv_obj_set_style_text_font(title, title_font ? title_font : &lv_font_montserrat_12, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 4, 2);
    reactive::bind_theme(title, view_model().dark_mode_subject(), reactive::ThemeRole::Text);

    auto* hint = lv_label_create(content);
    lv_label_set_text(hint, "ESC: back");
    auto* hint_font = assets().load_standard_font(11, app::StandardFontWeight::Regular);
    lv_obj_set_style_text_font(hint, hint_font ? hint_font : &lv_font_montserrat_10, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_RIGHT, -4, 3);
    reactive::bind_theme(hint, view_model().dark_mode_subject(), reactive::ThemeRole::Text);
    lv_obj_set_style_opa(hint, LV_OPA_60, 0);

    scroll_container_ = lv_obj_create(content);
    lv_obj_remove_style_all(scroll_container_);
    lv_obj_set_size(scroll_container_, 314, 68);
    lv_obj_align(scroll_container_, LV_ALIGN_TOP_LEFT, 3, 20);
    lv_obj_add_flag(scroll_container_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(scroll_container_, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(scroll_container_, LV_SCROLLBAR_MODE_AUTO);

    auto* netcheck_label = lv_label_create(scroll_container_);
    lv_label_bind_text(netcheck_label, view_model().netcheck_text_subject(), nullptr);
    lv_label_set_long_mode(netcheck_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(netcheck_label, 300);
    auto* netcheck_font = assets().load_font("JetBrainsMono-Regular.ttf", 10);
    if (!netcheck_font) {
        netcheck_font = assets().load_standard_font(10, app::StandardFontWeight::Regular);
    }
    lv_obj_set_style_text_font(netcheck_label, netcheck_font ? netcheck_font : &lv_font_montserrat_10, 0);
    reactive::bind_theme(netcheck_label, view_model().dark_mode_subject(), reactive::ThemeRole::Text);
}

void NetCheckScreen::handle_vertical_scroll(int32_t steps) {
    if (!scroll_container_) {
        return;
    }
    constexpr int32_t kPixelsPerStep = 14;
    lv_obj_scroll_by(scroll_container_, 0, -steps * kPixelsPerStep, LV_ANIM_ON);
}

} // namespace screen
