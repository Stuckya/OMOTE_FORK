#pragma once

#include "remote_messages.pb.h"
#include <cstdint>
#include <ctime>
#include <string>

// The remote's view of what one hub media device is playing. The hub sends a
// device's state as a complete snapshot, so a snapshot without playback means
// nothing is playing any more. The hub composes the lines and the cover size;
// this class only picks the layout that shows them and keeps the clock moving.
class HubNowPlaying {
public:
  // NONE covers idle and any state this firmware does not know.
  enum class State { NONE, PLAYING, PAUSED, STOPPED, LOADING, SEEKING };

  // TEXT_FORWARD promotes the title when this remote cannot fetch art or the
  // hub sent no cover.
  enum class View { IDLE, COVER, TEXT_FORWARD };

  void observe(const omote_DeviceState& snapshot);

  std::string title() const;
  std::string subtitle() const;
  std::string caption() const;
  State state() const;
  omote_MediaType mediaType() const;
  View view(bool fetchesArtwork) const;
  uint32_t coverWidth() const;
  uint32_t coverHeight() const;

  // False when the hub does not know the duration.
  bool hasProgress() const;
  bool ticks() const;
  // Advances the position from the instant the hub read it, once the clock has
  // been set from the hub; never past the duration.
  uint32_t elapsedSeconds(time_t now) const;
  int progressPermille(time_t now) const;
  // "--:--" for a time the hub does not know.
  std::string elapsedText(time_t now) const;
  std::string durationText() const;

private:
  omote_Metadata playback = omote_Metadata_init_zero;
};

// "2:04", or "1:12:30" from an hour up.
std::string formatPlaybackTime(uint32_t seconds);

enum class HubLink { NONE, ESP_NOW, MQTT, WEBSOCKET };

// Art travels as an HTTP GET, so it needs an IP link to the hub and room for a
// cover-sized buffer.
bool remoteFetchesArtwork(bool hasArtworkMemory, HubLink link);
