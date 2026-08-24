#include <unity.h>

#include <hubOutboundQueue.h>
#include <hubPowerSession.h>
#include <hubSleepTimer.h>

// millis() is an unsigned long that wraps roughly every 49 days. Every deadline
// in the firmware is safe only because it is written as (now - start) >= span,
// which stays correct across the wrap. Rewritten as (start + span) <= now it
// would be wrong for 49 days and then wrong once, silently. These tests exist to
// make that rewrite fail here rather than in someone's living room.
// A start point 1 s before millis() wraps, so any elapsed time over 1 s lands on
// a clock reading *smaller* than the start.
static const unsigned long JUST_BEFORE_WRAP = (unsigned long)-1 - 1000UL;

// The clock reading after `elapsed` ms of real time. Unsigned overflow is
// defined, and that is exactly the behaviour under test.
static unsigned long after(unsigned long elapsed) {
  return JUST_BEFORE_WRAP + elapsed;
}

// Guards the tests against quietly ceasing to exercise the wrap.
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

  // 3 s later in real time, but a smaller number than when it was queued.
  assertWrapped(after(3000));
  TEST_ASSERT_FALSE(queue.isExpired(*queued, after(3000)));
}

// The case that separates the two forms. Here `now` has NOT wrapped yet, but
// start + ttl has -- so the naive form compares a wrapped sum against an
// unwrapped clock and reports everything expired the instant it is queued.
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

  // Well inside the budget in real time, despite the smaller number.
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

  // 10 s of real time elapsed, straddling the wrap.
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
