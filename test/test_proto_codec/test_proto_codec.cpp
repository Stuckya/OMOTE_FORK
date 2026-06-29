#include <unity.h>
#include <cstring>

#include "applicationInternal/hub/protoCodec.h"

using Hub::ProtoCodec;

void setUp() {}
void tearDown() {}

static omote_CommandResult makeCommandResult(omote_ResponseKind kind, pb_size_t dataTag) {
  omote_CommandResult result = omote_CommandResult_init_zero;
  memset(&result.data, 0, sizeof(result.data));
  result.kind = kind;
  result.which_data = dataTag;
  return result;
}

static void assertMaps(const char* cmd, omote_OmoteCommand expected) {
  TEST_ASSERT_EQUAL(expected, ProtoCodec::stringToCommand(cmd));
}

void test_synonyms_map_to_existing_enums() {
  assertMaps("SOURCE", omote_OmoteCommand_SRC);
  assertMaps("MUTE_TOGGLE", omote_OmoteCommand_VOL_MUTE);
  assertMaps("RETURN", omote_OmoteCommand_BACK);
  assertMaps("EXIT", omote_OmoteCommand_BACK);
  assertMaps("KEY_A", omote_OmoteCommand_RED);
  assertMaps("KEY_B", omote_OmoteCommand_GREEN);
  assertMaps("KEY_C", omote_OmoteCommand_YELLOW);
  assertMaps("KEY_D", omote_OmoteCommand_BLUE);
}

void test_existing_mappings_still_hold() {
  assertMaps("SRC", omote_OmoteCommand_SRC);
  assertMaps("BACK", omote_OmoteCommand_BACK);
  assertMaps("VOL_MUTE", omote_OmoteCommand_VOL_MUTE);
  assertMaps("POWER_ON", omote_OmoteCommand_POWER_ON);
}

void test_unknown_command_is_unspecified() {
  assertMaps("NOPE", omote_OmoteCommand_OMOTE_COMMAND_UNSPECIFIED);
  assertMaps("", omote_OmoteCommand_OMOTE_COMMAND_UNSPECIFIED);
}

void test_valid_command_result_round_trips() {
  omote_CommandResult source = makeCommandResult(omote_ResponseKind_VOLUME, omote_CommandResult_volume_tag);
  source.data.volume.level = -30.5f;
  source.data.volume.is_muted = true;

  uint8_t buffer[omote_CommandResult_size];
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  TEST_ASSERT_TRUE(pb_encode(&stream, omote_CommandResult_fields, &source));

  omote_CommandResult decoded = omote_CommandResult_init_zero;
  TEST_ASSERT_TRUE(ProtoCodec::decodeCommandResult(buffer, stream.bytes_written, decoded));

  TEST_ASSERT_EQUAL(omote_ResponseKind_VOLUME, decoded.kind);
  TEST_ASSERT_EQUAL(omote_CommandResult_volume_tag, decoded.which_data);
  TEST_ASSERT_EQUAL_FLOAT(-30.5f, decoded.data.volume.level);
  TEST_ASSERT_TRUE(decoded.data.volume.is_muted);
}

void test_pairing_command_result_round_trips_enum_step() {
  omote_CommandResult source = makeCommandResult(omote_ResponseKind_PAIRING, omote_CommandResult_pairing_tag);
  strncpy(source.data.pairing.device_id, "AppleTV", sizeof(source.data.pairing.device_id) - 1);
  source.data.pairing.step = omote_PairingStep_PAIRING_STEP_AWAITING_PIN;
  strncpy(source.data.pairing.message, "Enter PIN", sizeof(source.data.pairing.message) - 1);
  source.data.pairing.requires_pin = true;
  source.data.pairing.expected_pin_length = 4;
  strncpy(source.data.pairing.context_token, "ctx", sizeof(source.data.pairing.context_token) - 1);

  uint8_t buffer[omote_CommandResult_size];
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  TEST_ASSERT_TRUE(pb_encode(&stream, omote_CommandResult_fields, &source));

  omote_CommandResult decoded = omote_CommandResult_init_zero;
  TEST_ASSERT_TRUE(ProtoCodec::decodeCommandResult(buffer, stream.bytes_written, decoded));

  TEST_ASSERT_EQUAL(omote_ResponseKind_PAIRING, decoded.kind);
  TEST_ASSERT_EQUAL(omote_CommandResult_pairing_tag, decoded.which_data);
  TEST_ASSERT_EQUAL_STRING("AppleTV", decoded.data.pairing.device_id);
  TEST_ASSERT_EQUAL(omote_PairingStep_PAIRING_STEP_AWAITING_PIN, decoded.data.pairing.step);
  TEST_ASSERT_EQUAL_STRING("Enter PIN", decoded.data.pairing.message);
  TEST_ASSERT_TRUE(decoded.data.pairing.requires_pin);
  TEST_ASSERT_EQUAL_UINT32(4, decoded.data.pairing.expected_pin_length);
  TEST_ASSERT_EQUAL_STRING("ctx", decoded.data.pairing.context_token);
}

void test_device_list_round_trips_with_full_entries() {
  omote_CommandResult source = makeCommandResult(omote_ResponseKind_DEVICE_LIST, omote_CommandResult_device_list_tag);
  source.data.device_list.devices_count = 2;
  source.data.device_list.scan_complete = true;

  omote_DeviceInfo& living = source.data.device_list.devices[0];
  strncpy(living.device_id, "APPLE_TV_LIVING", sizeof(living.device_id) - 1);
  strncpy(living.name, "Living Room", sizeof(living.name) - 1);
  strncpy(living.address, "10.0.0.4", sizeof(living.address) - 1);
  strncpy(living.model, "Apple TV 4K", sizeof(living.model) - 1);
  living.pairing = omote_PairingRequirement_PAIRING_REQUIREMENT_MANDATORY;
  living.paired = true;
  living.paired_at = 1747008000;

  omote_DeviceInfo& kitchen = source.data.device_list.devices[1];
  strncpy(kitchen.device_id, "APPLE_TV_KITCHEN", sizeof(kitchen.device_id) - 1);
  strncpy(kitchen.name, "Kitchen", sizeof(kitchen.name) - 1);
  kitchen.pairing = omote_PairingRequirement_PAIRING_REQUIREMENT_DISABLED;

  uint8_t buffer[omote_CommandResult_size];
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  TEST_ASSERT_TRUE(pb_encode(&stream, omote_CommandResult_fields, &source));

  omote_CommandResult decoded = omote_CommandResult_init_zero;
  TEST_ASSERT_TRUE(ProtoCodec::decodeCommandResult(buffer, stream.bytes_written, decoded));

  TEST_ASSERT_EQUAL(omote_ResponseKind_DEVICE_LIST, decoded.kind);
  TEST_ASSERT_EQUAL(omote_CommandResult_device_list_tag, decoded.which_data);
  TEST_ASSERT_EQUAL_UINT(2, decoded.data.device_list.devices_count);
  TEST_ASSERT_TRUE(decoded.data.device_list.scan_complete);
  const omote_DeviceInfo& d0 = decoded.data.device_list.devices[0];
  TEST_ASSERT_EQUAL_STRING("APPLE_TV_LIVING", d0.device_id);
  TEST_ASSERT_EQUAL_STRING("Living Room", d0.name);
  TEST_ASSERT_EQUAL_STRING("10.0.0.4", d0.address);
  TEST_ASSERT_EQUAL_STRING("Apple TV 4K", d0.model);
  TEST_ASSERT_EQUAL(omote_PairingRequirement_PAIRING_REQUIREMENT_MANDATORY, d0.pairing);
  TEST_ASSERT_TRUE(d0.paired);
  TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(d0.paired_at >> 32));
  TEST_ASSERT_EQUAL_UINT32(1747008000UL, static_cast<uint32_t>(d0.paired_at));
  const omote_DeviceInfo& d1 = decoded.data.device_list.devices[1];
  TEST_ASSERT_EQUAL_STRING("APPLE_TV_KITCHEN", d1.device_id);
  TEST_ASSERT_EQUAL(omote_PairingRequirement_PAIRING_REQUIREMENT_DISABLED, d1.pairing);
  TEST_ASSERT_FALSE(d1.paired);
}

// The wire caps are the contract the hub truncates against (max_size includes
// the NUL): at-cap entries must round-trip, proving the firmware can decode
// everything a cap-respecting hub will ever send.
void test_device_list_at_cap_fields_round_trip() {
  omote_CommandResult source = makeCommandResult(omote_ResponseKind_DEVICE_LIST, omote_CommandResult_device_list_tag);
  source.data.device_list.devices_count =
      sizeof(source.data.device_list.devices) / sizeof(source.data.device_list.devices[0]);

  for (pb_size_t i = 0; i < source.data.device_list.devices_count; ++i) {
    omote_DeviceInfo& device = source.data.device_list.devices[i];
    memset(device.device_id, 'i', sizeof(device.device_id) - 1);
    memset(device.name, 'n', sizeof(device.name) - 1);
    memset(device.address, 'a', sizeof(device.address) - 1);
    memset(device.model, 'm', sizeof(device.model) - 1);
  }

  uint8_t buffer[omote_CommandResult_size];
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  TEST_ASSERT_TRUE(pb_encode(&stream, omote_CommandResult_fields, &source));

  omote_CommandResult decoded = omote_CommandResult_init_zero;
  TEST_ASSERT_TRUE(ProtoCodec::decodeCommandResult(buffer, stream.bytes_written, decoded));

  TEST_ASSERT_EQUAL_UINT(source.data.device_list.devices_count, decoded.data.device_list.devices_count);
  TEST_ASSERT_EQUAL_UINT(sizeof(source.data.device_list.devices[0].device_id) - 1,
                         strlen(decoded.data.device_list.devices[0].device_id));
  TEST_ASSERT_EQUAL_UINT(sizeof(source.data.device_list.devices[0].name) - 1,
                         strlen(decoded.data.device_list.devices[0].name));
  TEST_ASSERT_EQUAL_UINT(sizeof(source.data.device_list.devices[0].address) - 1,
                         strlen(decoded.data.device_list.devices[0].address));
  TEST_ASSERT_EQUAL_UINT(sizeof(source.data.device_list.devices[0].model) - 1,
                         strlen(decoded.data.device_list.devices[0].model));
}

void test_garbage_bytes_fail_to_decode() {
  const uint8_t garbage[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  omote_CommandResult decoded = omote_CommandResult_init_zero;

  TEST_ASSERT_FALSE(ProtoCodec::decodeCommandResult(garbage, sizeof(garbage), decoded));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_synonyms_map_to_existing_enums);
  RUN_TEST(test_existing_mappings_still_hold);
  RUN_TEST(test_unknown_command_is_unspecified);
  RUN_TEST(test_valid_command_result_round_trips);
  RUN_TEST(test_pairing_command_result_round_trips_enum_step);
  RUN_TEST(test_device_list_round_trips_with_full_entries);
  RUN_TEST(test_device_list_at_cap_fields_round_trip);
  RUN_TEST(test_garbage_bytes_fail_to_decode);
  return UNITY_END();
}
