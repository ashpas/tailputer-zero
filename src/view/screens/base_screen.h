/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "navbar.h"
#include "base_viewmodel.h"
#include "titlebar.h"
#include "lvgl.h"

#include <memory>

namespace app {
class AssetManager;
}

namespace screen {

class BaseScreen {
public:
    BaseScreen(viewmodel::BaseViewModel& view_model, app::AssetManager& assets);
    virtual ~BaseScreen();

    BaseScreen(const BaseScreen&) = delete;
    BaseScreen& operator=(const BaseScreen&) = delete;

    void init();
    lv_obj_t* root() const;

    /* Default no-op: most screens have nothing to scroll. Screens with
     * content taller than the viewport (e.g. LogScreen) override this.
     * steps is in "lines", positive = scroll down/later content, negative =
     * scroll up/earlier content -- the screen decides what a line means.
     * Called directly from ShortcutController in app.cpp in response to
     * raw F/X (up/down) key events; bypasses LVGL's focus/group input
     * system entirely, consistent with every other input path in this app
     * (there is no touchscreen and no input group anywhere). */
    virtual void handle_vertical_scroll(int32_t steps);

protected:
    virtual void build_content(lv_obj_t* content) = 0;

    viewmodel::BaseViewModel& view_model();
    app::AssetManager& assets();

private:
    viewmodel::BaseViewModel& view_model_;
    app::AssetManager& assets_;
    lv_obj_t* root_{nullptr};
    lv_obj_t* content_{nullptr};
    std::unique_ptr<view::widgets::TitleBar> title_bar_;
    std::unique_ptr<view::widgets::NavBar> nav_bar_;
};

} // namespace screen
