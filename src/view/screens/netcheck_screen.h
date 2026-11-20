/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "base_screen.h"

namespace screen {

/* Raw `tailscale netcheck` output (UDP/IPv4/IPv6 availability, NAT type,
 * nearest DERP region and latency). Reached from DevicesScreen via Enter;
 * dismissed by a short ESC tap back to Devices. Scrolls via F/X like
 * LogScreen/DevicesScreen. */
class NetCheckScreen : public BaseScreen {
public:
    NetCheckScreen(viewmodel::BaseViewModel& view_model, app::AssetManager& assets);

    void handle_vertical_scroll(int32_t steps) override;

private:
    void build_content(lv_obj_t* content) override;

    lv_obj_t* scroll_container_{nullptr};
};

} // namespace screen
