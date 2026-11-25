#pragma once

#include <cstdint>

// Decodes a BlurHash into width * height RGB565 pixels, row by row, in the byte order LVGL draws
// on this firmware. False, with pixels untouched, for a malformed hash or an empty image.
bool decodeBlurHashRgb565(const char* hash, uint32_t width, uint32_t height, uint16_t* pixels);
