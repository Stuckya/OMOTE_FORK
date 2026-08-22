#pragma once

#include <string>
#include <vector>

namespace Hub {

// Priority-ordered hub device ids a scene composes, primary first. The primary
// is the richest single-device snapshot (the media device: power + now-playing),
// so a constrained transport that can only carry one device syncs that one.
// Scenes that drive no hub device return empty -> time-only sync.
inline std::vector<std::string> hubSyncTargetsForScene(const std::string& sceneName) {
  if (sceneName == "Shield") {
    return {"ANDROID_TV", "DENON_AVR", "LG_TV"};
  }
  if (sceneName == "TV") {
    return {"LG_TV"};
  }
  return {};
}

// The hub device a scene's volume keys drive — not necessarily its primary
// sync target (Shield's primary is the media device; VOLUP/MUTE go to the AVR).
// Empty when the scene's volume is not hub-driven.
inline std::string hubVolumeDeviceForScene(const std::string& sceneName) {
  if (sceneName == "Shield") {
    return "DENON_AVR";
  }
  return "";
}

}  // namespace Hub
