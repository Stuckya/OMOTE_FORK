#include <unity.h>
#include <cstring>
#include <hubCoverArt.h>

void setUp() {}
void tearDown() {}

static const char* ART_URL = "http://hub:8080/artwork/APPLE_TV/a1";
static const char* BLUR = "LEHV6nWB2yk8pyo0adR*.7kCMdnj";

static omote_Cover makeCover(omote_CoverArt art, const char* url, const char* blurhash) {
  omote_Cover cover = omote_Cover_init_zero;
  cover.width = 200;
  cover.height = 113;
  cover.art = art;
  strncpy(cover.url, url, sizeof(cover.url) - 1);
  strncpy(cover.blurhash, blurhash, sizeof(cover.blurhash) - 1);
  return cover;
}

static HubCoverArt observed(const omote_Cover& cover) {
  HubCoverArt art;
  art.observe(cover);
  return art;
}

void test_ready_art_shows_its_blurhash_while_the_remote_fetches() {
  HubCoverArt art = observed(makeCover(omote_CoverArt_COVER_ART_READY, ART_URL, BLUR));
  art.fetchStarted();

  TEST_ASSERT_TRUE(HubCoverArt::Fill::BLURHASH == art.fill());
  TEST_ASSERT_EQUAL_STRING(BLUR, art.blurhash().c_str());
}

void test_fetched_art_replaces_the_blurhash() {
  HubCoverArt art = observed(makeCover(omote_CoverArt_COVER_ART_READY, ART_URL, BLUR));
  art.fetchStarted();

  art.fetchFinished(true);

  TEST_ASSERT_TRUE(HubCoverArt::Fill::ART == art.fill());
}

void test_a_failed_fetch_drops_the_blurhash_for_the_glyph() {
  HubCoverArt art = observed(makeCover(omote_CoverArt_COVER_ART_READY, ART_URL, BLUR));
  art.fetchStarted();

  art.fetchFinished(false);

  TEST_ASSERT_TRUE(HubCoverArt::Fill::GLYPH == art.fill());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_ready_art_shows_its_blurhash_while_the_remote_fetches);
  RUN_TEST(test_fetched_art_replaces_the_blurhash);
  RUN_TEST(test_a_failed_fetch_drops_the_blurhash_for_the_glyph);
  return UNITY_END();
}
