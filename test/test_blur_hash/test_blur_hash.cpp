#include <unity.h>
#include <cstdlib>
#include <vector>
#include <blurHash.h>

void setUp() {}
void tearDown() {}

struct Pixel {
  uint32_t x;
  uint32_t y;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

// Expected pixels come from the reference decoder (woltapp/blurhash, via blurhash-python 1.2.2).
static void assertDecodesTo(const char* hash, uint32_t width, uint32_t height,
                            const Pixel* expected, size_t count) {
  std::vector<uint16_t> pixels(width * height);
  TEST_ASSERT_TRUE(decodeBlurHashRgb565(hash, width, height, pixels.data()));
  for (size_t i = 0; i < count; i++) {
    const uint16_t actual = pixels[expected[i].y * width + expected[i].x];
    // RGB565 keeps 5, 6 and 5 bits; allow one step for rounding at the boundary.
    TEST_ASSERT_INT_WITHIN(1, expected[i].r >> 3, (actual >> 11) & 0x1F);
    TEST_ASSERT_INT_WITHIN(1, expected[i].g >> 2, (actual >> 5) & 0x3F);
    TEST_ASSERT_INT_WITHIN(1, expected[i].b >> 3, actual & 0x1F);
  }
}

void test_decodes_a_4_by_3_hash_like_the_reference() {
  const Pixel expected[] = {
      {0, 0, 135, 164, 177}, {31, 0, 137, 166, 181}, {16, 16, 158, 125, 108},
      {31, 31, 133, 142, 147}, {3, 30, 139, 145, 146},
  };
  assertDecodesTo("LEHV6nWB2yk8pyo0adR*.7kCMdnj", 32, 32, expected, 5);
}

void test_decodes_a_wide_cover_like_the_reference() {
  const Pixel expected[] = {
      {0, 0, 176, 118, 163}, {19, 0, 223, 84, 129}, {10, 5, 107, 126, 130},
      {19, 9, 135, 100, 109}, {3, 8, 146, 159, 139},
  };
  assertDecodesTo("LGF5]+Yk^6#M@-5c,1J5@[or[Q6.", 20, 10, expected, 5);
}

void test_decodes_an_odd_size_like_the_reference() {
  const Pixel expected[] = {
      {0, 0, 229, 228, 226}, {6, 0, 227, 226, 223}, {3, 2, 216, 206, 195},
      {6, 4, 219, 216, 213}, {3, 3, 213, 203, 192},
  };
  assertDecodesTo("L6PZfSi_.AyE_3t7t7R**0o#DgR4", 7, 5, expected, 5);
}

void test_rejects_a_hash_too_short_to_hold_a_color() {
  uint16_t pixel = 0;
  TEST_ASSERT_FALSE(decodeBlurHashRgb565("LEH", 1, 1, &pixel));
}

void test_rejects_a_hash_whose_length_disagrees_with_its_size_flag() {
  uint16_t pixel = 0;
  TEST_ASSERT_FALSE(decodeBlurHashRgb565("LEHV6nWB2yk8pyo0adR*.7kCMdn", 1, 1, &pixel));
}

void test_rejects_a_hash_with_a_character_outside_base83() {
  uint16_t pixel = 0;
  TEST_ASSERT_FALSE(decodeBlurHashRgb565("LEHV6nWB2yk8pyo0adR*.7kCMd n", 1, 1, &pixel));
}

void test_rejects_an_empty_image() {
  uint16_t pixel = 0;
  TEST_ASSERT_FALSE(decodeBlurHashRgb565("LEHV6nWB2yk8pyo0adR*.7kCMdnj", 0, 1, &pixel));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_decodes_a_4_by_3_hash_like_the_reference);
  RUN_TEST(test_decodes_a_wide_cover_like_the_reference);
  RUN_TEST(test_decodes_an_odd_size_like_the_reference);
  RUN_TEST(test_rejects_a_hash_too_short_to_hold_a_color);
  RUN_TEST(test_rejects_a_hash_whose_length_disagrees_with_its_size_flag);
  RUN_TEST(test_rejects_a_hash_with_a_character_outside_base83);
  RUN_TEST(test_rejects_an_empty_image);
  return UNITY_END();
}
