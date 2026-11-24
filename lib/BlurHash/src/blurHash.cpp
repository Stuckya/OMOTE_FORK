// Ported from the reference C decoder at https://github.com/woltapp/blurhash (C/decode.c),
// with cosine and sRGB lookup tables in place of per-pixel cos() and pow().
//
// Copyright (c) 2018 Wolt Enterprises
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "blurHash.h"

#include <cmath>
#include <cstring>
#include <vector>

namespace {

const char BASE83[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz#$%*+,-.:;=?@[]^_{|}~";
const int BASE83_RADIX = 83;
const int MAX_COMPONENTS = 9;
const int AC_LEVELS = 19;
const float AC_CENTER = 9.0f;
const float MAX_VALUE_SCALE = 166.0f;
const size_t SIZE_AND_MAX_CHARS = 2;
const size_t DC_CHARS = 4;
const size_t AC_CHARS = 2;
const float PI = 3.14159265358979f;
const int LINEAR_STEPS = 4095;

struct Color {
  float r;
  float g;
  float b;
};

int decode83(const char* text, size_t length) {
  int value = 0;
  for (size_t i = 0; i < length; i++) {
    const char* digit = strchr(BASE83, text[i]);
    if (text[i] == '\0' || digit == nullptr) {
      return -1;
    }
    value = value * BASE83_RADIX + (int)(digit - BASE83);
  }
  return value;
}

float srgbToLinear(int value) {
  static float table[256];
  static bool built = false;
  if (!built) {
    for (int i = 0; i < 256; i++) {
      const float v = i / 255.0f;
      table[i] = v <= 0.04045f ? v / 12.92f : powf((v + 0.055f) / 1.055f, 2.4f);
    }
    built = true;
  }
  return table[value & 0xFF];
}

uint8_t linearToSrgb(float value) {
  static uint8_t table[LINEAR_STEPS + 1];
  static bool built = false;
  if (!built) {
    for (int i = 0; i <= LINEAR_STEPS; i++) {
      const float v = (float)i / LINEAR_STEPS;
      const float srgb = v <= 0.0031308f ? v * 12.92f : 1.055f * powf(v, 1 / 2.4f) - 0.055f;
      table[i] = (uint8_t)(srgb * 255 + 0.5f);
    }
    built = true;
  }
  const float clamped = value < 0 ? 0 : (value > 1 ? 1 : value);
  return table[(int)(clamped * LINEAR_STEPS + 0.5f)];
}

float signedSquare(float value) {
  return copysignf(value * value, value);
}

Color decodeDc(int value) {
  return {srgbToLinear(value >> 16), srgbToLinear(value >> 8), srgbToLinear(value)};
}

Color decodeAc(int value, float maxValue) {
  const int quantR = value / (AC_LEVELS * AC_LEVELS);
  const int quantG = (value / AC_LEVELS) % AC_LEVELS;
  const int quantB = value % AC_LEVELS;
  return {signedSquare((quantR - AC_CENTER) / AC_CENTER) * maxValue,
          signedSquare((quantG - AC_CENTER) / AC_CENTER) * maxValue,
          signedSquare((quantB - AC_CENTER) / AC_CENTER) * maxValue};
}

bool decodeColors(const char* hash, int componentsX, int componentsY, std::vector<Color>& colors) {
  const int quantizedMax = decode83(hash + 1, 1);
  if (quantizedMax < 0) {
    return false;
  }
  const float maxValue = (quantizedMax + 1) / MAX_VALUE_SCALE;
  colors.resize(componentsX * componentsY);
  const char* cursor = hash + SIZE_AND_MAX_CHARS;
  for (size_t i = 0; i < colors.size(); i++) {
    const size_t chars = i == 0 ? DC_CHARS : AC_CHARS;
    const int value = decode83(cursor, chars);
    if (value < 0) {
      return false;
    }
    colors[i] = i == 0 ? decodeDc(value) : decodeAc(value, maxValue);
    cursor += chars;
  }
  return true;
}

std::vector<float> cosineTable(int components, uint32_t length) {
  std::vector<float> table(components * length);
  for (int c = 0; c < components; c++) {
    for (uint32_t p = 0; p < length; p++) {
      table[c * length + p] = cosf(PI * p * c / length);
    }
  }
  return table;
}

uint16_t toRgb565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

}  // namespace

bool decodeBlurHashRgb565(const char* hash, uint32_t width, uint32_t height, uint16_t* pixels) {
  if (hash == nullptr || width == 0 || height == 0 || strlen(hash) < SIZE_AND_MAX_CHARS + DC_CHARS) {
    return false;
  }
  const int sizeFlag = decode83(hash, 1);
  if (sizeFlag < 0) {
    return false;
  }
  const int componentsX = sizeFlag % MAX_COMPONENTS + 1;
  const int componentsY = sizeFlag / MAX_COMPONENTS + 1;
  const size_t expectedLength = SIZE_AND_MAX_CHARS + DC_CHARS + AC_CHARS * (componentsX * componentsY - 1);
  std::vector<Color> colors;
  if (strlen(hash) != expectedLength || !decodeColors(hash, componentsX, componentsY, colors)) {
    return false;
  }

  const std::vector<float> cosX = cosineTable(componentsX, width);
  const std::vector<float> cosY = cosineTable(componentsY, height);
  for (uint32_t y = 0; y < height; y++) {
    for (uint32_t x = 0; x < width; x++) {
      Color sum = {0, 0, 0};
      for (int j = 0; j < componentsY; j++) {
        const float basisY = cosY[j * height + y];
        for (int i = 0; i < componentsX; i++) {
          const float basis = cosX[i * width + x] * basisY;
          const Color& color = colors[j * componentsX + i];
          sum.r += color.r * basis;
          sum.g += color.g * basis;
          sum.b += color.b * basis;
        }
      }
      pixels[y * width + x] = toRgb565(linearToSrgb(sum.r), linearToSrgb(sum.g), linearToSrgb(sum.b));
    }
  }
  return true;
}
