#include <unity.h>
#include <cstddef>

#define ENABLE_WIFI_AND_MQTT 1
#include "applicationInternal/hub/mqttHubTransport.h"
#include "applicationInternal/hub/syncRequest.h"

void setUp() {}
void tearDown() {}

// Each field below costs a tag byte and, for a message, a length prefix of at most 2 bytes.
static const size_t FIELD_OVERHEAD = 3;

// The largest one-device response or push the hub sends: CommandResult.kind, then a StateSync
// holding the time, one DeviceState and the sleep timer.
static const size_t WORST_SINGLE_DEVICE_COMMAND_RESULT =
    FIELD_OVERHEAD + FIELD_OVERHEAD + (FIELD_OVERHEAD + omote_Time_size) +
    (FIELD_OVERHEAD + omote_DeviceState_size) + (FIELD_OVERHEAD + omote_SleepTimerStatus_size);

void test_mqtt_carries_the_single_device_sync_it_asks_for() {
  const size_t inbound = MqttHubTransport::INBOUND_COMMAND_RESULT_BYTES;

  TEST_ASSERT_GREATER_OR_EQUAL_UINT(Hub::STATE_SYNC_SINGLE_DEVICE_BUDGET, inbound);
}

void test_mqtt_carries_a_worst_case_single_device_push() {
  const size_t inbound = MqttHubTransport::INBOUND_COMMAND_RESULT_BYTES;

  TEST_ASSERT_GREATER_OR_EQUAL_UINT(WORST_SINGLE_DEVICE_COMMAND_RESULT, inbound);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_mqtt_carries_the_single_device_sync_it_asks_for);
  RUN_TEST(test_mqtt_carries_a_worst_case_single_device_push);
  return UNITY_END();
}
