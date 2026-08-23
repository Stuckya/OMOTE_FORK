#include <unity.h>
#include <hubPowerSession.h>

using Phase = HubPowerSession::Phase;
using Outcome = HubPowerSession::Outcome;
using Tone = HubPowerSession::Tone;

void setUp() {}
void tearDown() {}

static const unsigned long T0 = 1000;

static HubPowerSession shieldSceneSession(unsigned long now = T0) {
  HubPowerSession session;
  session.expect("ANDROID_TV", "Shield", true, false, now);
  session.expect("DENON_AVR", "Denon AVR", true, false, now);
  session.expect("LG_TV", "LG TV", true, false, now);
  return session;
}

void test_starts_idle_with_nothing_to_show() {
  HubPowerSession session;

  TEST_ASSERT_EQUAL(Phase::IDLE, session.phase());
  TEST_ASSERT_EQUAL_UINT(0, session.size());
  TEST_ASSERT_EQUAL(Phase::IDLE, session.tick(T0 + HubPowerSession::TIMEOUT_MS * 2));
}

void test_expect_opens_a_session_in_send_order() {
  HubPowerSession session = shieldSceneSession();

  TEST_ASSERT_EQUAL(Phase::IN_PROGRESS, session.phase());
  TEST_ASSERT_TRUE(session.targetOn());
  TEST_ASSERT_EQUAL_UINT(3, session.size());
  TEST_ASSERT_EQUAL_STRING("ANDROID_TV", session.at(0).id.c_str());
  TEST_ASSERT_EQUAL_STRING("DENON_AVR", session.at(1).id.c_str());
  TEST_ASSERT_EQUAL_STRING("LG_TV", session.at(2).id.c_str());
  TEST_ASSERT_EQUAL(Outcome::PENDING, session.at(2).outcome);
  TEST_ASSERT_EQUAL(Tone::BUSY, session.tone());
  TEST_ASSERT_EQUAL_STRING("Powering on...", session.headline().c_str());
}

void test_expect_ignores_duplicates_and_caps_at_capacity() {
  HubPowerSession session;
  session.expect("LG_TV", "LG TV", true, false, T0);
  session.expect("LG_TV", "LG TV", true, false, T0);
  TEST_ASSERT_EQUAL_UINT(1, session.size());

  char id[8];
  for (int i = 0; i < (int)HubPowerSession::CAPACITY + 2; i++) {
    snprintf(id, sizeof(id), "DEV_%d", i);
    session.expect(id, id, true, false, T0);
  }
  TEST_ASSERT_EQUAL_UINT(HubPowerSession::CAPACITY, session.size());
}

void test_device_already_in_target_state_is_confirmed_on_arrival() {
  HubPowerSession session;
  session.expect("ANDROID_TV", "Shield", true, false, T0);
  session.expect("LG_TV", "LG TV", true, true, T0);  // left on by the previous scene

  TEST_ASSERT_EQUAL(Outcome::PENDING, session.at(0).outcome);
  TEST_ASSERT_EQUAL(Outcome::CONFIRMED, session.at(1).outcome);
  TEST_ASSERT_EQUAL(Phase::IN_PROGRESS, session.phase());
}

void test_resolves_immediately_when_every_device_is_already_there() {
  HubPowerSession session;
  session.expect("LG_TV", "LG TV", true, true, T0);

  TEST_ASSERT_EQUAL(Phase::RESOLVED, session.phase());
  TEST_ASSERT_EQUAL(Tone::OK, session.tone());
  TEST_ASSERT_EQUAL_STRING("LG TV on", session.headline().c_str());
}

void test_observations_confirm_devices_and_resolve_the_session() {
  HubPowerSession session = shieldSceneSession();

  TEST_ASSERT_TRUE(session.observe("DENON_AVR", true));
  TEST_ASSERT_EQUAL(Outcome::CONFIRMED, session.at(1).outcome);
  TEST_ASSERT_EQUAL(Phase::IN_PROGRESS, session.phase());

  TEST_ASSERT_TRUE(session.observe("ANDROID_TV", true));
  TEST_ASSERT_TRUE(session.observe("LG_TV", true));
  TEST_ASSERT_EQUAL(Phase::RESOLVED, session.phase());
  TEST_ASSERT_EQUAL(Tone::OK, session.tone());
  TEST_ASSERT_EQUAL_STRING("All devices on", session.headline().c_str());
}

void test_observations_that_do_not_change_anything_report_false() {
  HubPowerSession session = shieldSceneSession();

  TEST_ASSERT_FALSE(session.observe("APPLE_TV", true));   // not in this session
  TEST_ASSERT_FALSE(session.observe("LG_TV", false));     // contrary: left to the timeout
  TEST_ASSERT_EQUAL(Outcome::PENDING, session.at(2).outcome);
  TEST_ASSERT_TRUE(session.observe("LG_TV", true));
  TEST_ASSERT_FALSE(session.observe("LG_TV", true));      // already confirmed

  HubPowerSession idle;
  TEST_ASSERT_FALSE(idle.observe("LG_TV", true));
}

void test_timeout_fails_pending_devices_and_names_the_straggler() {
  HubPowerSession session = shieldSceneSession();
  session.observe("ANDROID_TV", true);
  session.observe("LG_TV", true);

  TEST_ASSERT_EQUAL(Phase::IN_PROGRESS, session.tick(T0 + HubPowerSession::TIMEOUT_MS - 1));
  TEST_ASSERT_EQUAL(Phase::TIMED_OUT, session.tick(T0 + HubPowerSession::TIMEOUT_MS));
  TEST_ASSERT_EQUAL(Outcome::FAILED, session.at(1).outcome);
  TEST_ASSERT_EQUAL(Outcome::CONFIRMED, session.at(0).outcome);
  TEST_ASSERT_EQUAL(Tone::WARN, session.tone());
  TEST_ASSERT_EQUAL_STRING("Denon AVR no reply", session.headline().c_str());
}

void test_timeout_counts_multiple_stragglers() {
  HubPowerSession session = shieldSceneSession();
  session.observe("ANDROID_TV", true);

  session.tick(T0 + HubPowerSession::TIMEOUT_MS);

  TEST_ASSERT_EQUAL_STRING("2 devices no reply", session.headline().c_str());
}

void test_late_observation_recovers_a_timed_out_session() {
  HubPowerSession session = shieldSceneSession();
  session.observe("ANDROID_TV", true);
  session.observe("LG_TV", true);
  session.tick(T0 + HubPowerSession::TIMEOUT_MS);

  TEST_ASSERT_TRUE(session.observe("DENON_AVR", true));

  TEST_ASSERT_EQUAL(Outcome::CONFIRMED, session.at(1).outcome);
  TEST_ASSERT_EQUAL(Phase::RESOLVED, session.phase());
  TEST_ASSERT_EQUAL_STRING("All devices on", session.headline().c_str());
}

void test_slots_fill_with_confirmations_from_the_left_regardless_of_device_order() {
  HubPowerSession session = shieldSceneSession();

  session.observe("DENON_AVR", true);  // sent second, confirmed first

  TEST_ASSERT_EQUAL(Outcome::CONFIRMED, session.slotOutcome(0));
  TEST_ASSERT_EQUAL(Outcome::PENDING, session.slotOutcome(1));
  TEST_ASSERT_EQUAL(Outcome::PENDING, session.slotOutcome(2));
  TEST_ASSERT_EQUAL(Outcome::CONFIRMED, session.at(1).outcome);  // the device record keeps who confirmed
}

void test_slots_place_failures_after_confirmations_at_timeout() {
  HubPowerSession session = shieldSceneSession();
  session.observe("DENON_AVR", true);
  session.observe("LG_TV", true);
  session.tick(T0 + HubPowerSession::TIMEOUT_MS);  // Shield, sent first, never answered

  TEST_ASSERT_EQUAL(Outcome::CONFIRMED, session.slotOutcome(0));
  TEST_ASSERT_EQUAL(Outcome::CONFIRMED, session.slotOutcome(1));
  TEST_ASSERT_EQUAL(Outcome::FAILED, session.slotOutcome(2));
  TEST_ASSERT_EQUAL(Outcome::FAILED, session.at(0).outcome);  // the device record keeps who failed
}

void test_power_off_session_uses_off_wording() {
  HubPowerSession session;
  session.expect("LG_TV", "LG TV", false, false, T0);
  session.expect("DENON_AVR", "Denon AVR", false, false, T0);

  TEST_ASSERT_FALSE(session.targetOn());
  TEST_ASSERT_EQUAL_STRING("Powering off...", session.headline().c_str());

  TEST_ASSERT_FALSE(session.observe("LG_TV", true));
  TEST_ASSERT_TRUE(session.observe("LG_TV", false));
  TEST_ASSERT_TRUE(session.observe("DENON_AVR", false));
  TEST_ASSERT_EQUAL_STRING("All devices off", session.headline().c_str());
}

void test_anonymous_error_takes_over_the_headline_while_in_progress() {
  HubPowerSession session = shieldSceneSession();

  session.noteError("Denon AVR: device unreachable");

  TEST_ASSERT_EQUAL(Phase::IN_PROGRESS, session.phase());
  TEST_ASSERT_EQUAL(Tone::WARN, session.tone());
  TEST_ASSERT_EQUAL_STRING("Denon AVR: device unreachable", session.headline().c_str());

  session.observe("ANDROID_TV", true);
  session.observe("DENON_AVR", true);
  session.observe("LG_TV", true);
  TEST_ASSERT_EQUAL_STRING("All devices on", session.headline().c_str());
}

void test_anonymous_error_fails_one_slot_immediately() {
  HubPowerSession session = shieldSceneSession();
  session.observe("ANDROID_TV", true);

  session.noteError("Denon AVR: device unreachable");

  TEST_ASSERT_EQUAL(Outcome::CONFIRMED, session.slotOutcome(0));
  TEST_ASSERT_EQUAL(Outcome::FAILED, session.slotOutcome(1));
  TEST_ASSERT_EQUAL(Outcome::PENDING, session.slotOutcome(2));
  TEST_ASSERT_EQUAL(Phase::IN_PROGRESS, session.phase());
  TEST_ASSERT_EQUAL(Outcome::PENDING, session.at(1).outcome);  // no device is blamed
}

void test_anonymous_error_is_not_double_counted_at_timeout() {
  HubPowerSession session = shieldSceneSession();
  session.noteError("Denon AVR: device unreachable");
  session.observe("ANDROID_TV", true);
  session.observe("LG_TV", true);

  session.tick(T0 + HubPowerSession::TIMEOUT_MS);

  TEST_ASSERT_EQUAL(Outcome::CONFIRMED, session.slotOutcome(0));
  TEST_ASSERT_EQUAL(Outcome::CONFIRMED, session.slotOutcome(1));
  TEST_ASSERT_EQUAL(Outcome::FAILED, session.slotOutcome(2));
  TEST_ASSERT_EQUAL_STRING("Denon AVR: device unreachable", session.headline().c_str());
}

void test_anonymous_error_slot_clears_when_every_device_confirms() {
  HubPowerSession session = shieldSceneSession();
  session.noteError("Denon AVR: device unreachable");
  session.observe("ANDROID_TV", true);
  session.observe("DENON_AVR", true);
  session.observe("LG_TV", true);

  TEST_ASSERT_EQUAL(Phase::RESOLVED, session.phase());
  TEST_ASSERT_EQUAL(Outcome::CONFIRMED, session.slotOutcome(2));
}

void test_expect_after_a_finished_session_starts_a_fresh_one() {
  HubPowerSession session;
  session.expect("LG_TV", "LG TV", true, false, T0);
  session.noteError("Hub error");
  session.tick(T0 + HubPowerSession::TIMEOUT_MS);
  TEST_ASSERT_EQUAL(Phase::TIMED_OUT, session.phase());

  session.expect("DENON_AVR", "Denon AVR", true, false, T0 + 20000);

  TEST_ASSERT_EQUAL(Phase::IN_PROGRESS, session.phase());
  TEST_ASSERT_EQUAL_UINT(1, session.size());
  TEST_ASSERT_EQUAL_STRING("DENON_AVR", session.at(0).id.c_str());
  TEST_ASSERT_EQUAL_STRING("Powering on...", session.headline().c_str());
  TEST_ASSERT_EQUAL(Phase::IN_PROGRESS, session.tick(T0 + 20000 + HubPowerSession::TIMEOUT_MS - 1));
}

void test_timeout_is_measured_from_the_session_open() {
  HubPowerSession session;
  session.expect("ANDROID_TV", "Shield", true, false, T0);
  session.expect("LG_TV", "LG TV", true, false, T0 + 3000);  // a slow scene sequence

  TEST_ASSERT_EQUAL(Phase::TIMED_OUT, session.tick(T0 + HubPowerSession::TIMEOUT_MS));
}

void test_reset_returns_to_idle() {
  HubPowerSession session = shieldSceneSession();

  session.reset();

  TEST_ASSERT_EQUAL(Phase::IDLE, session.phase());
  TEST_ASSERT_EQUAL_UINT(0, session.size());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_starts_idle_with_nothing_to_show);
  RUN_TEST(test_expect_opens_a_session_in_send_order);
  RUN_TEST(test_expect_ignores_duplicates_and_caps_at_capacity);
  RUN_TEST(test_device_already_in_target_state_is_confirmed_on_arrival);
  RUN_TEST(test_resolves_immediately_when_every_device_is_already_there);
  RUN_TEST(test_observations_confirm_devices_and_resolve_the_session);
  RUN_TEST(test_observations_that_do_not_change_anything_report_false);
  RUN_TEST(test_timeout_fails_pending_devices_and_names_the_straggler);
  RUN_TEST(test_timeout_counts_multiple_stragglers);
  RUN_TEST(test_late_observation_recovers_a_timed_out_session);
  RUN_TEST(test_slots_fill_with_confirmations_from_the_left_regardless_of_device_order);
  RUN_TEST(test_slots_place_failures_after_confirmations_at_timeout);
  RUN_TEST(test_power_off_session_uses_off_wording);
  RUN_TEST(test_anonymous_error_takes_over_the_headline_while_in_progress);
  RUN_TEST(test_anonymous_error_fails_one_slot_immediately);
  RUN_TEST(test_anonymous_error_is_not_double_counted_at_timeout);
  RUN_TEST(test_anonymous_error_slot_clears_when_every_device_confirms);
  RUN_TEST(test_expect_after_a_finished_session_starts_a_fresh_one);
  RUN_TEST(test_timeout_is_measured_from_the_session_open);
  RUN_TEST(test_reset_returns_to_idle);
  return UNITY_END();
}
