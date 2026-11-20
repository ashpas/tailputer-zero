/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "base_screen.h"

namespace screen {

/* The app's single primary screen: one big Connect/Disconnect button plus a
 * status line, modeled after the Android Tailscale app's one-tap UX. */
class ConnectScreen : public BaseScreen {
public:
    ConnectScreen(viewmodel::BaseViewModel& view_model, app::AssetManager& assets);
    ~ConnectScreen() override;

private:
    void build_content(lv_obj_t* content) override;

    lv_timer_t* refresh_timer_{nullptr};
};

} // namespace screen
