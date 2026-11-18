#include <cstring>
#include <unity.h>

#include "extensions/lcd1602/viewport.h"

namespace {

Lcd1602LineState lineWith(const char* text, Lcd1602Align align) {
  Lcd1602LineState line{};
  strlcpy(line.text, text, sizeof(line.text));
  line.align = align;
  return line;
}

}  // namespace

void test_short_left_static() {
  char out[kLcd1602Cols + 1];
  lcd1602FormatViewport(lineWith("hi", Lcd1602Align::Left), 0, out);
  TEST_ASSERT_EQUAL_STRING("hi              ", out);
  lcd1602FormatViewport(lineWith("hi", Lcd1602Align::Left), 3, out);
  TEST_ASSERT_EQUAL_STRING("hi              ", out);
}

void test_short_right_and_center() {
  char out[kLcd1602Cols + 1];
  lcd1602FormatViewport(lineWith("ABC", Lcd1602Align::Right), 0, out);
  TEST_ASSERT_EQUAL_STRING("             ABC", out);
  lcd1602FormatViewport(lineWith("ABCD", Lcd1602Align::Center), 0, out);
  TEST_ASSERT_EQUAL_STRING("      ABCD      ", out);
}

void test_long_line_scroll_window() {
  char out[kLcd1602Cols + 1];
  const auto line = lineWith("ABCDEFGHIJKLMNOPQRST", Lcd1602Align::Left);
  lcd1602FormatViewport(line, 0, out);
  TEST_ASSERT_EQUAL_STRING("ABCDEFGHIJKLMNOP", out);
  lcd1602FormatViewport(line, 1, out);
  TEST_ASSERT_EQUAL_STRING("BCDEFGHIJKLMNOPQ", out);
  lcd1602FormatViewport(line, 4, out);
  TEST_ASSERT_EQUAL_STRING("EFGHIJKLMNOPQRST", out);
  // After last char: 3-space gap before wrapping to A.
  lcd1602FormatViewport(line, 19, out);
  TEST_ASSERT_EQUAL_STRING("T   ABCDEFGHIJKL", out);
  lcd1602FormatViewport(line, 20, out);
  TEST_ASSERT_EQUAL_STRING("   ABCDEFGHIJKLM", out);
  lcd1602FormatViewport(line, 23, out);
  TEST_ASSERT_EQUAL_STRING("ABCDEFGHIJKLMNOP", out);
}

void test_empty_line_clears() {
  char out[kLcd1602Cols + 1];
  lcd1602FormatViewport(lineWith("", Lcd1602Align::Center), 0, out);
  TEST_ASSERT_EQUAL_STRING("                ", out);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_short_left_static);
  RUN_TEST(test_short_right_and_center);
  RUN_TEST(test_long_line_scroll_window);
  RUN_TEST(test_empty_line_clears);
  return UNITY_END();
}
