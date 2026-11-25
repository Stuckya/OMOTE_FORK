#include <unity.h>
#include <cstring>
#include <hubCoverArt.h>

void setUp() {}
void tearDown() {}

static const char* ART_URL = "http://hub:8080/artwork/APPLE_TV/a1";
static const char* NEXT_URL = "http://hub:8080/artwork/APPLE_TV/b2";

static omote_Cover makeCover(omote_CoverArt art, const char* url = "") {
  omote_Cover cover = omote_Cover_init_zero;
  cover.width = 200;
  cover.height = 113;
  cover.art = art;
  strncpy(cover.url, url, sizeof(cover.url) - 1);
  return cover;
}

static HubCoverArt observed(const omote_Cover& cover) {
  HubCoverArt art;
  art.observe(cover);
  return art;
}

void test_a_placeholder_cover_shows_the_glyph() {
  TEST_ASSERT_TRUE(HubCoverArt::Fill::GLYPH ==
                   observed(makeCover(omote_CoverArt_COVER_ART_PLACEHOLDER)).fill());
}

void test_an_unspecified_cover_shows_the_glyph() {
  TEST_ASSERT_TRUE(HubCoverArt::Fill::GLYPH ==
                   observed(makeCover(omote_CoverArt_COVER_ART_UNSPECIFIED)).fill());
}

void test_art_the_hub_is_still_fetching_shows_a_blank_tile() {
  HubCoverArt art = observed(makeCover(omote_CoverArt_COVER_ART_LOADING));

  TEST_ASSERT_TRUE(HubCoverArt::Fill::BLANK == art.fill());
  TEST_ASSERT_FALSE(art.shouldFetch());
}

void test_ready_art_is_fetched_once_at_the_cover_size() {
  HubCoverArt art = observed(makeCover(omote_CoverArt_COVER_ART_READY, ART_URL));

  TEST_ASSERT_TRUE(art.shouldFetch());
  TEST_ASSERT_EQUAL_STRING(ART_URL, art.url().c_str());
  TEST_ASSERT_EQUAL_UINT32(200 * 113 * 2, art.expectedBytes());
  TEST_ASSERT_TRUE(HubCoverArt::Fill::BLANK == art.fill());

  art.fetchStarted();

  TEST_ASSERT_FALSE(art.shouldFetch());
  TEST_ASSERT_TRUE(art.fetchInFlight());
}

void test_fetched_art_fills_the_cover() {
  HubCoverArt art = observed(makeCover(omote_CoverArt_COVER_ART_READY, ART_URL));
  art.fetchStarted();

  art.fetchFinished(true);

  TEST_ASSERT_TRUE(HubCoverArt::Fill::ART == art.fill());
  TEST_ASSERT_FALSE(art.shouldFetch());
}

void test_a_failed_fetch_settles_on_the_glyph_without_retrying() {
  HubCoverArt art = observed(makeCover(omote_CoverArt_COVER_ART_READY, ART_URL));
  art.fetchStarted();

  art.fetchFinished(false);
  art.observe(makeCover(omote_CoverArt_COVER_ART_READY, ART_URL));

  TEST_ASSERT_TRUE(HubCoverArt::Fill::GLYPH == art.fill());
  TEST_ASSERT_FALSE(art.shouldFetch());
}

void test_new_art_replaces_the_shown_art_once_fetched() {
  HubCoverArt art = observed(makeCover(omote_CoverArt_COVER_ART_READY, ART_URL));
  art.fetchStarted();
  art.fetchFinished(true);

  art.observe(makeCover(omote_CoverArt_COVER_ART_READY, NEXT_URL));

  TEST_ASSERT_TRUE(HubCoverArt::Fill::BLANK == art.fill());
  TEST_ASSERT_TRUE(art.shouldFetch());
}

void test_art_that_changed_during_a_fetch_is_fetched_next() {
  HubCoverArt art = observed(makeCover(omote_CoverArt_COVER_ART_READY, ART_URL));
  art.fetchStarted();
  art.observe(makeCover(omote_CoverArt_COVER_ART_READY, NEXT_URL));

  TEST_ASSERT_FALSE(art.shouldFetch());

  art.fetchFinished(true);

  TEST_ASSERT_TRUE(HubCoverArt::Fill::BLANK == art.fill());
  TEST_ASSERT_TRUE(art.shouldFetch());
  TEST_ASSERT_EQUAL_STRING(NEXT_URL, art.url().c_str());
}

void test_returning_to_fetched_art_shows_it_without_a_new_fetch() {
  HubCoverArt art = observed(makeCover(omote_CoverArt_COVER_ART_READY, ART_URL));
  art.fetchStarted();
  art.fetchFinished(true);
  art.observe(makeCover(omote_CoverArt_COVER_ART_PLACEHOLDER));

  art.observe(makeCover(omote_CoverArt_COVER_ART_READY, ART_URL));

  TEST_ASSERT_TRUE(HubCoverArt::Fill::ART == art.fill());
  TEST_ASSERT_FALSE(art.shouldFetch());
}

void test_ready_art_without_a_url_is_never_fetched() {
  HubCoverArt art = observed(makeCover(omote_CoverArt_COVER_ART_READY, ""));

  TEST_ASSERT_FALSE(art.shouldFetch());
  TEST_ASSERT_TRUE(HubCoverArt::Fill::GLYPH == art.fill());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_a_placeholder_cover_shows_the_glyph);
  RUN_TEST(test_an_unspecified_cover_shows_the_glyph);
  RUN_TEST(test_art_the_hub_is_still_fetching_shows_a_blank_tile);
  RUN_TEST(test_ready_art_is_fetched_once_at_the_cover_size);
  RUN_TEST(test_fetched_art_fills_the_cover);
  RUN_TEST(test_a_failed_fetch_settles_on_the_glyph_without_retrying);
  RUN_TEST(test_new_art_replaces_the_shown_art_once_fetched);
  RUN_TEST(test_art_that_changed_during_a_fetch_is_fetched_next);
  RUN_TEST(test_returning_to_fetched_art_shows_it_without_a_new_fetch);
  RUN_TEST(test_ready_art_without_a_url_is_never_fetched);
  return UNITY_END();
}
