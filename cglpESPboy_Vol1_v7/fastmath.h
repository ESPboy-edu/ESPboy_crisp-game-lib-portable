#ifndef FASTMATH_H
#define FASTMATH_H

#include <Arduino.h>
#define RAD_TO_INDEX 162.97466f

extern const float sinLUT[1024] PROGMEM;

static inline float fast_sin(float radians) {
    int32_t index32 = (int32_t)(radians * RAD_TO_INDEX);
    uint16_t safe_index = index32 & 1023;
    return pgm_read_float(&sinLUT[safe_index]);
}

static inline float fast_cos(float radians) {
    int32_t index32 = (int32_t)(radians * RAD_TO_INDEX) + 256;
    uint16_t safe_index = index32 & 1023;
    return pgm_read_float(&sinLUT[safe_index]);
}

#endif
