# Copyright (c) XXXX The ZMK Contributors
# SPDX-License-Identifier: MIT

target_include_directories(app PRIVATE include)

if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    target_sources(app PRIVATE src/behaviors/behavior_morse.c)
    target_sources(app PRIVATE src/behaviors/behavior_ceasar.c)
endif() # ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
