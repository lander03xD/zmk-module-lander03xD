/*
 * Copyright (c) XXXX The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_ceasar

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/split/bluetooth/service.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/hid_usage.h>
#include <layout_swapping.h>

#include <zephyr/logging/log.h> //logging
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL); //logging

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define ROT DT_INST_PROP(0, rotation)

struct noise_state { 
    uint32_t jiggle_time;
    bool noise_active;
    struct k_work_delayable jiggle_work; // no clue
};

static struct noise_state state = { 
.jiggle_time = 7
.noise_active = false };

static void jiggle_now(void) {
    //Do something like press 
    if (state.noise_active){
    LOG_DBG("NOISE: jiggle_now");    
    // do keypress
    }
}

static void jiggle(struct k_work *work) {
    ARG_UNUSED(work);
    jiggle_now();
    k_work_reschedule(&state.jiggle_work, K_MSEC(state.jiggle_time));  
}


static int on_noise_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    state.noise_active = !state.noise_active;
    LOG_DBG("NOISE: state changed");
    if(state.noise_active){    
        k_work_reschedule(&state.jiggle_work, K_MSEC(state.jiggle_time));
    } else {
        k_work_cancel_delayable(&state.jiggle_work);
    }
    return 0; //success
}

// API struct
static const struct behavior_driver_api ceasar_driver_api = {
    .binding_released = on_noise_binding_released,
};


static int noise_init(const struct device *dev) {
    ARG_UNUSED(dev);
    k_work_init_delayable(&state.jiggle_work, jiggle);
    return 0;
}

BEHAVIOR_DT_INST_DEFINE(0,
                        noise_init,
                        NULL,
                        NULL,
                        NULL,
                        POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &ceasar_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
