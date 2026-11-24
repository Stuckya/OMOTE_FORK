#include <unity.h>
#include <string>
#include <vector>

#include "applicationInternal/gui/guiShownListeners.h"

static std::vector<std::string> heardByFirst;
static std::vector<std::string> heardBySecond;

void setUp() {
  heardByFirst.clear();
  heardBySecond.clear();
}
void tearDown() {}

static void recordFirst(const std::string& shownGui) {
  heardByFirst.push_back(shownGui);
}

static void recordSecond(const std::string& shownGui) {
  heardBySecond.push_back(shownGui);
}

void test_nothing_is_heard_before_a_tab_is_shown() {
  Gui::registerShownGuiListener(recordFirst);

  TEST_ASSERT_EQUAL_UINT(0, heardByFirst.size());
}

void test_a_listener_hears_every_shown_tab() {
  Gui::registerShownGuiListener(recordFirst);

  Gui::notifyShownGui("Apple TV");
  Gui::notifyShownGui("Numpad");

  TEST_ASSERT_EQUAL_UINT(2, heardByFirst.size());
  TEST_ASSERT_EQUAL_STRING("Apple TV", heardByFirst[0].c_str());
  TEST_ASSERT_EQUAL_STRING("Numpad", heardByFirst[1].c_str());
}

void test_every_listener_hears_the_shown_tab_once() {
  Gui::registerShownGuiListener(recordFirst);
  Gui::registerShownGuiListener(recordSecond);
  Gui::registerShownGuiListener(recordSecond);

  Gui::notifyShownGui("Apple TV");

  TEST_ASSERT_EQUAL_UINT(1, heardByFirst.size());
  TEST_ASSERT_EQUAL_UINT(1, heardBySecond.size());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_nothing_is_heard_before_a_tab_is_shown);
  RUN_TEST(test_a_listener_hears_every_shown_tab);
  RUN_TEST(test_every_listener_hears_the_shown_tab_once);
  return UNITY_END();
}
