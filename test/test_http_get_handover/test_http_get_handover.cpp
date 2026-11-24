#include <unity.h>
#include <httpGetHandover.h>

void setUp() {}
void tearDown() {}

void test_nothing_to_take_before_a_get_starts() {
  HttpGetHandover handover;
  bool ok = true;

  TEST_ASSERT_FALSE(handover.take(ok));
  TEST_ASSERT_FALSE(handover.inFlight());
}

void test_one_get_at_a_time() {
  HttpGetHandover handover;

  TEST_ASSERT_TRUE(handover.begin());
  TEST_ASSERT_TRUE(handover.inFlight());
  TEST_ASSERT_FALSE(handover.begin());
}

void test_the_main_loop_takes_a_finished_get_once() {
  HttpGetHandover handover;
  handover.begin();
  bool ok = false;

  TEST_ASSERT_FALSE(handover.take(ok));
  handover.finish(true);

  TEST_ASSERT_TRUE(handover.take(ok));
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_FALSE(handover.take(ok));
  TEST_ASSERT_FALSE(handover.inFlight());
}

void test_a_failed_get_is_reported_as_failed() {
  HttpGetHandover handover;
  handover.begin();
  handover.finish(false);
  bool ok = true;

  TEST_ASSERT_TRUE(handover.take(ok));
  TEST_ASSERT_FALSE(ok);
}

void test_a_new_get_can_start_after_the_last_is_taken() {
  HttpGetHandover handover;
  handover.begin();
  handover.finish(true);
  TEST_ASSERT_FALSE(handover.begin());
  bool ok = false;
  handover.take(ok);

  TEST_ASSERT_TRUE(handover.begin());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_nothing_to_take_before_a_get_starts);
  RUN_TEST(test_one_get_at_a_time);
  RUN_TEST(test_the_main_loop_takes_a_finished_get_once);
  RUN_TEST(test_a_failed_get_is_reported_as_failed);
  RUN_TEST(test_a_new_get_can_start_after_the_last_is_taken);
  return UNITY_END();
}
