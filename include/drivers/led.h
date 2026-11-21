#pragma once
#include <Arduino.h>
#include "config/config.h"

inline void set_led(int16_t r, int16_t g, int16_t b) {
    rgbLedWrite(RGB_PIN, r, g, b);
}