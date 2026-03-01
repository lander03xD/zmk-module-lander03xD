#include <stddef.h>
#include <stdint.h>
#include <dt-bindings/zmk/keys.h>

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
            return DV_QW[i].keycode_to;
        }
    }
    return keycode;
}

uint32_t qwerty_to_dvorak(uint32_t keycode){
    for (size_t i = 0; i < sizeof(DV_QW)/sizeof(DV_QW[0]); i++){
        if (DV_QW[i].keycode_to == keycode){
            return DV_QW[i].keycode_from;
        }
    }
    return keycode;
}
