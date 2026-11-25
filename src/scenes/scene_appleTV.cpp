#include <map>
#include "scenes/scene_appleTV.h"
#include "applicationInternal/keys.h"
#include "applicationInternal/scenes/sceneRegistry.h"
#include "applicationInternal/hardware/hardwarePresenter.h"
#include "applicationInternal/commandHandler.h"
#include "devices/mediaPlayer/device_appleTV/device_appleTV.h"
#include "devices/mediaPlayer/device_appleTV/gui_appleTV.h"
#include "devices/AVreceiver/device_denonAvr/device_denonAvr.h"
#include "devices/TV/device_lgTV/device_lgTV.h"
#include "devices/misc/device_hub_helper.h"
#include "guis/gui_numpad.h"

uint16_t SCENE_APPLETV;
uint16_t SCENE_APPLETV_FORCE;

std::map<char, repeatModes> key_repeatModes_appleTV;
std::map<char, uint16_t> key_commands_short_appleTV;
std::map<char, uint16_t> key_commands_long_appleTV;

void scene_setKeys_appleTV() {
  key_repeatModes_appleTV = {
      {KEY_UP, SHORT_REPEATED},    {KEY_LEFT, SHORT_REPEATED},
      {KEY_RIGHT, SHORT_REPEATED}, {KEY_DOWN, SHORT_REPEATED},
      {KEY_VOLDO, SHORT_REPEATED}, {KEY_VOLUP, SHORT_REPEATED},
  };

  key_commands_short_appleTV = {
      {KEY_STOP, APPLETV_STOP},       {KEY_REWI, APPLETV_SKIP_BACKWARD},
      {KEY_PLAY, APPLETV_PLAY_PAUSE}, {KEY_FORW, APPLETV_SKIP_FORWARD},
      {KEY_CONF, APPLETV_HOME},       {KEY_INFO, APPLETV_INFO},
      {KEY_UP, APPLETV_UP},           {KEY_LEFT, APPLETV_LEFT},
      {KEY_OK, APPLETV_SELECT},       {KEY_RIGHT, APPLETV_RIGHT},
      {KEY_DOWN, APPLETV_DOWN},       {KEY_BACK, APPLETV_MENU},
      {KEY_CHUP, COMMAND_UNKNOWN},    {KEY_REC, COMMAND_UNKNOWN},
      {KEY_CHDOW, COMMAND_UNKNOWN},   {KEY_VOLDO, DENONAVR_VOL_MINUS},
      {KEY_VOLUP, DENONAVR_VOL_PLUS}, {KEY_MUTE, DENONAVR_VOL_MUTE},
  };

  key_commands_long_appleTV = {};
}

void scene_start_sequence_appleTV(void) {
  #if (ENABLE_HUB_COMMUNICATION > 0)
  execute_hub_command(APPLETV_POWER_ON);
  execute_hub_command(DENONAVR_POWER_ON);
  execute_hub_command(DENONAVR_INPUT_APPLETV);
  execute_hub_command(LGTV_POWER_ON);
  #endif
}

void scene_end_sequence_appleTV(void) {
}

std::string scene_name_appleTV = "Apple TV";
t_gui_list scene_appleTV_gui_list = {tabName_appleTV, tabName_numpad};

void register_scene_appleTV(void) {
  register_command(&SCENE_APPLETV,       makeCommandData(SCENE, {scene_name_appleTV}));
  register_command(&SCENE_APPLETV_FORCE, makeCommandData(SCENE, {scene_name_appleTV, "FORCE"}));

  register_scene(
    scene_name_appleTV,
    & scene_setKeys_appleTV,
    & scene_start_sequence_appleTV,
    & scene_end_sequence_appleTV,
    & key_repeatModes_appleTV,
    & key_commands_short_appleTV,
    & key_commands_long_appleTV,
    & scene_appleTV_gui_list,
    SCENE_APPLETV);
}
