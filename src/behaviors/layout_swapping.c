#include <stddef.h>
#include <stdint.h>
#include <dt-bindings/zmk/keys.h>

#include <zephyr/logging/log.h> //logging
LOG_MODULE_REGISTER(my_behavior, LOG_LEVEL_DBG); //logging

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
    {V, PERIOD},
    {W, COMMA},
    {X, B},
    {Y, T},
    {Z, SLASH},
};

uint32_t dvorak_to_qwerty(uint32_t keycode){
    for (size_t i = 0; i < sizeof(DV_QW)/sizeof(DV_QW[0]); i++){
        if (DV_QW[i].keycode_from == keycode){
            LOG_DBG("Keycode mapped from: 0x%08" PRIx32, keycode);
            LOG_DBG("Keycode mapped to: 0x%08" PRIx32, DV_QW[i].keycode_to);
            return DV_QW[i].keycode_to;
        }
    }
    LOG_DBG("Keycode not mapped: 0x%08" PRIx32, keycode);
    return keycode;
}

uint32_t qwerty_to_dvorak(uint32_t keycode){
    for (size_t i = 0; i < sizeof(DV_QW)/sizeof(DV_QW[0]); i++){
        if (DV_QW[i].keycode_to == keycode){
            LOG_DBG("Keycode mapped from: 0x%08" PRIx32, keycode);
            LOG_DBG("Keycode mapped to: 0x%08" PRIx32, DV_QW[i].keycode_from);
            return DV_QW[i].keycode_from;
        }
    }
    LOG_DBG("Keycode not mapped: 0x%08" PRIx32, keycode);
    return keycode;
}
