#include <unity.h>
#include <cstring>
#include <string>

#include "applicationInternal/hub/hubDeviceStateListeners.h"

static int appleTvDeliveries;
static std::string lastAppleTvDeviceId;
static int shieldDeliveries;

void setUp() {
  appleTvDeliveries = 0;
  lastAppleTvDeviceId.clear();
  shieldDeliveries = 0;
}
void tearDown() {}

static void recordAppleTv(const omote_DeviceState& snapshot) {
  appleTvDeliveries++;
  lastAppleTvDeviceId = snapshot.device_id;
}

static void recordShield(const omote_DeviceState&) {
  shieldDeliveries++;
}

static omote_DeviceState makeSnapshot(const char* id) {
  omote_DeviceState snapshot = omote_DeviceState_init_zero;
  strncpy(snapshot.device_id, id, sizeof(snapshot.device_id) - 1);
  return snapshot;
}

void test_a_listener_receives_its_own_devices_snapshots() {
  Hub::registerDeviceStateListener("APPLE_TV", recordAppleTv);

  Hub::notifyDeviceStateListener(makeSnapshot("APPLE_TV"));

  TEST_ASSERT_EQUAL_INT(1, appleTvDeliveries);
  TEST_ASSERT_EQUAL_STRING("APPLE_TV", lastAppleTvDeviceId.c_str());
}

void test_a_listener_does_not_receive_another_devices_snapshots() {
  Hub::registerDeviceStateListener("APPLE_TV", recordAppleTv);
  Hub::registerDeviceStateListener("SHIELD", recordShield);

  Hub::notifyDeviceStateListener(makeSnapshot("SHIELD"));

  TEST_ASSERT_EQUAL_INT(0, appleTvDeliveries);
  TEST_ASSERT_EQUAL_INT(1, shieldDeliveries);
}

void test_a_snapshot_for_a_device_without_a_listener_goes_nowhere() {
  Hub::registerDeviceStateListener("APPLE_TV", recordAppleTv);

  Hub::notifyDeviceStateListener(makeSnapshot("DENON_AVR"));

  TEST_ASSERT_EQUAL_INT(0, appleTvDeliveries);
  TEST_ASSERT_EQUAL_INT(0, shieldDeliveries);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_a_listener_receives_its_own_devices_snapshots);
  RUN_TEST(test_a_listener_does_not_receive_another_devices_snapshots);
  RUN_TEST(test_a_snapshot_for_a_device_without_a_listener_goes_nowhere);
  return UNITY_END();
}
