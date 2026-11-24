#include "hubNowPlaying.h"

#include <cstdio>
#include <cstring>

// Before the hub's first StateSync sets it, the clock reads as the epoch.
static const time_t EARLIEST_SET_CLOCK = 1000000000;
static const int PERMILLE = 1000;
static const uint32_t SECONDS_PER_MINUTE = 60;
static const uint32_t SECONDS_PER_HOUR = 3600;
static const char* const UNKNOWN_TIME = "--:--";

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

omote_MediaType HubNowPlaying::mediaType() const {
  return playback.media_type;
}

HubNowPlaying::View HubNowPlaying::view(bool fetchesArtwork) const {
  const State current = state();
  const bool nothingPlaying = playback.title[0] == '\0' &&
                              (current == State::NONE || current == State::STOPPED);
  if (nothingPlaying) {
    return View::IDLE;
  }
  if (!fetchesArtwork || !playback.has_cover) {
    return View::TEXT_FORWARD;
  }
  return View::COVER;
}

uint32_t HubNowPlaying::coverWidth() const {
  return playback.cover.width;
}

uint32_t HubNowPlaying::coverHeight() const {
  return playback.cover.height;
}

const omote_Cover& HubNowPlaying::cover() const {
  return playback.cover;
}

bool HubNowPlaying::hasProgress() const {
  return playback.duration > 0;
}

bool HubNowPlaying::ticks() const {
  return hasProgress() && state() == State::PLAYING;
}

uint32_t HubNowPlaying::elapsedSeconds(time_t now) const {
  uint64_t elapsed = playback.position;
  const bool clockIsSet = now >= EARLIEST_SET_CLOCK;
  const bool readInstantKnown = playback.position_at > 0;
  const time_t readAt = static_cast<time_t>(playback.position_at);
  if (state() == State::PLAYING && clockIsSet && readInstantKnown && now > readAt) {
    elapsed += static_cast<uint64_t>(now - readAt);
  }
  if (hasProgress() && elapsed > playback.duration) {
    return playback.duration;
  }
  return static_cast<uint32_t>(elapsed);
}

int HubNowPlaying::progressPermille(time_t now) const {
  if (!hasProgress()) {
    return 0;
  }
  const uint64_t permille = static_cast<uint64_t>(elapsedSeconds(now)) * PERMILLE / playback.duration;
  return static_cast<int>(permille);
}

std::string HubNowPlaying::elapsedText(time_t now) const {
  // Position has no presence on the wire, so a 0 without a duration reads as unknown.
  const bool positionKnown = hasProgress() || playback.position > 0;
  if (!positionKnown) {
    return UNKNOWN_TIME;
  }
  return formatPlaybackTime(elapsedSeconds(now));
}

std::string HubNowPlaying::durationText() const {
  if (!hasProgress()) {
    return UNKNOWN_TIME;
  }
  return formatPlaybackTime(playback.duration);
}

std::string formatPlaybackTime(uint32_t seconds) {
  const uint32_t hours = seconds / SECONDS_PER_HOUR;
  const uint32_t minutes = (seconds % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE;
  const uint32_t secs = seconds % SECONDS_PER_MINUTE;
  char text[16];
  if (hours > 0) {
    snprintf(text, sizeof(text), "%u:%02u:%02u", (unsigned)hours, (unsigned)minutes, (unsigned)secs);
  } else {
    snprintf(text, sizeof(text), "%u:%02u", (unsigned)minutes, (unsigned)secs);
  }
  return text;
}

bool remoteFetchesArtwork(bool hasArtworkMemory, HubLink link) {
  const bool hasIpLink = link == HubLink::MQTT || link == HubLink::WEBSOCKET;
  return hasArtworkMemory && hasIpLink;
}
