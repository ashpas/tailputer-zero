/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "base_screen.h"

namespace screen {

/* Shown automatically when Tailscale reports it needs interactive
 * re-authentication. Renders the `tailscale up` login URL as a QR code so
 * the user can approve it from a phone instead of typing a URL on the
 * device's keyboard. */
class QrLoginScreen : public BaseScreen {
public:
    QrLoginScreen(viewmodel::BaseViewModel& view_model, app::AssetManager& assets);

private:
    void build_content(lv_obj_t* content) override;
};

} // namespace screen
