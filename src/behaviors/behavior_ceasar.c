/*
 * Copyright (c) XXXX The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_ceasar

// Dependencies
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <dt-bindings/zmk/keys.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/split/bluetooth/service.h>
#include <zmk/hid.h>


LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define ROT DT_INST_PROP(0, rotation)

struct ceasar_state { 
    bool encryption_active;
};

static struct ceasar_state state = { .encryption_active = false } ;

static uint32_t caesar_transform(uint32_t keycode) {
    uint32_t usage = zmk_hid_get_usage_id(keycode);  // Already uint32_t

    if (usage >= HID_USAGE_KEY_A && usage <= HID_USAGE_KEY_Z) {
        uint32_t offset = usage - HID_USAGE_KEY_A;
        uint32_t rotated = (offset + ROT) % 26;
        uint32_t new_usage = HID_USAGE_KEY_A + rotated;

        return zmk_hid_keycode_from_usage(new_usage);
    }

    return keycode;
}

static int ceasar_listener(const zmk_event_t *eh) {
    if (!state.encryption_active) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (!zmk_split_bt_central()) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    struct zmk_keycode_state_changed *ev =
        as_zmk_keycode_state_changed(eh);

    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    ev->keycode = caesar_transform(ev->keycode);

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(ceasar, ceasar_listener);
ZMK_SUBSCRIPTION(ceasar, zmk_keycode_state_changed);

static void on_ceasar_binding_pressed(struct zmk_behavior_binding *binding,
                                                 struct zmk_behavior_binding_event event) {
    //toggle encryption
    state.encryption_active = !state.encryption_active;
}

// API struct
static const struct behavior_driver_api ceasar_driver_api = {
    .binding_pressed = on_ceasar_binding_pressed,
};

BEHAVIOR_DT_INST_DEFINE(0,                                                // Instance Number (0)
                        NULL,                          // Initialization Function
                        NULL,                                             // Power Management Device Pointer
                        NULL,                         // Behavior Data Pointer
                        NULL,                       // Behavior Configuration Pointer
                        POST_KERNEL, 
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,  // Initialization Level, Device Priority
                        &ceasar_driver_api);                  // API struct
                        
#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */

