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
#include <dt-bindings/zmk/hid_usage.h>
#include <zmk/events/keycode_state_changed.h>


LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define MAX_SYMBOLS 6 

struct morse_state { 
    char buffer[MAX_SYMBOLS]; //buffer of collected dots and dashes
    uint8_t count; //amount of dots and dashes
    int64_t last_release_time; 
    struct k_work_delayable flush_letter_work; // no clue
    struct k_work_delayable flush_word_work; // no clue   
};

static struct morse_state state;

struct morse_mapping {
    const char *symbols;
    uint8_t keycode; // HID keycode
};


struct morse_mapping {
    const char *symbols;
    uint8_t keycode; // HID keycode
};

static const struct morse_mapping morse_table[] = {
    {".-", HID_USAGE_KEY_KEYBOARD_A},
    {"-...", HID_USAGE_KEY_KEYBOARD_B},
    {"-.-.", HID_USAGE_KEY_KEYBOARD_C},
    {"-..", HID_USAGE_KEY_KEYBOARD_D},
    {".", HID_USAGE_KEY_KEYBOARD_E},
    {"..-.", HID_USAGE_KEY_KEYBOARD_F},
    {"--.", HID_USAGE_KEY_KEYBOARD_G},
    {"....", HID_USAGE_KEY_KEYBOARD_H},
    {"..", HID_USAGE_KEY_KEYBOARD_I},
    {".---", HID_USAGE_KEY_KEYBOARD_J},
    {"-.-", HID_USAGE_KEY_KEYBOARD_K},
    {".-..", HID_USAGE_KEY_KEYBOARD_L},
    {"--", HID_USAGE_KEY_KEYBOARD_M},
    {"-.", HID_USAGE_KEY_KEYBOARD_N},
    {"---", HID_USAGE_KEY_KEYBOARD_O},
    {".--.", HID_USAGE_KEY_KEYBOARD_P},
    {"--.-", HID_USAGE_KEY_KEYBOARD_Q},
    {".-.", HID_USAGE_KEY_KEYBOARD_R},
    {"...", HID_USAGE_KEY_KEYBOARD_S},
    {"-", HID_USAGE_KEY_KEYBOARD_T},
    {"..-", HID_USAGE_KEY_KEYBOARD_U},
    {"...-", HID_USAGE_KEY_KEYBOARD_V},
    {".--", HID_USAGE_KEY_KEYBOARD_W},
    {"-..-", HID_USAGE_KEY_KEYBOARD_X},
    {"-.--", HID_USAGE_KEY_KEYBOARD_Y},
    {"--..", HID_USAGE_KEY_KEYBOARD_Z},
    {"-----", HID_USAGE_KEY_KEYBOARD_0},
    {".----", HID_USAGE_KEY_KEYBOARD_1},
    {"..---", HID_USAGE_KEY_KEYBOARD_2},
    {"...--", HID_USAGE_KEY_KEYBOARD_3},
    {"....-", HID_USAGE_KEY_KEYBOARD_4},
    {".....", HID_USAGE_KEY_KEYBOARD_5},
    {"-....", HID_USAGE_KEY_KEYBOARD_6},
    {"--...", HID_USAGE_KEY_KEYBOARD_7},
    {"---..", HID_USAGE_KEY_KEYBOARD_8},
    {"----.", HID_USAGE_KEY_KEYBOARD_9},
}; 


static uint8_t lookup_morse(const char *buffer, uint8_t len) {
    for (size_t i = 0; i < sizeof(morse_table)/sizeof(morse_table[0]); i++) {
        if (strncmp(morse_table[i].symbols, buffer, len) == 0 &&
            morse_table[i].symbols[len] == '\0') {
            return morse_table[i].keycode;
        }
    }
    return 0; // 0 means not found
}

static void flush_letter(struct k_work *work) {
    if (state.count == 0) {
        return;
    }

    uint8_t keycode = lookup_morse(state.buffer, state.count);
    if (keycode) {
        zmk_hid_keypress(keycode); //todo this is wrong code, this doesn't exist in zmk
        zmk_hid_keyrelease(keycode); //todo this is wrong code, this doesn't exist in zmk
    } else {
        // Invalid sequence: output literal dots/dashes
        for (uint8_t i = 0; i < state.count; i++) {
            if (state.buffer[i] == '.') {
                zmk_hid_keypress(HID_USAGE_KEY_KEYBOARD_PERIOD);
                zmk_hid_keyrelease(HID_USAGE_KEY_KEYBOARD_PERIOD);
            } else if (state.buffer[i] == '-') {
                zmk_hid_keypress(HID_USAGE_KEY_KEYBOARD_MINUS);
                zmk_hid_keyrelease(HID_USAGE_KEY_KEYBOARD_MINUS);
            }
        }
    }

    state.count = 0;
}

static void flush_word(struct k_work *work) {
    if (state.count == 0) {
        zmk_hid_keypress(HID_USAGE_KEY_KEYBOARD_SPACEBAR);
        zmk_hid_keyrelease(HID_USAGE_KEY_KEYBOARD_SPACEBAR);
    }
}

static void schedule_flush_letter(int64_t unit_ms) {
    k_work_reschedule(&state.flush_letter_work, K_MSEC(unit_ms * 3)); //todo check k_work_reschedule
    k_work_reschedule(&state.flush_word_work, K_MSEC(unit_ms * 7));
}

static void on_morse_binding_pressed(struct zmk_behavior_binding *binding,
                                                 struct zmk_behavior_binding_event event) {
    // Cancel pending flushes while key is pressed
    k_work_cancel_delayable(&state.flush_letter_work);
    k_work_cancel_delayable(&state.flush_word_work);

    state.last_release_time = event.timestamp;
}

static void on_morse_binding_released(struct zmk_behavior_binding *binding,
                                                  struct zmk_behavior_binding_event event) {
    const uint32_t unit = DT_INST_PROP(0, time_unit_ms); //todo WTF does this do?
    int64_t duration = event.timestamp - state.last_release_time; 

    if (state.count < MAX_SYMBOLS) {
        if (duration < 2 * unit) {
            state.buffer[state.count++] = '.';
        } else {
            state.buffer[state.count++] = '-';
        }
    } else {
        // Overflow, flush immediately as invalid sequence
        flush_letter(NULL); //todo why does the flush_letter have a unit_ms?
    }

    schedule_flush_letter(unit);
}

// API struct
static const struct behavior_driver_api morse_driver_api = {
    .binding_pressed = on_morse_binding_pressed,
    .binding_released = on_morse_binding_released,
};

BEHAVIOR_DT_INST_DEFINE(0,                                                // Instance Number (0)
                        NULL,                          // Initialization Function
                        NULL,                                             // Power Management Device Pointer
                        NULL,                         // Behavior Data Pointer
                        NULL,                       // Behavior Configuration Pointer
                        POST_KERNEL, 
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,  // Initialization Level, Device Priority
                        &morse_driver_api);                  // API struct
                        
K_WORK_DELAYABLE_DEFINE(state.flush_letter_work, flush_letter);
K_WORK_DELAYABLE_DEFINE(state.flush_word_work, flush_word);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */

