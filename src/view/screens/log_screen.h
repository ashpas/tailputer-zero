/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "base_screen.h"

namespace screen {

/* Shows recent lines of the on-device log file, so Tailscale connect/
 * disconnect activity can be checked without SSH. Scrolls via F/X (mapped
 * to up/down per the CardputerZero Key Specifications doc's documented
 * arrow-key convention) -- see handle_vertical_scroll(). Dismissed by a
 * short ESC tap (handled in app.cpp's ShortcutController, same pattern as
 * QrLoginScreen's dismissal). */
class LogScreen : public BaseScreen {
public:
    LogScreen(viewmodel::BaseViewModel& view_model, app::AssetManager& assets);

    void handle_vertical_scroll(int32_t steps) override;

private:
    void build_content(lv_obj_t* content) override;

    lv_obj_t* scroll_container_{nullptr};
};

} // namespace screen
