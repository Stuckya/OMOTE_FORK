#include <unity.h>
#include <hubSleepTimer.h>

using Stage = HubSleepTimer::Stage;

void setUp() {}
void tearDown() {}

static const unsigned long T0 = 1000;

static HubSleepTimer armed(uint32_t remainingSeconds, unsigned long now = T0) {
  HubSleepTimer timer;
  timer.observe(Stage::ARMED, remainingSeconds, now);
  return timer;
}

void test_starts_with_no_timer() {
  HubSleepTimer timer;

  TEST_ASSERT_EQUAL(Stage::IDLE, timer.stage());
  TEST_ASSERT_FALSE(timer.isArmed());
  TEST_ASSERT_EQUAL_UINT32(0, timer.remainingSeconds(T0));
}

void test_an_armed_push_starts_the_countdown() {
  HubSleepTimer timer = armed(45 * 60);

  TEST_ASSERT_EQUAL(Stage::ARMED, timer.stage());
  TEST_ASSERT_TRUE(timer.isArmed());
  TEST_ASSERT_EQUAL_UINT32(45 * 60, timer.remainingSeconds(T0));
}

void test_the_countdown_ticks_between_pushes() {
  HubSleepTimer timer = armed(45 * 60);

  TEST_ASSERT_EQUAL_UINT32(45 * 60 - 90, timer.remainingSeconds(T0 + 90000));
}

void test_the_countdown_never_runs_past_zero() {
  HubSleepTimer timer = armed(30);

  TEST_ASSERT_EQUAL_UINT32(0, timer.remainingSeconds(T0 + 120000));
}

void test_a_later_push_overrides_the_local_tick() {
  HubSleepTimer timer = armed(45 * 60);

  timer.observe(Stage::ARMED, 60 * 60, T0 + 90000);

  TEST_ASSERT_EQUAL_UINT32(60 * 60, timer.remainingSeconds(T0 + 90000));
}

void test_the_indicator_rounds_minutes_up() {
  // The sheet shows 41:32 while the status bar shows 42m: the same minute.
  HubSleepTimer timer = armed(41 * 60 + 32);

  TEST_ASSERT_EQUAL_STRING("42m", timer.indicatorText(T0).c_str());
  TEST_ASSERT_EQUAL_STRING("41:32", timer.countdownText(T0).c_str());
}

void test_the_last_partial_minute_still_reads_as_one() {
  HubSleepTimer timer = armed(32);

  TEST_ASSERT_EQUAL_STRING("1m", timer.indicatorText(T0).c_str());
  TEST_ASSERT_EQUAL_STRING("00:32", timer.countdownText(T0).c_str());
}

void test_a_long_timer_keeps_counting_in_minutes() {
  HubSleepTimer timer = armed(180 * 60);

  TEST_ASSERT_EQUAL_STRING("180m", timer.indicatorText(T0).c_str());
  TEST_ASSERT_EQUAL_STRING("180:00", timer.countdownText(T0).c_str());
}

void test_an_idle_push_clears_the_timer() {
  HubSleepTimer timer = armed(45 * 60);

  timer.observe(Stage::IDLE, 0, T0 + 1000);

  TEST_ASSERT_EQUAL(Stage::IDLE, timer.stage());
  TEST_ASSERT_FALSE(timer.isArmed());
}

void test_the_warning_is_raised_once_per_crossing() {
  HubSleepTimer timer = armed(45 * 60);

  timer.observe(Stage::WARNING, 60, T0 + 1000);
  TEST_ASSERT_TRUE(timer.takeWarning());
  TEST_ASSERT_FALSE(timer.takeWarning());

  timer.observe(Stage::WARNING, 45, T0 + 16000);
  TEST_ASSERT_FALSE(timer.takeWarning());
}

void test_extending_out_of_the_warning_arms_a_fresh_one() {
  HubSleepTimer timer = armed(45 * 60);
  timer.observe(Stage::WARNING, 60, T0 + 1000);
  TEST_ASSERT_TRUE(timer.takeWarning());

  timer.observe(Stage::ARMED, 15 * 60, T0 + 2000);
  TEST_ASSERT_EQUAL(Stage::ARMED, timer.stage());

  timer.observe(Stage::WARNING, 60, T0 + 3000);
  TEST_ASSERT_TRUE(timer.takeWarning());
}

void test_waking_inside_the_final_minute_still_warns() {
  // Deep sleep took the previous stage with it; the first push is the crossing.
  HubSleepTimer timer;

  timer.observe(Stage::WARNING, 30, T0);

  TEST_ASSERT_TRUE(timer.takeWarning());
  TEST_ASSERT_TRUE(timer.isArmed());
}

void test_firing_leaves_no_timer_behind() {
  HubSleepTimer timer = armed(60);

  timer.observe(Stage::FIRED, 0, T0 + 60000);

  TEST_ASSERT_TRUE(timer.takeFired());
  TEST_ASSERT_FALSE(timer.takeFired());
  TEST_ASSERT_EQUAL(Stage::IDLE, timer.stage());
  TEST_ASSERT_FALSE(timer.isArmed());
}

void test_a_fired_push_does_not_also_warn() {
  HubSleepTimer timer = armed(60);

  timer.observe(Stage::FIRED, 0, T0 + 60000);

  TEST_ASSERT_FALSE(timer.takeWarning());
}

void test_minutes_snap_to_the_quarter_hour_within_range() {
  TEST_ASSERT_EQUAL_UINT16(15, HubSleepTimer::snapMinutes(15));
  TEST_ASSERT_EQUAL_UINT16(45, HubSleepTimer::snapMinutes(44));
  TEST_ASSERT_EQUAL_UINT16(45, HubSleepTimer::snapMinutes(52));
  TEST_ASSERT_EQUAL_UINT16(60, HubSleepTimer::snapMinutes(53));
}

void test_minutes_clamp_to_the_ends_of_the_ring() {
  TEST_ASSERT_EQUAL_UINT16(15, HubSleepTimer::snapMinutes(0));
  TEST_ASSERT_EQUAL_UINT16(15, HubSleepTimer::snapMinutes(-30));
  TEST_ASSERT_EQUAL_UINT16(180, HubSleepTimer::snapMinutes(200));
}

void test_reset_forgets_the_timer_and_its_pending_edges() {
  HubSleepTimer timer = armed(45 * 60);
  timer.observe(Stage::WARNING, 60, T0 + 1000);

  timer.reset();

  TEST_ASSERT_EQUAL(Stage::IDLE, timer.stage());
  TEST_ASSERT_FALSE(timer.takeWarning());
  TEST_ASSERT_FALSE(timer.takeFired());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_starts_with_no_timer);
  RUN_TEST(test_an_armed_push_starts_the_countdown);
  RUN_TEST(test_the_countdown_ticks_between_pushes);
  RUN_TEST(test_the_countdown_never_runs_past_zero);
  RUN_TEST(test_a_later_push_overrides_the_local_tick);
  RUN_TEST(test_the_indicator_rounds_minutes_up);
  RUN_TEST(test_the_last_partial_minute_still_reads_as_one);
  RUN_TEST(test_a_long_timer_keeps_counting_in_minutes);
  RUN_TEST(test_an_idle_push_clears_the_timer);
  RUN_TEST(test_the_warning_is_raised_once_per_crossing);
  RUN_TEST(test_extending_out_of_the_warning_arms_a_fresh_one);
  RUN_TEST(test_waking_inside_the_final_minute_still_warns);
  RUN_TEST(test_firing_leaves_no_timer_behind);
  RUN_TEST(test_a_fired_push_does_not_also_warn);
  RUN_TEST(test_minutes_snap_to_the_quarter_hour_within_range);
  RUN_TEST(test_minutes_clamp_to_the_ends_of_the_ring);
  RUN_TEST(test_reset_forgets_the_timer_and_its_pending_edges);
  return UNITY_END();
}
