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
    bool last_event_state;
};
struct layout_mapping {
    uint32_t keycode_from;
    uint32_t keycode_to;
};

static const struct layout_mapping DV_QW[] = {
    {A, A},
    {B, N},
    {C, I},
    {D, H},
    {E, D},
    {F, Y},
    {G, U},
    {H, J},
    {I, G},
    {J, C},
    {K, V},
    {L, P},
    {M, M},
    {N, L},
    {O, S},
    {P, R},
    {Q, X},
    {R, O},
    {S, SEMICOLON}, 
    {T, K},
    {U, F},
    {V, DOT}, 
    {W, COMMA},   
    {X, B},
    {Y, T},
    {Z, SLASH} 
};

static uint32_t dvorak_to_qwerty(uint32_t keycode){
	for (size_t i = 0; i < sizeof(DV_QW)/sizeof(DV_QW[0]); i++){
	    if (DV_QW[i].keycode_from = keycode){
	        return DV_QW[i].keycode_to;
	    }
	}
	return keycode;
}

static uint32_t qwerty_to_dvorak(uint32_t keycode){
	for (size_t i = 0; i < sizeof(DV_QW)/sizeof(DV_QW[0]); i++){
	    if (DV_QW[i].keycode_to = keycode){
	        return DV_QW[i].keycode_from;
	    }
	}
	return keycode;
}

static struct ceasar_state state = { 
.encryption_active = false,
.last_event_state = false };

static uint32_t ceasar_transform(uint32_t keycode) {

    uint32_t usage = dvorak_to_qwerty(keycode);

    if (usage >= HID_USAGE_KEY_KEYBOARD_A && usage <= HID_USAGE_KEY_KEYBOARD_Z) {
        uint32_t offset = usage - HID_USAGE_KEY_KEYBOARD_A;
        uint32_t rotated = (offset + ROT) % 26;
        uint32_t new_usage = HID_USAGE_KEY_KEYBOARD_A + rotated;
        return dvorak_to_qwerty(new_usage);
    }

    return keycode;
}

static int ceasar_listener(const zmk_event_t *eh) {
    //If state is not active pass to next listener
    if (!state.encryption_active) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    //Check if event is a keycode state change
    const struct zmk_keycode_state_changed *ev =
        as_zmk_keycode_state_changed(eh);

    //If not the correct event, pass it to the next listener
    if (!ev) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    
    if (ev->state == state.last_event_state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    uint32_t new_keycode = ceasar_transform(ev->keycode);
    state.last_event_state = ev->state;
    raise_zmk_keycode_state_changed_from_encoded(new_keycode, ev->state, ev->timestamp);

    return ZMK_EV_EVENT_HANDLED;
}

ZMK_LISTENER(ceasar, ceasar_listener);
ZMK_SUBSCRIPTION(ceasar, zmk_keycode_state_changed);

static void on_ceasar_binding_released(struct zmk_behavior_binding *binding,
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
    .binding_released = on_ceasar_binding_released,
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
