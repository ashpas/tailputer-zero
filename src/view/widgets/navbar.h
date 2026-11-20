/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "base_widget.h"
#include "base_viewmodel.h"
#include "icon_button.h"

#include <array>
#include <memory>

namespace app {
class AssetManager;
}

namespace view::widgets {

/* Five-icon bar at keys 4/5/6/7/8: exit (sign-out), log viewer, light/dark
 * toggle, device list, and a Connect/Disconnect status indicator. The
 * device has no touchscreen, so the big on-screen Connect/Disconnect
 * button is not reachable by click -- but unlike the other icons, the
 * key-8 one is status-only: it has no click handler and is never
 * registered via register_nav_button. Pressing '8' (held, like
 * ESC-to-exit) is handled globally by ShortcutController in app.cpp, since
 * the action needs a long-press confirmation to avoid accidental
 * disconnects, which register_nav_button's instant-click dispatch doesn't
 * support. That icon just mirrors live state. QrLoginScreen, LogScreen,
 * DevicesScreen and NetCheckScreen are reached automatically/via this bar
 * and dismissed with ESC, so there is no page-toggle icon for QrLogin or
 * NetCheck here. */
class NavBar : public BaseWidgets {
public:
    NavBar(lv_obj_t* parent, viewmodel::BaseViewModel& view_model, app::AssetManager& assets);
    ~NavBar() override;

    void build() override;

private:
    void create_icon_buttons();
    void update_icon_buttons();

    static void toggle_theme_cb(lv_event_t* event);
    static void show_logs_cb(lv_event_t* event);
    static void show_devices_cb(lv_event_t* event);
    static void update_icons_cb(lv_observer_t* observer, lv_subject_t* subject);

    viewmodel::BaseViewModel& view_model_;
    app::AssetManager& assets_;
    std::array<std::unique_ptr<IconButton>, 5> icon_buttons_;
    lv_font_t* icon_font_{nullptr};
    lv_observer_t* theme_observer_{nullptr};
    lv_observer_t* connect_state_observer_{nullptr};
    lv_observer_t* connect_enabled_observer_{nullptr};
};

} // namespace view::widgets
