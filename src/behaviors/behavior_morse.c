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
#include <zephyr/sys/util.h> //for ARG_UNUSED(work);
#include <string.h> // for string compare


LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define MAX_SYMBOLS 6 
#define MORSE_TIME_UNIT DT_INST_PROP(0, time_unit_ms)

struct morse_state { 
    // morse buffer
    char buffer[MAX_SYMBOLS]; //buffer of collected dots and dashes
    uint8_t count; //amount of dots and dashes
    // timings
    int64_t press_start_time; // to compute dot vs dash duration todo not sure if this is needed
    int64_t last_event_timestamp; // safe timestamp for emitting events
    //work 
    struct k_work_delayable flush_letter_work; // no clue
    struct k_work_delayable flush_word_work; // no clue   
};

static struct morse_state state;

struct morse_mapping {
    const char *symbols;
    uint8_t keycode; // HID keycode
};

static const struct morse_mapping morse_table[] = {
    {".-", A},
    {"-...", B},
    {"-.-.", C},
    {"-..", D},
    {".", E},
    {"..-.", F},
    {"--.", G},
    {"....", H},
    {"..", I},
    {".---", J},
    {"-.-", K},
    {".-..", L},
    {"--", M},
    {"-.", N},
    {"---", O},
    {".--.", P},
    {"--.-", Q},
    {".-.", R},
    {"...", S},
    {"-", T},
    {"..-", U},
    {"...-", V},
    {".--", W},
    {"-..-", X},
    {"-.--", Y},
    {"--..", Z},
    {"-----", N0},
    {".----", N1},
    {"..---", N2},
    {"...--", N3},
    {"....-", N4},
    {".....", N5},
    {"-....",  N6},
    {"--...", N7},
    {"---..", N8},
    {"----.", N9},
}; 

static void emit_key(uint32_t keycode) {
    raise_zmk_keycode_state_changed_from_encoded(keycode, true, state.last_event_timestamp);
    raise_zmk_keycode_state_changed_from_encoded(keycode, false, state.last_event_timestamp); 
}

static uint8_t lookup_morse(const char *buffer, uint8_t len) {
    for (size_t i = 0; i < sizeof(morse_table)/sizeof(morse_table[0]); i++) {
        if (strncmp(morse_table[i].symbols, buffer, len) == 0 &&
            morse_table[i].symbols[len] == '\0') {
            return morse_table[i].keycode;
        }
    }
    return 0;
}
static void flush_letter_now(void) {
    if (state.count == 0) {
        return;
    }
    uint8_t keycode = lookup_morse(state.buffer, state.count);
    if (keycode) {
        emit_key(keycode);
    } else {
        // Invalid sequence: output literal dots/dashes
        for (uint8_t i = 0; i < state.count; i++) {
            if (state.buffer[i] == '.') {
                emit_key(KP_DOT);
            } else if (state.buffer[i] == '-') {
                emit_key(KP_MINUS);
            }
        }
    }
    state.count = 0;
}


static void flush_letter(struct k_work *work) {
    ARG_UNUSED(work);
    flush_letter_now();
    k_work_reschedule(&state.flush_word_work, K_MSEC(MORSE_TIME_UNIT * 7));
}

static void flush_word(struct k_work *work) {
    ARG_UNUSED(work);
    if (state.count == 0) {
        emit_key(SPACE);
    }
}

static void schedule_flush() {
    k_work_reschedule(&state.flush_letter_work, K_MSEC(MORSE_TIME_UNIT * 3));
}

static void on_morse_binding_pressed(struct zmk_behavior_binding *binding,
                                                 struct zmk_behavior_binding_event event) {
    state.last_event_timestamp = event.timestamp;
    state.press_start_time = event.timestamp;
    // Cancel pending flushes while key is pressed
    k_work_cancel_delayable(&state.flush_letter_work);
    k_work_cancel_delayable(&state.flush_word_work);
}

static void on_morse_binding_released(struct zmk_behavior_binding *binding,
                                                  struct zmk_behavior_binding_event event) {
    state.last_event_timestamp = event.timestamp;
    int64_t duration = event.timestamp - state.press_start_time;
    
    if (state.count < MAX_SYMBOLS) {
        if (duration < 2 * MORSE_TIME_UNIT) {
            state.buffer[state.count++] = '.';
        } else {
            state.buffer[state.count++] = '-';
        }
    } else {
        // Overflow, flush immediately as invalid sequence
        flush_letter_now();
    	k_work_cancel_delayable(&state.flush_letter_work);
    	k_work_cancel_delayable(&state.flush_word_work);
    }
    
    schedule_flush();
}

// API struct
static const struct behavior_driver_api morse_driver_api = {
    .binding_pressed = on_morse_binding_pressed,
    .binding_released = on_morse_binding_released,
};

static int morse_init(const struct device *dev) {
    ARG_UNUSED(dev);
    k_work_init_delayable(&state.flush_letter_work, flush_letter);
    k_work_init_delayable(&state.flush_word_work, flush_word);
    return 0;
}

BEHAVIOR_DT_INST_DEFINE(0,                                                // Instance Number (0)
                        morse_init,                          // Initialization Function
                        NULL,                                             // Power Management Device Pointer
                        NULL,                         // Behavior Data Pointer
                        NULL,                       // Behavior Configuration Pointer
                        POST_KERNEL, 
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,  // Initialization Level, Device Priority
                        &morse_driver_api);                  // API struct
                        
                    
//K_WORK_DELAYABLE_DEFINE(state.flush_letter_work, flush_letter); //to be removed?
//K_WORK_DELAYABLE_DEFINE(state.flush_word_work, flush_word); //to be removed?

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */

