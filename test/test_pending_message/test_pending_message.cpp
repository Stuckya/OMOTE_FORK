#include <unity.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <pendingMessage.h>

void setUp() {}
void tearDown() {}

struct RecordingSocket {
  bool accepts = true;
  int sends = 0;
  std::vector<uint8_t> lastBytes;

  bool send(const uint8_t* data, size_t len) {
    sends++;
    lastBytes.assign(data, data + len);
    return accepts;
  }
};

static void openSocket(PendingMessage& message, RecordingSocket& socket) {
  message.sendIfWaiting([&socket](const uint8_t* data, size_t len) { return socket.send(data, len); });
}

static const uint8_t VOLUME_UP[] = {0x08, 0x05, 0x10, 0x01};
static const uint8_t VOLUME_DOWN[] = {0x08, 0x06, 0x10, 0x01, 0x18};

void test_a_message_set_before_the_connect_is_sent_when_the_socket_opens() {
  PendingMessage message;
  RecordingSocket socket;

  TEST_ASSERT_TRUE(message.set(VOLUME_UP, sizeof(VOLUME_UP)));
  openSocket(message, socket);

  TEST_ASSERT_EQUAL_INT(1, socket.sends);
  TEST_ASSERT_EQUAL_UINT(sizeof(VOLUME_UP), socket.lastBytes.size());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(VOLUME_UP, socket.lastBytes.data(), sizeof(VOLUME_UP));
  TEST_ASSERT_TRUE(message.wasSent());
}

void test_nothing_is_sent_when_no_message_was_set() {
  PendingMessage message;
  RecordingSocket socket;

  openSocket(message, socket);

  TEST_ASSERT_EQUAL_INT(0, socket.sends);
  TEST_ASSERT_FALSE(message.wasSent());
}

void test_a_message_is_sent_only_once() {
  PendingMessage message;
  RecordingSocket socket;
  message.set(VOLUME_UP, sizeof(VOLUME_UP));

  openSocket(message, socket);
  openSocket(message, socket);

  TEST_ASSERT_EQUAL_INT(1, socket.sends);
}

void test_only_the_first_message_is_taken() {
  PendingMessage message;
  RecordingSocket socket;
  message.set(VOLUME_UP, sizeof(VOLUME_UP));

  TEST_ASSERT_FALSE(message.set(VOLUME_DOWN, sizeof(VOLUME_DOWN)));
  openSocket(message, socket);

  TEST_ASSERT_EQUAL_UINT(sizeof(VOLUME_UP), socket.lastBytes.size());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(VOLUME_UP, socket.lastBytes.data(), sizeof(VOLUME_UP));
}

void test_a_message_is_not_sent_before_the_socket_opens() {
  PendingMessage message;

  message.set(VOLUME_UP, sizeof(VOLUME_UP));

  TEST_ASSERT_FALSE(message.wasSent());
}

void test_a_failed_send_reads_as_not_sent() {
  PendingMessage message;
  RecordingSocket socket;
  socket.accepts = false;
  message.set(VOLUME_UP, sizeof(VOLUME_UP));

  openSocket(message, socket);

  TEST_ASSERT_EQUAL_INT(1, socket.sends);
  TEST_ASSERT_FALSE(message.wasSent());
}

void test_a_message_that_does_not_fit_is_not_taken() {
  PendingMessage message;
  RecordingSocket socket;
  const std::vector<uint8_t> tooLong(PendingMessage::MAX_BYTES + 1, 0x2A);

  TEST_ASSERT_FALSE(message.set(tooLong.data(), tooLong.size()));
  openSocket(message, socket);

  TEST_ASSERT_EQUAL_INT(0, socket.sends);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_a_message_set_before_the_connect_is_sent_when_the_socket_opens);
  RUN_TEST(test_nothing_is_sent_when_no_message_was_set);
  RUN_TEST(test_a_message_is_sent_only_once);
  RUN_TEST(test_only_the_first_message_is_taken);
  RUN_TEST(test_a_message_is_not_sent_before_the_socket_opens);
  RUN_TEST(test_a_failed_send_reads_as_not_sent);
  RUN_TEST(test_a_message_that_does_not_fit_is_not_taken);
  return UNITY_END();
}
