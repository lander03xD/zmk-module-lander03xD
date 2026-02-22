/*
 * Copyright (c) XXXX The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_morse

// Dependencies
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <dt-bindings/zmk/keys.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);


#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

// Instance-specific Data struct (Optional)
struct behavior_morse_data { //not used
    bool data_param1;
    bool data_param2;
    bool data_param3;
};

// Instance-specific Config struct (Optional)
struct behavior_morse_config { //not used
    bool config_param1;
    bool config_param2;
    bool config_param3;
};

// Initialization Function (Optional)
static int morse_init(const struct device *dev) {
    return 0;
};

static int on_morse_binding_pressed(struct zmk_behavior_binding *binding,
                                                 struct zmk_behavior_binding_event event) {
    return raise_zmk_keycode_state_changed_from_encoded(A, true, event.timestamp);
}

static int on_morse_binding_released(struct zmk_behavior_binding *binding,
                                                  struct zmk_behavior_binding_event event) {
    return  raise_zmk_keycode_state_changed_from_encoded(B, true, event.timestamp);;
}

// API struct
static const struct behavior_driver_api morse_driver_api = {
    .binding_pressed = on_morse_binding_pressed,
    .binding_released = on_morse_binding_released,
};

BEHAVIOR_DT_INST_DEFINE(0,                                                // Instance Number (0)
                        morse_init,                          // Initialization Function
                        NULL,                                             // Power Management Device Pointer
                        NULL,                         // Behavior Data Pointer
                        NULL,                       // Behavior Configuration Pointer
                        POST_KERNEL, 
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,  // Initialization Level, Device Priority
                        &morse_driver_api);                  // API struct

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */

