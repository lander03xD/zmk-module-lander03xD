/*
 * Copyright (c) XXXX The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_caesar

#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <dt-bindings/zmk/keys.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/split/bluetooth/service.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/hid_usage.h>  // <-- Added for HID_USAGE_KEY_A/Z

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define ROT DT_INST_PROP(0, rotation)

struct caesar_state { 
    bool encryption_active;
};

static struct caesar_state state = { .encryption_active = false };

static uint32_t caesar_transform(uint32_t keycode) {
    uint32_t usage = zmk_hid_get_usage_id(keycode);

    if (usage >= HID_USAGE_KEY_A && usage <= HID_USAGE_KEY_Z) {
        uint32_t offset = usage - HID_USAGE_KEY_A;
        uint32_t rotated = (offset + ROT) % 26;
        uint32_t new_usage = HID_USAGE_KEY_A + rotated;
        return zmk_hid_keycode_from_usage(new_usage);
    }

    return keycode;
}

static int caesar_listener(const zmk_event_t *eh) {
    if (!state.encryption_active) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (!zmk_split_bt_central()) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    const struct zmk_keycode_state_changed *ev =
        as_zmk_keycode_state_changed(eh);

    if (!ev) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // Create a modified keycode event instead of modifying the original
    struct zmk_keycode_state_changed new_ev = *ev;
    new_ev.keycode = caesar_transform(ev->keycode);

    // Emit the transformed event
    zmk_event_manager_raise(&new_ev);

    return ZMK_EV_EVENT_HANDLED;
}

ZMK_LISTENER(caesar, caesar_listener);
ZMK_SUBSCRIPTION(caesar, zmk_keycode_state_changed);

static void on_caesar_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    state.encryption_active = !state.encryption_active;
}

// API struct
static const struct behavior_driver_api caesar_driver_api = {
    .binding_pressed = on_caesar_binding_pressed,
};

BEHAVIOR_DT_INST_DEFINE(0,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &caesar_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
