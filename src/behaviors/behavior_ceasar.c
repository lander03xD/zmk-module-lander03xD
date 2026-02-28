/*
 * Copyright (c) XXXX The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_ceasar

#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <dt-bindings/zmk/keys.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/split/bluetooth/service.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/hid_usage.h>  // <-- Added for HID_USAGE_KEYBOARD_A/Z

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define ROT DT_INST_PROP(0, rotation)

struct ceasar_state { 
    bool encryption_active;
};

static struct ceasar_state state = { .encryption_active = false };

static uint32_t ceasar_transform(uint32_t keycode) {
    uint32_t usage = keycode;// zmk_hid_get_usage_id(keycode);

    if (usage >= HID_USAGE_KEY_KEYBOARD_A && usage <= HID_USAGE_KEY_KEYBOARD_Z) {
        uint32_t offset = usage - HID_USAGE_KEY_KEYBOARD_A;
        uint32_t rotated = (offset + ROT) % 26;
        uint32_t new_usage = HID_USAGE_KEY_KEYBOARD_A + rotated;
        return new_usage;//zmk_hid_keycode_from_usage(new_usage);
    }

    return keycode;
}

static int ceasar_listener(const zmk_event_t *eh) {
    //If state is not active pass to next listener
    if (!state.encryption_active) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    //If event gets handled on non-central side pass to next listener
    //if (!zmk_split_bt_central()) {
    //    return ZMK_EV_EVENT_BUBBLE;
    //}

    //Check if event is a keycode state change
    const struct zmk_keycode_state_changed *ev =
        as_zmk_keycode_state_changed(eh);

    //If not the correct event, pass it to the next listener
    if (!ev) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // Create a modified keycode event instead of modifying the original
    struct zmk_keycode_state_changed new_ev = *ev;
    new_ev.keycode = ceasar_transform(ev->keycode);

    // Emit the transformed event
    zmk_event_manager_raise(&new_ev);
    
    raise_zmk_keycode_state_changed_from_encoded(C, true, event.timestamp);
    raise_zmk_keycode_state_changed_from_encoded(C, false, event.timestamp); 


    return ZMK_EV_EVENT_HANDLED;
}

ZMK_LISTENER(ceasar, ceasar_listener);
ZMK_SUBSCRIPTION(ceasar, zmk_keycode_state_changed);

static void on_ceasar_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    //testing code
    if (state.encryption_active){
        raise_zmk_keycode_state_changed_from_encoded(A, true, event.timestamp);
        raise_zmk_keycode_state_changed_from_encoded(A, false, event.timestamp); 
    } else {
        raise_zmk_keycode_state_changed_from_encoded(B, true, event.timestamp);
        raise_zmk_keycode_state_changed_from_encoded(B, false, event.timestamp); 
    }
    state.encryption_active = !state.encryption_active;
}

// API struct
static const struct behavior_driver_api ceasar_driver_api = {
    .binding_pressed = on_ceasar_binding_pressed,
};

BEHAVIOR_DT_INST_DEFINE(0,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &ceasar_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
