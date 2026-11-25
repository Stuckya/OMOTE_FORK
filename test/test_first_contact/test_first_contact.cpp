#include <unity.h>
#include <string.h>
#include <firstContact.h>

void setUp() {}
void tearDown() {}

const unsigned long JOINED_AT_MS = 187;
const int MAX_ASKS = 40;

struct Asks {
  unsigned long msAfterJoin[MAX_ASKS];
  int count;
};

// Polls once a millisecond, as the task that drives the socket does, with no answer to any ask.
Asks asksWithoutAnAnswer(FirstContact& contact, unsigned long fromMs, unsigned long untilMs) {
  Asks asks = {};
  for (unsigned long ms = fromMs; ms <= untilMs; ms++) {
    if (contact.next(JOINED_AT_MS + ms, false) == FirstContact::Step::AskForAddress && asks.count < MAX_ASKS) {
      asks.msAfterJoin[asks.count++] = ms;
    }
  }
  return asks;
}

void test_it_asks_for_the_address_as_soon_as_wifi_is_joined() {
  FirstContact contact;

  TEST_ASSERT_TRUE(contact.next(JOINED_AT_MS, false) == FirstContact::Step::AskForAddress);
}

void test_it_connects_once_the_address_is_known() {
  FirstContact contact;
  contact.next(JOINED_AT_MS, false);

  TEST_ASSERT_TRUE(contact.next(JOINED_AT_MS + 3, true) == FirstContact::Step::Connect);
}

void test_it_waits_after_an_ask() {
  FirstContact contact;
  contact.next(JOINED_AT_MS, false);

  TEST_ASSERT_TRUE(contact.next(JOINED_AT_MS + 1, false) == FirstContact::Step::Wait);
}

void test_early_asks_come_4_ms_apart() {
  FirstContact contact;

  const Asks asks = asksWithoutAnAnswer(contact, 0, 16);

  const unsigned long expected[] = {0, 4, 8, 12, 16};
  TEST_ASSERT_EQUAL_INT(5, asks.count);
  TEST_ASSERT_TRUE(memcmp(expected, asks.msAfterJoin, sizeof(expected)) == 0);
}

void test_later_asks_come_at_widening_gaps() {
  FirstContact contact;

  const Asks asks = asksWithoutAnAnswer(contact, 0, 999);

  // After 16 ms each gap is half as long again as the last, rounded down: 6, 9, 13, 19, 28, 42, ...
  const unsigned long expected[] = {0, 4, 8, 12, 16, 22, 31, 44, 63, 91, 133, 196, 290, 431, 642, 958};
  TEST_ASSERT_EQUAL_INT(16, asks.count);
  TEST_ASSERT_TRUE(memcmp(expected, asks.msAfterJoin, sizeof(expected)) == 0);
}

void test_it_connects_without_an_answer_after_one_second() {
  FirstContact contact;
  asksWithoutAnAnswer(contact, 0, 999);

  TEST_ASSERT_TRUE(contact.next(JOINED_AT_MS + 1000, false) == FirstContact::Step::Connect);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_it_asks_for_the_address_as_soon_as_wifi_is_joined);
  RUN_TEST(test_it_connects_once_the_address_is_known);
  RUN_TEST(test_it_waits_after_an_ask);
  RUN_TEST(test_early_asks_come_4_ms_apart);
  RUN_TEST(test_later_asks_come_at_widening_gaps);
  RUN_TEST(test_it_connects_without_an_answer_after_one_second);
  return UNITY_END();
}
