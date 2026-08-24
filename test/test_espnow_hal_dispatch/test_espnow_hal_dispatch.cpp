#include <unity.h>

#include <espNowRxQueue.h>
#include "mock_hub_simulator.h"

#include <cstring>
#include <vector>

// Stub the threaded mock hub so this unit can include the HAL implementation.
void startMockHubSimulator(EspNowMessageCallback) {}
void stopMockHubSimulator() {}
void handleMockHubCommand(const uint8_t*, size_t) {}

#include "espnow_hal_windows_linux.cpp"

static int dispatchCount = 0;
static std::vector<std::vector<uint8_t> > dispatched;

static void recordDispatch(const uint8_t* data, size_t len) {
  dispatchCount++;
  dispatched.push_back(std::vector<uint8_t>(data, data + len));
}

static std::vector<uint8_t> frameOf(uint8_t marker, size_t length) {
  std::vector<uint8_t> frame(length);
  for (size_t i = 0; i < length; i++) {
    frame[i] = (uint8_t)(marker + i);
  }
  return frame;
}

static void deliver(const std::vector<uint8_t>& frame) {
  receiveEspNowFrame_HAL(frame.data(), frame.size());
}

void setUp() {
  dispatchCount = 0;
  dispatched.clear();
  set_announceEspNowMessage_cb_HAL(&recordDispatch);
  init_espnow_HAL();
  // Drain anything a previous test left parked; the HAL owns process-wide state.
  espnow_loop_HAL();
  dispatchCount = 0;
  dispatched.clear();
}

void tearDown() {
  espnow_loop_HAL();
}

// Regression: the WiFi callback previously invoked application code directly.
void test_a_received_frame_is_not_dispatched_inline() {
  const std::vector<uint8_t> frame = frameOf(1, 8);

  deliver(frame);

  TEST_ASSERT_EQUAL_INT(0, dispatchCount);
}

void test_the_frame_is_dispatched_when_the_loop_runs() {
  const std::vector<uint8_t> frame = frameOf(1, 8);

  deliver(frame);
  espnow_loop_HAL();

  TEST_ASSERT_EQUAL_INT(1, dispatchCount);
  TEST_ASSERT_EQUAL_UINT(frame.size(), dispatched[0].size());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(frame.data(), dispatched[0].data(), frame.size());
}

void test_frames_delivered_between_loops_are_dispatched_in_order() {
  deliver(frameOf(10, 4));
  deliver(frameOf(20, 5));
  deliver(frameOf(30, 6));

  TEST_ASSERT_EQUAL_INT(0, dispatchCount);

  espnow_loop_HAL();

  TEST_ASSERT_EQUAL_INT(3, dispatchCount);
  TEST_ASSERT_EQUAL_UINT(4, dispatched[0].size());
  TEST_ASSERT_EQUAL_UINT(5, dispatched[1].size());
  TEST_ASSERT_EQUAL_UINT(6, dispatched[2].size());
  TEST_ASSERT_EQUAL_UINT8(10, dispatched[0][0]);
  TEST_ASSERT_EQUAL_UINT8(20, dispatched[1][0]);
  TEST_ASSERT_EQUAL_UINT8(30, dispatched[2][0]);
}

void test_a_loop_with_nothing_queued_dispatches_nothing() {
  espnow_loop_HAL();
  TEST_ASSERT_EQUAL_INT(0, dispatchCount);
}

void test_each_frame_is_dispatched_exactly_once() {
  deliver(frameOf(7, 4));

  espnow_loop_HAL();
  espnow_loop_HAL();

  TEST_ASSERT_EQUAL_INT(1, dispatchCount);
}

void test_an_oversize_frame_never_reaches_the_application() {
  const std::vector<uint8_t> frame = frameOf(1, 251);

  deliver(frame);
  espnow_loop_HAL();

  TEST_ASSERT_EQUAL_INT(0, dispatchCount);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_a_received_frame_is_not_dispatched_inline);
  RUN_TEST(test_the_frame_is_dispatched_when_the_loop_runs);
  RUN_TEST(test_frames_delivered_between_loops_are_dispatched_in_order);
  RUN_TEST(test_a_loop_with_nothing_queued_dispatches_nothing);
  RUN_TEST(test_each_frame_is_dispatched_exactly_once);
  RUN_TEST(test_an_oversize_frame_never_reaches_the_application);
  return UNITY_END();
}
