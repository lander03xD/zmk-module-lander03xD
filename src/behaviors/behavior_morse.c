/*
 * Copyright (c) XXXX The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_morse

// Dependencies
#include <zephyr/device.h>
//#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <dt-bindings/zmk/keys.h>
#include <zmk/events/keycode_state_changed.h>
#include <zephyr/sys/util.h> //for ARG_UNUSED(work);
#include <string.h> // for string compare
#include <layout_swapping.h>


//LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

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
    uint32_t keycode; // HID keycode
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
    {"-----", HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS},
    {".----", HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION},
    {"..---", HID_USAGE_KEY_KEYBOARD_2_AND_AT},
    {"...--", HID_USAGE_KEY_KEYBOARD_3_AND_HASH},
    {"....-", HID_USAGE_KEY_KEYBOARD_4_AND_DOLLAR},
    {".....", HID_USAGE_KEY_KEYBOARD_5_AND_PERCENT},
    {"-....",  HID_USAGE_KEY_KEYBOARD_6_AND_CARET},
    {"--...", HID_USAGE_KEY_KEYBOARD_7_AND_AMPERSAND},
    {"---..", HID_USAGE_KEY_KEYBOARD_8_AND_ASTERISK},
    {"----.", HID_USAGE_KEY_KEYBOARD_9_AND_LEFT_PARENTHESIS},
}; 

static void emit_key(uint32_t keycode) {
    raise_zmk_keycode_state_changed_from_encoded(keycode, true, state.last_event_timestamp);
    raise_zmk_keycode_state_changed_from_encoded(keycode, false, state.last_event_timestamp); 
}

static uint32_t lookup_morse(const char *buffer, uint8_t len) {
    for (size_t i = 0; i < sizeof(morse_table)/sizeof(morse_table[0]); i++) {
        if (strncmp(morse_table[i].symbols, buffer, len) == 0 &&
            morse_table[i].symbols[len] == '\0') {
            return dvorak_to_qwerty(morse_table[i].keycode);
        }
    }
    return 0;
}
static void flush_letter_now(void) {
    if (state.count == 0) {
        return;
    }
    uint32_t keycode = lookup_morse(state.buffer, state.count);
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

static int on_morse_binding_pressed(struct zmk_behavior_binding *binding,
                                                 struct zmk_behavior_binding_event event) {
    state.last_event_timestamp = event.timestamp;
    state.press_start_time = event.timestamp;
    // Cancel pending flushes while key is pressed
    k_work_cancel_delayable(&state.flush_letter_work);
    k_work_cancel_delayable(&state.flush_word_work);
    return 0; //success
}

static int on_morse_binding_released(struct zmk_behavior_binding *binding,
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
    return 0; //success
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
                        
#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */

