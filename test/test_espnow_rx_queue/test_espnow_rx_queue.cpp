#include <unity.h>
#include <espNowRxQueue.h>

#include <cstring>

static const size_t MAX_HELD = EspNowRxQueue::CAPACITY - 1;

// Distinct, length-varying payload per marker so ordering and byte fidelity
// both fail loudly if the ring mixes slots up.
static size_t makeFrame(uint8_t marker, uint8_t* out) {
  const size_t length = 1 + (marker % 16);
  for (size_t i = 0; i < length; i++) {
    out[i] = (uint8_t)(marker + i);
  }
  return length;
}

static void pushFrame(EspNowRxQueue& queue, uint8_t marker) {
  uint8_t payload[EspNowRxQueue::MAX_FRAME_BYTES];
  const size_t length = makeFrame(marker, payload);
  TEST_ASSERT_TRUE(queue.push(payload, length));
}

static void expectFrame(EspNowRxQueue& queue, uint8_t marker) {
  uint8_t expected[EspNowRxQueue::MAX_FRAME_BYTES];
  const size_t expectedLength = makeFrame(marker, expected);

  EspNowRxQueue::Frame frame;
  TEST_ASSERT_TRUE(queue.pop(frame));
  TEST_ASSERT_EQUAL_UINT(expectedLength, frame.length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, frame.bytes, expectedLength);
}

void setUp() {
}

void tearDown() {
}

void test_pop_returns_the_bytes_that_were_pushed() {
  EspNowRxQueue queue;
  TEST_ASSERT_TRUE(queue.isEmpty());

  pushFrame(queue, 42);
  TEST_ASSERT_EQUAL_UINT(1, queue.count());

  expectFrame(queue, 42);
  TEST_ASSERT_TRUE(queue.isEmpty());
  TEST_ASSERT_EQUAL_UINT32(0, queue.droppedFrames());
}

void test_frames_are_drained_in_arrival_order() {
  EspNowRxQueue queue;
  for (uint8_t i = 0; i < MAX_HELD; i++) {
    pushFrame(queue, i);
  }
  TEST_ASSERT_EQUAL_UINT(MAX_HELD, queue.count());

  for (uint8_t i = 0; i < MAX_HELD; i++) {
    expectFrame(queue, i);
  }
  TEST_ASSERT_TRUE(queue.isEmpty());
}

void test_pop_on_empty_queue_reports_no_frame() {
  EspNowRxQueue queue;
  EspNowRxQueue::Frame frame;
  TEST_ASSERT_FALSE(queue.pop(frame));
}

void test_full_queue_drops_the_newest_and_keeps_the_backlog() {
  EspNowRxQueue queue;
  for (uint8_t i = 0; i < MAX_HELD; i++) {
    pushFrame(queue, i);
  }

  uint8_t overflow[EspNowRxQueue::MAX_FRAME_BYTES];
  const size_t overflowLength = makeFrame(200, overflow);
  TEST_ASSERT_FALSE(queue.push(overflow, overflowLength));

  TEST_ASSERT_EQUAL_UINT(MAX_HELD, queue.count());
  TEST_ASSERT_EQUAL_UINT32(1, queue.droppedFrames());

  // The already-queued frames must survive the drop unchanged.
  for (uint8_t i = 0; i < MAX_HELD; i++) {
    expectFrame(queue, i);
  }
}

void test_draining_frees_slots_for_later_frames() {
  EspNowRxQueue queue;
  for (uint8_t i = 0; i < MAX_HELD; i++) {
    pushFrame(queue, i);
  }

  expectFrame(queue, 0);
  expectFrame(queue, 1);

  pushFrame(queue, 100);
  pushFrame(queue, 101);
  TEST_ASSERT_EQUAL_UINT(MAX_HELD, queue.count());

  for (uint8_t i = 2; i < MAX_HELD; i++) {
    expectFrame(queue, i);
  }
  expectFrame(queue, 100);
  expectFrame(queue, 101);
  TEST_ASSERT_TRUE(queue.isEmpty());
}

void test_indices_wrap_without_losing_order() {
  EspNowRxQueue queue;
  // Several laps around the ring, one in flight at a time.
  for (uint8_t i = 0; i < (uint8_t)(EspNowRxQueue::CAPACITY * 3); i++) {
    pushFrame(queue, i);
    expectFrame(queue, i);
  }
  TEST_ASSERT_TRUE(queue.isEmpty());
  TEST_ASSERT_EQUAL_UINT32(0, queue.droppedFrames());
}

void test_oversized_frame_is_rejected_and_counted() {
  EspNowRxQueue queue;
  uint8_t oversized[EspNowRxQueue::MAX_FRAME_BYTES + 1];
  memset(oversized, 0xAB, sizeof(oversized));

  TEST_ASSERT_FALSE(queue.push(oversized, sizeof(oversized)));
  TEST_ASSERT_TRUE(queue.isEmpty());
  TEST_ASSERT_EQUAL_UINT32(1, queue.droppedFrames());
}

void test_empty_and_null_frames_are_rejected() {
  EspNowRxQueue queue;
  uint8_t payload[4] = {1, 2, 3, 4};

  TEST_ASSERT_FALSE(queue.push(payload, 0));
  TEST_ASSERT_FALSE(queue.push(nullptr, 4));
  TEST_ASSERT_TRUE(queue.isEmpty());
  TEST_ASSERT_EQUAL_UINT32(2, queue.droppedFrames());
}

void test_maximum_sized_frame_round_trips() {
  EspNowRxQueue queue;
  uint8_t payload[EspNowRxQueue::MAX_FRAME_BYTES];
  for (size_t i = 0; i < sizeof(payload); i++) {
    payload[i] = (uint8_t)i;
  }

  TEST_ASSERT_TRUE(queue.push(payload, sizeof(payload)));

  EspNowRxQueue::Frame frame;
  TEST_ASSERT_TRUE(queue.pop(frame));
  TEST_ASSERT_EQUAL_UINT(sizeof(payload), frame.length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, frame.bytes, sizeof(payload));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_pop_returns_the_bytes_that_were_pushed);
  RUN_TEST(test_frames_are_drained_in_arrival_order);
  RUN_TEST(test_pop_on_empty_queue_reports_no_frame);
  RUN_TEST(test_full_queue_drops_the_newest_and_keeps_the_backlog);
  RUN_TEST(test_draining_frees_slots_for_later_frames);
  RUN_TEST(test_indices_wrap_without_losing_order);
  RUN_TEST(test_oversized_frame_is_rejected_and_counted);
  RUN_TEST(test_empty_and_null_frames_are_rejected);
  RUN_TEST(test_maximum_sized_frame_round_trips);
  return UNITY_END();
}
