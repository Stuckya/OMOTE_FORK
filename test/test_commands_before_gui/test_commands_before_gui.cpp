// The firmware builds set this. Without it the command handlers have no hub kind.
#define ENABLE_HUB_COMMUNICATION 3

#include <unity.h>
#include "applicationInternal/commandsBeforeGui.h"

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

void test_a_hub_command_runs_before_the_gui_exists() {
  CommandsBeforeGui commands;

  TEST_ASSERT_TRUE(commands.mayRunNow(press(1), command(HUB, {"denon_avr", "VOL_PLUS"})));
}

void test_a_scene_command_waits_for_the_gui() {
  CommandsBeforeGui commands;

  TEST_ASSERT_FALSE(commands.mayRunNow(press(1), command(SCENE, {"Apple TV"})));
}

void test_a_gui_command_waits_for_the_gui() {
  CommandsBeforeGui commands;

  TEST_ASSERT_FALSE(commands.mayRunNow(press(1), command(GUI, {"Settings"})));
}

void test_an_ir_command_runs_before_the_gui_exists() {
  CommandsBeforeGui commands;

  TEST_ASSERT_TRUE(commands.mayRunNow(press(1), command(IR, {"4", "0x20DF10EF"})));
}

void test_a_hub_power_command_waits_because_it_draws_its_progress() {
  CommandsBeforeGui commands;

  TEST_ASSERT_FALSE(commands.mayRunNow(press(1), command(HUB, {"lg_tv", "POWER_ON"})));
  TEST_ASSERT_FALSE(commands.mayRunNow(press(2), command(HUB, {"lg_tv", "POWER_OFF"})));
}

void test_commands_that_waited_come_back_in_order_once_the_gui_exists() {
  CommandsBeforeGui commands;
  commands.mayRunNow(press(7), command(SCENE, {"Apple TV"}));
  commands.mayRunNow(press(8), command(GUI, {"Settings"}));

  const std::vector<CommandExecutionParams> waited = commands.guiExists();

  TEST_ASSERT_EQUAL(2, waited.size());
  TEST_ASSERT_EQUAL(7, waited[0].commandId);
  TEST_ASSERT_EQUAL(8, waited[1].commandId);
}

void test_every_command_runs_at_once_after_the_gui_exists() {
  CommandsBeforeGui commands;
  commands.guiExists();

  TEST_ASSERT_TRUE(commands.mayRunNow(press(1), command(SCENE, {"Apple TV"})));
  TEST_ASSERT_TRUE(commands.mayRunNow(press(2), command(HUB, {"lg_tv", "POWER_ON"})));
}

void test_commands_that_waited_come_back_only_once() {
  CommandsBeforeGui commands;
  commands.mayRunNow(press(7), command(SCENE, {"Apple TV"}));
  commands.guiExists();

  TEST_ASSERT_EQUAL(0, commands.guiExists().size());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_a_hub_command_runs_before_the_gui_exists);
  RUN_TEST(test_a_scene_command_waits_for_the_gui);
  RUN_TEST(test_a_gui_command_waits_for_the_gui);
  RUN_TEST(test_an_ir_command_runs_before_the_gui_exists);
  RUN_TEST(test_a_hub_power_command_waits_because_it_draws_its_progress);
  RUN_TEST(test_commands_that_waited_come_back_in_order_once_the_gui_exists);
  RUN_TEST(test_every_command_runs_at_once_after_the_gui_exists);
  RUN_TEST(test_commands_that_waited_come_back_only_once);
  return UNITY_END();
}
