#include <unity.h>

#include <hubOutboundQueue.h>
#include <hubPowerSession.h>
#include <hubSleepTimer.h>

// Start near wrap; unsigned subtraction keeps elapsed-time checks valid across it.
static const unsigned long JUST_BEFORE_WRAP = (unsigned long)-1 - 1000UL;

static unsigned long after(unsigned long elapsed) {
  return JUST_BEFORE_WRAP + elapsed;
}

static void assertWrapped(unsigned long now) {
  TEST_ASSERT_TRUE_MESSAGE(now < JUST_BEFORE_WRAP, "clock did not actually wrap");
}

void setUp() {
}

void tearDown() {
}

void test_the_wrap_helper_really_wraps() {
  assertWrapped(after(3000));
  TEST_ASSERT_FALSE(after(500) < JUST_BEFORE_WRAP);
}

void test_a_queued_event_does_not_expire_early_across_the_wrap() {
  HubOutboundQueue queue;
  omote_RemoteEvent event = omote_RemoteEvent_init_zero;
  event.command = omote_OmoteCommand_POWER_ON;

  TEST_ASSERT_EQUAL(HubOutboundQueue::EnqueueResult::QUEUED,
                    queue.enqueue(event, JUST_BEFORE_WRAP, 5000));

  const HubOutboundQueue::QueuedEvent* queued = queue.peek();
  TEST_ASSERT_NOT_NULL(queued);

  assertWrapped(after(3000));
  TEST_ASSERT_FALSE(queue.isExpired(*queued, after(3000)));
}

// Distinguishes elapsed subtraction from an overflowing absolute deadline.
void test_a_queued_event_is_not_expired_when_only_its_deadline_would_overflow() {
  HubOutboundQueue queue;
  omote_RemoteEvent event = omote_RemoteEvent_init_zero;
  event.command = omote_OmoteCommand_POWER_ON;

  queue.enqueue(event, JUST_BEFORE_WRAP, 5000);
  const HubOutboundQueue::QueuedEvent* queued = queue.peek();

  const unsigned long now = after(500);
  TEST_ASSERT_TRUE_MESSAGE(now > JUST_BEFORE_WRAP, "clock should not have wrapped yet");
  TEST_ASSERT_FALSE(queue.isExpired(*queued, now));
}

void test_a_power_session_is_not_timed_out_when_only_its_deadline_would_overflow() {
  HubPowerSession session;
  session.expect("LG_TV", "TV", true, false, JUST_BEFORE_WRAP);
  session.noteTransmitted(JUST_BEFORE_WRAP);

  const unsigned long now = after(500);
  TEST_ASSERT_TRUE_MESSAGE(now > JUST_BEFORE_WRAP, "clock should not have wrapped yet");
  TEST_ASSERT_EQUAL(HubPowerSession::Phase::IN_PROGRESS, session.tick(now));
}

void test_the_sleep_timer_is_unaffected_when_only_its_deadline_would_overflow() {
  HubSleepTimer timer;
  timer.observe(HubSleepTimer::Stage::ARMED, 600, JUST_BEFORE_WRAP);

  const unsigned long now = after(500);
  TEST_ASSERT_TRUE_MESSAGE(now > JUST_BEFORE_WRAP, "clock should not have wrapped yet");
  TEST_ASSERT_EQUAL_UINT32(600, timer.remainingSeconds(now));
}

void test_a_queued_event_still_expires_on_time_across_the_wrap() {
  HubOutboundQueue queue;
  omote_RemoteEvent event = omote_RemoteEvent_init_zero;
  event.command = omote_OmoteCommand_POWER_ON;

  queue.enqueue(event, JUST_BEFORE_WRAP, 5000);
  const HubOutboundQueue::QueuedEvent* queued = queue.peek();

  assertWrapped(after(5000));
  TEST_ASSERT_TRUE(queue.isExpired(*queued, after(5000)));
}

void test_a_power_session_does_not_time_out_early_across_the_wrap() {
  HubPowerSession session;
  session.expect("LG_TV", "TV", true, false, JUST_BEFORE_WRAP);
  session.noteTransmitted(JUST_BEFORE_WRAP);

  assertWrapped(after(3000));
  TEST_ASSERT_EQUAL(HubPowerSession::Phase::IN_PROGRESS, session.tick(after(3000)));
}

void test_a_power_session_still_times_out_across_the_wrap() {
  HubPowerSession session;
  session.expect("LG_TV", "TV", true, false, JUST_BEFORE_WRAP);
  session.noteTransmitted(JUST_BEFORE_WRAP);

  assertWrapped(after(HubPowerSession::TIMEOUT_MS));
  TEST_ASSERT_EQUAL(HubPowerSession::Phase::TIMED_OUT,
                    session.tick(after(HubPowerSession::TIMEOUT_MS)));
}

void test_the_sleep_timer_counts_down_correctly_across_the_wrap() {
  HubSleepTimer timer;
  timer.observe(HubSleepTimer::Stage::ARMED, 600, JUST_BEFORE_WRAP);

  assertWrapped(after(10000));
  TEST_ASSERT_EQUAL_UINT32(590, timer.remainingSeconds(after(10000)));
}

void test_the_sleep_timer_does_not_jump_to_zero_at_the_wrap() {
  HubSleepTimer timer;
  timer.observe(HubSleepTimer::Stage::ARMED, 600, JUST_BEFORE_WRAP);

  TEST_ASSERT_EQUAL_UINT32(600, timer.remainingSeconds(JUST_BEFORE_WRAP));
  assertWrapped(after(2000));
  TEST_ASSERT_EQUAL_UINT32(598, timer.remainingSeconds(after(2000)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_the_wrap_helper_really_wraps);
  RUN_TEST(test_a_queued_event_does_not_expire_early_across_the_wrap);
  RUN_TEST(test_a_queued_event_is_not_expired_when_only_its_deadline_would_overflow);
  RUN_TEST(test_a_power_session_is_not_timed_out_when_only_its_deadline_would_overflow);
  RUN_TEST(test_the_sleep_timer_is_unaffected_when_only_its_deadline_would_overflow);
  RUN_TEST(test_a_queued_event_still_expires_on_time_across_the_wrap);
  RUN_TEST(test_a_power_session_does_not_time_out_early_across_the_wrap);
  RUN_TEST(test_a_power_session_still_times_out_across_the_wrap);
  RUN_TEST(test_the_sleep_timer_counts_down_correctly_across_the_wrap);
  RUN_TEST(test_the_sleep_timer_does_not_jump_to_zero_at_the_wrap);
  return UNITY_END();
}
