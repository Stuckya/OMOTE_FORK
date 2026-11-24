#include "hubNowPlaying.h"

#include <cstdint>
#include <cstring>

void HubNowPlaying::observe(const omote_DeviceState& snapshot) {
  if (snapshot.has_media_player && snapshot.media_player.has_playback) {
    playback = snapshot.media_player.playback;
    return;
  }
  const omote_Metadata nothingPlaying = omote_Metadata_init_zero;
  playback = nothingPlaying;
}

std::string HubNowPlaying::title() const {
  return playback.title;
}

std::string HubNowPlaying::subtitle() const {
  return playback.subtitle;
}

std::string HubNowPlaying::caption() const {
  return playback.caption;
}

HubNowPlaying::State HubNowPlaying::state() const {
  struct WireState {
    const char* wire;
    State state;
  };
  static const WireState wireStates[] = {
      {"playing", State::PLAYING}, {"paused", State::PAUSED},   {"stopped", State::STOPPED},
      {"loading", State::LOADING}, {"seeking", State::SEEKING},
  };
  for (const WireState& known : wireStates) {
    if (strcmp(playback.state, known.wire) == 0) {
      return known.state;
    }
  }
  return State::NONE;
}

bool HubNowPlaying::hasProgress() const {
  return playback.duration > 0;
}

int HubNowPlaying::progressPercent() const {
  if (!hasProgress()) {
    return 0;
  }
  const uint64_t percent = static_cast<uint64_t>(playback.position) * 100 / playback.duration;
  return percent > 100 ? 100 : static_cast<int>(percent);
}
