// The firmware builds set this. Without it the command handlers have no hub kind.
#define ENABLE_HUB_COMMUNICATION 3

#include <unity.h>
#include "applicationInternal/startupCommandQueue.h"

void setUp() {}
void tearDown() {}

static CommandExecutionParams press(uint16_t commandId) {
  CommandExecutionParams params;
  params.commandId = commandId;
  return params;
}

static commandData command(commandHandlers handler, std::list<std::string> payloads) {
  commandData data = {handler, payloads};
  return data;
}

void test_a_hub_command_does_not_require_the_gui() {
  TEST_ASSERT_FALSE(requiresGui(command(HUB, {"denon_avr", "VOL_PLUS"})));
}

void test_a_scene_command_requires_the_gui() {
  TEST_ASSERT_TRUE(requiresGui(command(SCENE, {"Apple TV"})));
}

void test_a_gui_command_requires_the_gui() {
  TEST_ASSERT_TRUE(requiresGui(command(GUI, {"Settings"})));
}

void test_an_ir_command_does_not_require_the_gui() {
  TEST_ASSERT_FALSE(requiresGui(command(IR, {"4", "0x20DF10EF"})));
}

void test_a_hub_power_command_requires_the_gui_because_it_draws_its_progress() {
  TEST_ASSERT_TRUE(requiresGui(command(HUB, {"lg_tv", "POWER_ON"})));
  TEST_ASSERT_TRUE(requiresGui(command(HUB, {"lg_tv", "POWER_OFF"})));
}

void test_pending_commands_come_back_in_the_order_pushed() {
  StartupCommandQueue pendingCommands;
  pendingCommands.push(press(7));
  pendingCommands.push(press(8));

  const std::vector<CommandExecutionParams> taken = pendingCommands.takeAll();

  TEST_ASSERT_EQUAL(2, taken.size());
  TEST_ASSERT_EQUAL(7, taken[0].commandId);
  TEST_ASSERT_EQUAL(8, taken[1].commandId);
}

void test_pending_commands_come_back_only_once() {
  StartupCommandQueue pendingCommands;
  pendingCommands.push(press(7));
  pendingCommands.takeAll();

  TEST_ASSERT_EQUAL(0, pendingCommands.takeAll().size());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_a_hub_command_does_not_require_the_gui);
  RUN_TEST(test_a_scene_command_requires_the_gui);
  RUN_TEST(test_a_gui_command_requires_the_gui);
  RUN_TEST(test_an_ir_command_does_not_require_the_gui);
  RUN_TEST(test_a_hub_power_command_requires_the_gui_because_it_draws_its_progress);
  RUN_TEST(test_pending_commands_come_back_in_the_order_pushed);
  RUN_TEST(test_pending_commands_come_back_only_once);
  return UNITY_END();
}
