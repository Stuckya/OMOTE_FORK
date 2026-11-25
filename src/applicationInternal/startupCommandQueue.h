#pragma once

#include <iterator>
#include <vector>
#include "applicationInternal/commandHandler.h"
#if (ENABLE_HUB_COMMUNICATION > 0)
#include "applicationInternal/hub/powerStatus.h"
#include "applicationInternal/hub/protoCodec.h"
#endif

#if (ENABLE_HUB_COMMUNICATION > 0)
inline bool drawsPowerProgress(const commandData& hubCommand) {
  if (hubCommand.commandPayloads.size() < 2) {
    return false;
  }
  const std::string& commandName = *std::next(hubCommand.commandPayloads.begin());
  return Hub::PowerStatus::opensSession(Hub::ProtoCodec::stringToCommand(commandName));
}
#endif

inline bool requiresGui(const commandData& data) {
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

class StartupCommandQueue {
public:
  void push(const CommandExecutionParams& params) {
    pending_.push_back(params);
  }

  std::vector<CommandExecutionParams> takeAll() {
    std::vector<CommandExecutionParams> taken;
    taken.swap(pending_);
    return taken;
  }

private:
  std::vector<CommandExecutionParams> pending_;
};
