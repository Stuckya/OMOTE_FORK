#include "hubCoverArt.h"

static const size_t RGB565_BYTES_PER_PIXEL = 2;

void HubCoverArt::observe(const omote_Cover& cover) {
  art = cover.art;
  currentUrl = cover.url;
  width = cover.width;
  height = cover.height;
}

bool HubCoverArt::artIsReady() const {
  return art == omote_CoverArt_COVER_ART_READY && !currentUrl.empty();
}

HubCoverArt::Fill HubCoverArt::fill() const {
  if (art == omote_CoverArt_COVER_ART_LOADING) {
    return Fill::BLANK;
  }
  if (!artIsReady() || currentUrl == failedUrl) {
    return Fill::GLYPH;
  }
  return currentUrl == fetchedUrl ? Fill::ART : Fill::BLANK;
}

bool HubCoverArt::shouldFetch() const {
  return artIsReady() && !fetchInFlight() && currentUrl != fetchedUrl && currentUrl != failedUrl;
}

const std::string& HubCoverArt::url() const {
  return currentUrl;
}

size_t HubCoverArt::expectedBytes() const {
  return width * height * RGB565_BYTES_PER_PIXEL;
}

bool HubCoverArt::fetchInFlight() const {
  return !inFlightUrl.empty();
}

void HubCoverArt::fetchStarted() {
  inFlightUrl = currentUrl;
}

void HubCoverArt::fetchFinished(bool ok) {
  if (ok) {
    fetchedUrl = inFlightUrl;
  } else {
    failedUrl = inFlightUrl;
  }
  inFlightUrl.clear();
}
