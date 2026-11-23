#include <unity.h>
#include <socketHandover.h>

void setUp() {}
void tearDown() {}

void test_the_main_loop_holds_a_socket_that_was_never_lent() {
  SocketHandover handover;

  TEST_ASSERT_TRUE(handover.mainLoopHoldsSocket());
}

void test_the_main_loop_does_not_hold_a_socket_lent_to_the_background() {
  SocketHandover handover;

  TEST_ASSERT_TRUE(handover.lendToBackground());

  TEST_ASSERT_FALSE(handover.mainLoopHoldsSocket());
}

void test_a_lent_socket_cannot_be_lent_again() {
  SocketHandover handover;
  handover.lendToBackground();

  TEST_ASSERT_FALSE(handover.lendToBackground());
}

void test_the_background_keeps_connecting_while_the_socket_is_not_connected() {
  SocketHandover handover;
  handover.lendToBackground();

  TEST_ASSERT_TRUE(handover.backgroundShouldKeepConnecting(false));
}

void test_the_background_stops_once_the_socket_is_connected() {
  SocketHandover handover;
  handover.lendToBackground();

  TEST_ASSERT_FALSE(handover.backgroundShouldKeepConnecting(true));
}

void test_the_background_stops_when_the_main_loop_asks_for_the_socket() {
  SocketHandover handover;
  handover.lendToBackground();

  handover.askForSocketBack();

  TEST_ASSERT_FALSE(handover.backgroundShouldKeepConnecting(false));
}

void test_the_main_loop_holds_the_socket_again_once_it_is_given_back() {
  SocketHandover handover;
  handover.lendToBackground();

  handover.giveBack();

  TEST_ASSERT_TRUE(handover.mainLoopHoldsSocket());
}

void test_a_socket_that_came_back_is_not_lent_a_second_time() {
  SocketHandover handover;
  handover.lendToBackground();
  handover.giveBack();

  TEST_ASSERT_FALSE(handover.lendToBackground());
  TEST_ASSERT_TRUE(handover.mainLoopHoldsSocket());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_the_main_loop_holds_a_socket_that_was_never_lent);
  RUN_TEST(test_the_main_loop_does_not_hold_a_socket_lent_to_the_background);
  RUN_TEST(test_a_lent_socket_cannot_be_lent_again);
  RUN_TEST(test_the_background_keeps_connecting_while_the_socket_is_not_connected);
  RUN_TEST(test_the_background_stops_once_the_socket_is_connected);
  RUN_TEST(test_the_background_stops_when_the_main_loop_asks_for_the_socket);
  RUN_TEST(test_the_main_loop_holds_the_socket_again_once_it_is_given_back);
  RUN_TEST(test_a_socket_that_came_back_is_not_lent_a_second_time);
  return UNITY_END();
}
