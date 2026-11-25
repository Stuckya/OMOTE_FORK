#pragma once

#include "remote_messages.pb.h"
#include <cstddef>
#include <string>

// What the now-playing cover shows, and when the remote fetches its art. One art at a time: the
// remote fetches the hub's current url once, and a failed url stays on the placeholder.
class HubCoverArt {
public:
  // GLYPH is the placeholder; BLANK waits for art the hub or the remote is still fetching.
  enum class Fill { GLYPH, BLANK, ART };

  void observe(const omote_Cover& cover);

  Fill fill() const;
  bool shouldFetch() const;
  const std::string& url() const;
  size_t expectedBytes() const;
  bool fetchInFlight() const;

  void fetchStarted();
  void fetchFinished(bool ok);

private:
  bool artIsReady() const;

  omote_CoverArt art = omote_CoverArt_COVER_ART_UNSPECIFIED;
  std::string currentUrl;
  size_t width = 0;
  size_t height = 0;
  std::string inFlightUrl;
  std::string fetchedUrl;
  std::string failedUrl;
};
