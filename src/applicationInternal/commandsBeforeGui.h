#pragma once

#include <iterator>
#include <vector>
#include "applicationInternal/commandHandler.h"
#if (ENABLE_HUB_COMMUNICATION > 0)
#include "applicationInternal/hub/powerStatus.h"
#include "applicationInternal/hub/protoCodec.h"
#endif

// A key can be handled before the GUI is built. A command that draws waits here until the GUI exists.
class CommandsBeforeGui {
public:
  // True: run the command now. False: it was kept, and guiExists() hands it back.
  bool mayRunNow(const CommandExecutionParams& params, const commandData& data) {
    if (guiExists_ || !draws(data)) {
      return true;
    }
    waiting_.push_back(params);
    return false;
  }

  std::vector<CommandExecutionParams> guiExists() {
    guiExists_ = true;
    std::vector<CommandExecutionParams> waited;
    waited.swap(waiting_);
    return waited;
  }

private:
  bool guiExists_ = false;
  std::vector<CommandExecutionParams> waiting_;

  static bool draws(const commandData& data) {
    if (data.commandHandler == IR) {
      return false;
    }
    #if (ENABLE_HUB_COMMUNICATION > 0)
    if (data.commandHandler == HUB) {
      return drawsPowerProgress(data);
    }
    #endif
    return true;
  }

  #if (ENABLE_HUB_COMMUNICATION > 0)
  static bool drawsPowerProgress(const commandData& hubCommand) {
    if (hubCommand.commandPayloads.size() < 2) {
      return false;
    }
    const std::string& commandName = *std::next(hubCommand.commandPayloads.begin());
    return Hub::PowerStatus::opensSession(Hub::ProtoCodec::stringToCommand(commandName));
  }
  #endif
};
