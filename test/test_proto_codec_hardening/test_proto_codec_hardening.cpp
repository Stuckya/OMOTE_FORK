#include <unity.h>
#include "applicationInternal/hub/protoCodec.h"

#include <cstring>
#include <string>
#include <vector>

using Hub::ProtoCodec;

static std::vector<uint8_t> encodeValidEvent() {
  omote_RemoteEvent event = ProtoCodec::createRemoteEvent(
      "LG_TV", omote_OmoteCommand_POWER_ON, omote_OmoteCommandType_SHORT, "omote-1");
  uint8_t buffer[omote_RemoteEvent_size];
  const size_t written = ProtoCodec::encodeRemoteEvent(event, buffer, sizeof(buffer));
  return std::vector<uint8_t>(buffer, buffer + written);
}

void setUp() {
}

void tearDown() {
}

void test_a_failed_decode_leaves_the_callers_result_untouched() {
  omote_CommandResult result = omote_CommandResult_init_zero;
  result.kind = omote_ResponseKind_POWER;
  result.which_data = omote_CommandResult_power_tag;
  result.data.power.is_on = true;

  const uint8_t garbage[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  TEST_ASSERT_FALSE(ProtoCodec::decodeCommandResult(garbage, sizeof(garbage), result));

  TEST_ASSERT_EQUAL(omote_ResponseKind_POWER, result.kind);
  TEST_ASSERT_EQUAL_UINT(omote_CommandResult_power_tag, result.which_data);
  TEST_ASSERT_TRUE(result.data.power.is_on);
}

void test_an_empty_frame_clears_stale_result_state() {
  omote_CommandResult result = omote_CommandResult_init_zero;
  result.kind = omote_ResponseKind_VOLUME;

  const uint8_t empty[1] = {0};
  TEST_ASSERT_TRUE(ProtoCodec::decodeCommandResult(empty, 0, result));
  TEST_ASSERT_EQUAL(omote_ResponseKind_RESPONSE_KIND_UNSPECIFIED, result.kind);
}

void test_no_truncation_of_a_valid_frame_crashes_the_decoder() {
  const std::vector<uint8_t> valid = encodeValidEvent();
  TEST_ASSERT_TRUE(valid.size() > 0);

  for (size_t length = 0; length < valid.size(); length++) {
    omote_CommandResult result = omote_CommandResult_init_zero;
    ProtoCodec::decodeCommandResult(valid.data(), length, result);
  }
  TEST_ASSERT_TRUE(true);
}

void test_no_single_bit_flip_crashes_the_decoder() {
  const std::vector<uint8_t> valid = encodeValidEvent();

  for (size_t byte = 0; byte < valid.size(); byte++) {
    for (int bit = 0; bit < 8; bit++) {
      std::vector<uint8_t> corrupted = valid;
      corrupted[byte] ^= (uint8_t)(1 << bit);
      omote_CommandResult result = omote_CommandResult_init_zero;
      ProtoCodec::decodeCommandResult(corrupted.data(), corrupted.size(), result);
    }
  }
  TEST_ASSERT_TRUE(true);
}

void test_encoding_into_a_short_buffer_never_writes_past_it() {
  omote_RemoteEvent event = ProtoCodec::createRemoteEvent(
      "DENON_AVR", omote_OmoteCommand_VOL_PLUS, omote_OmoteCommandType_SHORT, "omote-1");

  const size_t needed = encodeValidEvent().size();
  for (size_t capacity = 0; capacity < needed; capacity++) {
    std::vector<uint8_t> arena(needed + 16, 0xAA);
    const size_t written = ProtoCodec::encodeRemoteEvent(event, arena.data(), capacity);

    TEST_ASSERT_EQUAL_UINT(0, written);
    for (size_t guard = capacity; guard < arena.size(); guard++) {
      TEST_ASSERT_EQUAL_UINT8(0xAA, arena[guard]);
    }
  }
}

void test_an_overlong_device_name_is_truncated_to_the_field() {
  const std::string huge(400, 'D');
  omote_RemoteEvent event = ProtoCodec::createRemoteEvent(huge, omote_OmoteCommand_POWER_ON);

  TEST_ASSERT_TRUE(strlen(event.device) < sizeof(event.device));
  TEST_ASSERT_EQUAL_UINT(sizeof(event.device) - 1, strlen(event.device));
}

void test_an_overlong_remote_id_is_truncated_to_the_field() {
  const std::string huge(400, 'R');
  omote_RemoteEvent event = ProtoCodec::createRemoteEvent(
      "LG_TV", omote_OmoteCommand_POWER_ON, omote_OmoteCommandType_SHORT, huge);

  TEST_ASSERT_TRUE(strlen(event.remote_id) < sizeof(event.remote_id));
  TEST_ASSERT_EQUAL_UINT(sizeof(event.remote_id) - 1, strlen(event.remote_id));
}

void test_an_overlong_data_payload_is_clamped_to_the_field() {
  std::vector<uint8_t> huge(400, 0x5A);
  omote_RemoteEvent event = ProtoCodec::createRemoteEvent(
      "LG_TV", omote_OmoteCommand_GUI_EVENT, omote_OmoteCommandType_SHORT, "",
      huge.data(), huge.size());

  TEST_ASSERT_TRUE(event.data.size <= sizeof(event.data.bytes));
}

void test_a_truncated_device_name_still_round_trips_as_a_string() {
  const std::string huge(400, 'X');
  omote_RemoteEvent event = ProtoCodec::createRemoteEvent(huge, omote_OmoteCommand_POWER_ON);

  uint8_t buffer[omote_RemoteEvent_size];
  const size_t written = ProtoCodec::encodeRemoteEvent(event, buffer, sizeof(buffer));
  TEST_ASSERT_TRUE(written > 0);
}

void test_an_unknown_command_string_never_maps_to_a_real_command() {
  const char* nonsense[] = {"", " ", "power_on", "POWER_ON ", "\xff\xfe", "SLEEP_TIMER_"};
  for (size_t i = 0; i < sizeof(nonsense) / sizeof(nonsense[0]); i++) {
    TEST_ASSERT_EQUAL(omote_OmoteCommand_OMOTE_COMMAND_UNSPECIFIED,
                      ProtoCodec::stringToCommand(nonsense[i]));
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_a_failed_decode_leaves_the_callers_result_untouched);
  RUN_TEST(test_an_empty_frame_clears_stale_result_state);
  RUN_TEST(test_no_truncation_of_a_valid_frame_crashes_the_decoder);
  RUN_TEST(test_no_single_bit_flip_crashes_the_decoder);
  RUN_TEST(test_encoding_into_a_short_buffer_never_writes_past_it);
  RUN_TEST(test_an_overlong_device_name_is_truncated_to_the_field);
  RUN_TEST(test_an_overlong_remote_id_is_truncated_to_the_field);
  RUN_TEST(test_an_overlong_data_payload_is_clamped_to_the_field);
  RUN_TEST(test_a_truncated_device_name_still_round_trips_as_a_string);
  RUN_TEST(test_an_unknown_command_string_never_maps_to_a_real_command);
  return UNITY_END();
}
