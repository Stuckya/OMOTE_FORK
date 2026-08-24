#include <unity.h>
#include <halCallback.h>

#include <string>

static int boolCalls = 0;
static bool lastBool = false;

static void recordBool(bool value) {
  boolCalls++;
  lastBool = value;
}

static void recordBoolAlternate(bool value) {
  boolCalls += 100;
  lastBool = value;
}

static int pairCalls = 0;
static std::string lastFirst;
static std::string lastSecond;

static void recordPair(std::string first, std::string second) {
  pairCalls++;
  lastFirst = first;
  lastSecond = second;
}

void setUp() {
  boolCalls = 0;
  lastBool = false;
  pairCalls = 0;
  lastFirst.clear();
  lastSecond.clear();
}

void tearDown() {
}

// The regression this type exists for: HAL events can arrive before the
// application registers, and every unguarded call site was a latent null deref.
void test_invoking_an_unset_callback_is_a_no_op() {
  HalCallback<bool> callback;

  TEST_ASSERT_FALSE(callback.isSet());
  callback(true);

  TEST_ASSERT_EQUAL_INT(0, boolCalls);
}

void test_a_registered_callback_receives_its_argument() {
  HalCallback<bool> callback;
  callback.set(&recordBool);

  TEST_ASSERT_TRUE(callback.isSet());
  callback(true);
  callback(false);

  TEST_ASSERT_EQUAL_INT(2, boolCalls);
  TEST_ASSERT_FALSE(lastBool);
}

void test_registering_again_replaces_the_previous_target() {
  HalCallback<bool> callback;
  callback.set(&recordBool);
  callback(true);
  TEST_ASSERT_EQUAL_INT(1, boolCalls);

  callback.set(&recordBoolAlternate);
  callback(true);
  TEST_ASSERT_EQUAL_INT(101, boolCalls);
}

void test_clearing_returns_the_callback_to_a_no_op() {
  HalCallback<bool> callback;
  callback.set(&recordBool);
  callback(true);

  callback.clear();
  TEST_ASSERT_FALSE(callback.isSet());
  callback(true);

  TEST_ASSERT_EQUAL_INT(1, boolCalls);
}

void test_setting_null_is_treated_as_unset() {
  HalCallback<bool> callback;
  callback.set(&recordBool);
  callback.set(nullptr);

  TEST_ASSERT_FALSE(callback.isSet());
  callback(true);

  TEST_ASSERT_EQUAL_INT(0, boolCalls);
}

void test_multiple_arguments_are_forwarded_in_order() {
  HalCallback<std::string, std::string> callback;

  callback("topic", "payload");
  TEST_ASSERT_EQUAL_INT(0, pairCalls);

  callback.set(&recordPair);
  callback("topic", "payload");

  TEST_ASSERT_EQUAL_INT(1, pairCalls);
  TEST_ASSERT_EQUAL_STRING("topic", lastFirst.c_str());
  TEST_ASSERT_EQUAL_STRING("payload", lastSecond.c_str());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_invoking_an_unset_callback_is_a_no_op);
  RUN_TEST(test_a_registered_callback_receives_its_argument);
  RUN_TEST(test_registering_again_replaces_the_previous_target);
  RUN_TEST(test_clearing_returns_the_callback_to_a_no_op);
  RUN_TEST(test_setting_null_is_treated_as_unset);
  RUN_TEST(test_multiple_arguments_are_forwarded_in_order);
  return UNITY_END();
}
