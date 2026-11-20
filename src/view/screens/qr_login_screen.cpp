/*
 * SPDX-FileCopyrightText: 2026 Ashish
 *
 * SPDX-License-Identifier: MIT
 */

#include "qr_login_screen.h"

#include "asset_manager.h"
#include "bindings.h"

#include <cstring>

namespace screen {
namespace {

void login_url_observer(lv_observer_t* observer, lv_subject_t* subject) {
    auto* qrcode = lv_observer_get_target_obj(observer);
    if (!qrcode) {
        return;
    }

    const char* url = lv_subject_get_string(subject);
    if (url && std::strlen(url) > 0) {
        lv_qrcode_update(qrcode, url, static_cast<uint32_t>(std::strlen(url)));
        lv_obj_remove_flag(qrcode, LV_OBJ_FLAG_HIDDEN);
    }
    else {
        lv_obj_add_flag(qrcode, LV_OBJ_FLAG_HIDDEN);
    }
}

} // namespace

QrLoginScreen::QrLoginScreen(viewmodel::BaseViewModel& view_model, app::AssetManager& assets)
    : BaseScreen(view_model, assets) {
    init();
}

void QrLoginScreen::build_content(lv_obj_t* content) {
    /* The content area between the title bar and nav bar is only ~110px
     * tall. The previous flex-column layout (title + 100px QR + subtitle +
     * a Cancel button) needed ~184px, so it overflowed -- the QR itself
     * was being clipped top and bottom, which breaks scanning outright
     * (lost finder-pattern/timing rows), confirmed by a real photo showing
     * dark bars cutting into the QR. Dropped the title, subtitle, and
     * Cancel button entirely (ESC already dismisses this screen -- see
     * ShortcutController's handle_release in app.cpp -- and the Help
     * popup's QrLogin rows already say exactly that plus "scan with your
     * camera"), so the QR can render at its full, unclipped, maximum size
     * for the best scan odds. */
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    constexpr int32_t kQrSize = 104;
    auto* qrcode = lv_qrcode_create(content);
    lv_obj_set_size(qrcode, kQrSize, kQrSize);
    lv_qrcode_set_dark_color(qrcode, lv_color_black());
    lv_qrcode_set_light_color(qrcode, lv_color_white());
    lv_obj_center(qrcode);
    lv_subject_add_observer_obj(view_model().login_url_subject(), login_url_observer, qrcode, nullptr);
    const char* initial_url = lv_subject_get_string(view_model().login_url_subject());
    if (initial_url && std::strlen(initial_url) > 0) {
        lv_qrcode_update(qrcode, initial_url, static_cast<uint32_t>(std::strlen(initial_url)));
    }
}

} // namespace screen
