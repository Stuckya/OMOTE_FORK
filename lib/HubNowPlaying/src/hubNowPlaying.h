#pragma once

#include "remote_messages.pb.h"
#include <string>

// The remote's view of what one hub media device is playing. The hub sends a
// device's state as a complete snapshot, so a snapshot without playback means
// nothing is playing any more.
class HubNowPlaying {
public:
  // NONE covers idle and any state this firmware does not know.
  enum class State { NONE, PLAYING, PAUSED, STOPPED, LOADING, SEEKING };

  void observe(const omote_DeviceState& snapshot);

  std::string title() const;
  std::string subtitle() const;
  std::string caption() const;
  State state() const;

  // False when the hub does not know the duration.
  bool hasProgress() const;
  int progressPercent() const;

private:
  omote_Metadata playback = omote_Metadata_init_zero;
};
