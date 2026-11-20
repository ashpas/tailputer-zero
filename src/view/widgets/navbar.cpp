/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#include "navbar.h"

#include "asset_manager.h"
#include "bindings.h"
#include "linux_input.h"
#include "theme.h"
#include "ui_const.h"

#include <cstring>

namespace view::widgets {
namespace {

/* Physical-key indices match the device's fn+4 / fn+6 convention used
 * elsewhere in the app (index N maps to key '4'+N). */
constexpr size_t kQuitButtonIndex = 0;
constexpr size_t kLogsButtonIndex = 1;
constexpr size_t kThemeButtonIndex = 2;
constexpr size_t kDevicesButtonIndex = 3;
constexpr size_t kConnectButtonIndex = 4;

/* Per the CardputerZero Key Specifications doc ("Recommended Key Mapping
 * Design"): the display is physically aligned with keys 4-8 below it, and
 * the five recommended icon-center X coordinates are 49/105/161/217/273 at
 * Y=153 (absolute, 320x170 screen coordinates), icon size 22x22. We only use
 * two of the five slots (keys 4 and 6), but still place them at the exact
 * documented centers rather than relying on flex layout, which does not
 * reproduce these positions once the slot count changes. */
constexpr int32_t kIconCenterY = 153;
constexpr std::array<int32_t, 5> kIconCenterX = {49, 105, 161, 217, 273};

} // namespace

NavBar::NavBar(lv_obj_t* parent, viewmodel::BaseViewModel& view_model, app::AssetManager& assets)
    : BaseWidgets(parent), view_model_(view_model), assets_(assets) {}

NavBar::~NavBar() {
    if (icon_buttons_[1]) {
        platform::unregister_nav_button(kThemeButtonIndex, icon_buttons_[1]->root());
    }
    if (icon_buttons_[3]) {
        platform::unregister_nav_button(kLogsButtonIndex, icon_buttons_[3]->root());
    }
    if (icon_buttons_[4]) {
        platform::unregister_nav_button(kDevicesButtonIndex, icon_buttons_[4]->root());
    }
    if (theme_observer_) {
        lv_observer_remove(theme_observer_);
        theme_observer_ = nullptr;
    }
    if (connect_state_observer_) {
        lv_observer_remove(connect_state_observer_);
        connect_state_observer_ = nullptr;
    }
    if (connect_enabled_observer_) {
        lv_observer_remove(connect_enabled_observer_);
        connect_enabled_observer_ = nullptr;
    }
}

void NavBar::build() {
    if (core_obj_) {
        return;
    }

    core_obj_ = lv_obj_create(parent_);
    lv_obj_remove_style_all(core_obj_);
    lv_obj_set_size(core_obj_, LV_PCT(100), kNavBarHeight);
    lv_obj_align(core_obj_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(core_obj_, LV_OBJ_FLAG_SCROLLABLE);
    reactive::bind_theme(core_obj_, view_model_.dark_mode_subject(), reactive::ThemeRole::Bar);

    create_icon_buttons();
    platform::set_nav_shortcut_mode(false);
    theme_observer_ = reactive::observe_obj(core_obj_, view_model_.dark_mode_subject(), update_icons_cb, this);
    connect_state_observer_ = reactive::observe_obj(core_obj_, view_model_.button_label_subject(), update_icons_cb, this);
    connect_enabled_observer_ = reactive::observe_obj(core_obj_, view_model_.button_enabled_subject(), update_icons_cb, this);
    update_icon_buttons();
}

void NavBar::create_icon_buttons() {
    const auto light_color = view::palette(false).text;
    const auto dark_color = view::palette(true).text;
    icon_font_ = assets_.load_font("Phosphor-Fill.ttf", 22);

    /* kIconCenterY is an absolute (0..170) screen coordinate from the spec;
     * core_obj_ is a kNavBarHeight-tall bar pinned to the bottom of the
     * kScreenHeight-tall screen, so translate it into bar-local space. */
    constexpr int32_t kButtonWidth = 32;
    constexpr int32_t kButtonHeight = 24;
    const int32_t rel_center_y = kIconCenterY - (view::kScreenHeight - kNavBarHeight);

    const auto place_at_key = [&](IconButton& button, size_t key_index) {
        const int32_t x = kIconCenterX[key_index] - kButtonWidth / 2;
        const int32_t y = rel_center_y - kButtonHeight / 2;
        lv_obj_align(button.root(), LV_ALIGN_TOP_LEFT, x, y);
    };

    for (auto& button : icon_buttons_) {
        button = std::make_unique<IconButton>(core_obj_,
                                              view_model_,
                                              kButtonWidth,
                                              kButtonHeight,
                                              "",
                                              icon_font_ ? icon_font_ : &lv_font_montserrat_14,
                                              light_color,
                                              dark_color,
                                              nullptr,
                                              &view_model_,
                                              true);
        button->build();
    }

    /* Status-only icon, like the Connect/Disconnect one: quitting is a
     * long-press action owned globally by ShortcutController (every page,
     * not just Connect -- see the bug note in app.cpp's handle_key), so
     * this icon has no click handler and is never registered as a nav
     * button. A plain instant-click dispatch here would bypass the
     * hold-to-confirm safety entirely. */
    icon_buttons_[0]->set_text(ICON_SIGN_OUT);
    icon_buttons_[0]->set_enabled(true);
    place_at_key(*icon_buttons_[0], kQuitButtonIndex);

    lv_obj_add_event_cb(icon_buttons_[1]->root(), toggle_theme_cb, LV_EVENT_CLICKED, &view_model_);
    icon_buttons_[1]->set_enabled(true);
    place_at_key(*icon_buttons_[1], kThemeButtonIndex);
    platform::register_nav_button(kThemeButtonIndex, icon_buttons_[1]->root());

    /* Status-only icon: Connect/Disconnect is now a long-press action owned
     * by ShortcutController (see app.cpp), not a click dispatched via
     * register_nav_button, so this icon has no click handler and is never
     * registered as a nav button. It just reflects current state. */
    place_at_key(*icon_buttons_[2], kConnectButtonIndex);

    icon_buttons_[3]->set_text(ICON_INFO);
    lv_obj_add_event_cb(icon_buttons_[3]->root(), show_logs_cb, LV_EVENT_CLICKED, &view_model_);
    icon_buttons_[3]->set_enabled(true);
    place_at_key(*icon_buttons_[3], kLogsButtonIndex);
    platform::register_nav_button(kLogsButtonIndex, icon_buttons_[3]->root());

    icon_buttons_[4]->set_text(ICON_ETHERNET);
    lv_obj_add_event_cb(icon_buttons_[4]->root(), show_devices_cb, LV_EVENT_CLICKED, &view_model_);
    icon_buttons_[4]->set_enabled(true);
    place_at_key(*icon_buttons_[4], kDevicesButtonIndex);
    platform::register_nav_button(kDevicesButtonIndex, icon_buttons_[4]->root());
}

void NavBar::update_icon_buttons() {
    const bool is_dark_mode = view_model_.is_dark_mode();
    icon_buttons_[1]->set_text(is_dark_mode ? ICON_SUN : ICON_MOON);

    const char* label = lv_subject_get_string(view_model_.button_label_subject());
    const bool is_connected = label && std::strcmp(label, "Disconnect") == 0;
    icon_buttons_[2]->set_text(is_connected ? ICON_WIFI_HIGH : ICON_WIFI_NONE);
    icon_buttons_[2]->set_enabled(lv_subject_get_int(view_model_.button_enabled_subject()) != 0);
}

void NavBar::toggle_theme_cb(lv_event_t* event) {
    auto* view_model = static_cast<viewmodel::BaseViewModel*>(lv_event_get_user_data(event));
    if (view_model) {
        view_model->toggle_dark_mode();
    }
}

void NavBar::show_logs_cb(lv_event_t* event) {
    auto* view_model = static_cast<viewmodel::BaseViewModel*>(lv_event_get_user_data(event));
    if (view_model) {
        view_model->show_logs_page();
    }
}

void NavBar::show_devices_cb(lv_event_t* event) {
    auto* view_model = static_cast<viewmodel::BaseViewModel*>(lv_event_get_user_data(event));
    if (view_model) {
        view_model->show_devices_page();
    }
}

void NavBar::update_icons_cb(lv_observer_t* observer, lv_subject_t* subject) {
    LV_UNUSED(subject);

    auto* nav_bar = static_cast<NavBar*>(lv_observer_get_user_data(observer));
    if (nav_bar) {
        nav_bar->update_icon_buttons();
    }
}

} // namespace view::widgets
