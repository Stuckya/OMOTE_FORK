#include <unity.h>
#include <cstring>
#include <hubNowPlaying.h>

void setUp() {}
void tearDown() {}

static const time_t HUB_READ_AT = 1795000000;

static omote_DeviceState makeSnapshotWithoutPlayback() {
  omote_DeviceState snapshot = omote_DeviceState_init_zero;
  strncpy(snapshot.device_id, "APPLE_TV", sizeof(snapshot.device_id) - 1);
  return snapshot;
}

static omote_DeviceState makePlayback(const char* title, const char* state) {
  omote_DeviceState snapshot = makeSnapshotWithoutPlayback();
  snapshot.has_media_player = true;
  snapshot.media_player.has_playback = true;
  omote_Metadata& playback = snapshot.media_player.playback;
  strncpy(playback.title, title, sizeof(playback.title) - 1);
  strncpy(playback.state, state, sizeof(playback.state) - 1);
  playback.has_cover = true;
  playback.cover.width = 148;
  playback.cover.height = 148;
  return snapshot;
}

static omote_DeviceState makeTimed(const char* state, uint32_t position, uint32_t duration) {
  omote_DeviceState snapshot = makePlayback("Midnight City", state);
  snapshot.media_player.playback.position = position;
  snapshot.media_player.playback.duration = duration;
  snapshot.media_player.playback.position_at = HUB_READ_AT;
  return snapshot;
}

static HubNowPlaying observed(const omote_DeviceState& snapshot) {
  HubNowPlaying nowPlaying;
  nowPlaying.observe(snapshot);
  return nowPlaying;
}

static HubNowPlaying::State stateFor(const char* wireState) {
  return observed(makePlayback("Midnight City", wireState)).state();
}

void test_nothing_is_playing_before_the_first_snapshot() {
  HubNowPlaying nowPlaying;

  TEST_ASSERT_EQUAL_STRING("", nowPlaying.title().c_str());
  TEST_ASSERT_TRUE(HubNowPlaying::State::NONE == nowPlaying.state());
  TEST_ASSERT_FALSE(nowPlaying.hasProgress());
}

void test_a_snapshot_with_playback_reports_what_is_playing() {
  HubNowPlaying nowPlaying;
  omote_DeviceState snapshot = makePlayback("Midnight City", "playing");
  strncpy(snapshot.media_player.playback.subtitle, "M83",
          sizeof(snapshot.media_player.playback.subtitle) - 1);
  strncpy(snapshot.media_player.playback.caption, "Hurry Up, We're Dreaming",
          sizeof(snapshot.media_player.playback.caption) - 1);

  nowPlaying.observe(snapshot);

  TEST_ASSERT_EQUAL_STRING("Midnight City", nowPlaying.title().c_str());
  TEST_ASSERT_EQUAL_STRING("M83", nowPlaying.subtitle().c_str());
  TEST_ASSERT_EQUAL_STRING("Hurry Up, We're Dreaming", nowPlaying.caption().c_str());
  TEST_ASSERT_TRUE(HubNowPlaying::State::PLAYING == nowPlaying.state());
}

void test_a_snapshot_without_playback_clears_what_was_playing() {
  HubNowPlaying nowPlaying;
  nowPlaying.observe(makeTimed("playing", 30, 120));

  nowPlaying.observe(makeSnapshotWithoutPlayback());

  TEST_ASSERT_EQUAL_STRING("", nowPlaying.title().c_str());
  TEST_ASSERT_TRUE(HubNowPlaying::State::NONE == nowPlaying.state());
  TEST_ASSERT_FALSE(nowPlaying.hasProgress());
}

void test_state_follows_the_hub_wire_string() {
  TEST_ASSERT_TRUE(HubNowPlaying::State::PLAYING == stateFor("playing"));
  TEST_ASSERT_TRUE(HubNowPlaying::State::PAUSED == stateFor("paused"));
  TEST_ASSERT_TRUE(HubNowPlaying::State::STOPPED == stateFor("stopped"));
  TEST_ASSERT_TRUE(HubNowPlaying::State::LOADING == stateFor("loading"));
  TEST_ASSERT_TRUE(HubNowPlaying::State::SEEKING == stateFor("seeking"));
}

void test_idle_and_unknown_states_report_no_state() {
  TEST_ASSERT_TRUE(HubNowPlaying::State::NONE == stateFor("idle"));
  TEST_ASSERT_TRUE(HubNowPlaying::State::NONE == stateFor(""));
  TEST_ASSERT_TRUE(HubNowPlaying::State::NONE == stateFor("buffering"));
}

void test_nothing_observed_shows_the_idle_view() {
  HubNowPlaying nowPlaying;

  TEST_ASSERT_TRUE(HubNowPlaying::View::IDLE == nowPlaying.view(true));
}

void test_a_snapshot_without_playback_shows_the_idle_view() {
  HubNowPlaying nowPlaying = observed(makePlayback("Midnight City", "playing"));

  nowPlaying.observe(makeSnapshotWithoutPlayback());

  TEST_ASSERT_TRUE(HubNowPlaying::View::IDLE == nowPlaying.view(true));
}

void test_an_untitled_idle_player_shows_the_idle_view() {
  HubNowPlaying nowPlaying = observed(makePlayback("", "idle"));

  TEST_ASSERT_TRUE(HubNowPlaying::View::IDLE == nowPlaying.view(true));
}

void test_playback_shows_the_cover_when_the_remote_fetches_art() {
  HubNowPlaying nowPlaying = observed(makePlayback("Midnight City", "playing"));

  TEST_ASSERT_TRUE(HubNowPlaying::View::COVER == nowPlaying.view(true));
  TEST_ASSERT_EQUAL_UINT32(148, nowPlaying.coverWidth());
  TEST_ASSERT_EQUAL_UINT32(148, nowPlaying.coverHeight());
}

void test_playback_promotes_the_title_when_the_remote_cannot_fetch_art() {
  HubNowPlaying nowPlaying = observed(makePlayback("Midnight City", "playing"));

  TEST_ASSERT_TRUE(HubNowPlaying::View::TEXT_FORWARD == nowPlaying.view(false));
}

void test_playback_without_a_cover_promotes_the_title() {
  omote_DeviceState snapshot = makePlayback("Midnight City", "playing");
  snapshot.media_player.playback.has_cover = false;

  TEST_ASSERT_TRUE(HubNowPlaying::View::TEXT_FORWARD == observed(snapshot).view(true));
}

void test_playing_advances_from_the_hub_read_instant() {
  HubNowPlaying nowPlaying = observed(makeTimed("playing", 124, 244));

  TEST_ASSERT_EQUAL_UINT32(124, nowPlaying.elapsedSeconds(HUB_READ_AT));
  TEST_ASSERT_EQUAL_UINT32(134, nowPlaying.elapsedSeconds(HUB_READ_AT + 10));
}

void test_paused_playback_holds_its_position() {
  HubNowPlaying nowPlaying = observed(makeTimed("paused", 124, 244));

  TEST_ASSERT_EQUAL_UINT32(124, nowPlaying.elapsedSeconds(HUB_READ_AT + 60));
}

void test_elapsed_stops_at_the_duration() {
  HubNowPlaying nowPlaying = observed(makeTimed("playing", 240, 244));

  TEST_ASSERT_EQUAL_UINT32(244, nowPlaying.elapsedSeconds(HUB_READ_AT + 60));
}

void test_elapsed_holds_until_the_clock_is_set() {
  HubNowPlaying nowPlaying = observed(makeTimed("playing", 124, 244));

  TEST_ASSERT_EQUAL_UINT32(124, nowPlaying.elapsedSeconds(42));
}

void test_elapsed_holds_without_a_read_instant() {
  omote_DeviceState snapshot = makeTimed("playing", 124, 244);
  snapshot.media_player.playback.position_at = 0;

  TEST_ASSERT_EQUAL_UINT32(124, observed(snapshot).elapsedSeconds(HUB_READ_AT + 10));
}

void test_a_clock_behind_the_hub_does_not_rewind() {
  HubNowPlaying nowPlaying = observed(makeTimed("playing", 124, 244));

  TEST_ASSERT_EQUAL_UINT32(124, nowPlaying.elapsedSeconds(HUB_READ_AT - 5));
}

void test_only_timed_playback_ticks() {
  TEST_ASSERT_TRUE(observed(makeTimed("playing", 0, 244)).ticks());
  TEST_ASSERT_FALSE(observed(makeTimed("paused", 0, 244)).ticks());
  TEST_ASSERT_FALSE(observed(makeTimed("playing", 0, 0)).ticks());
}

void test_progress_is_the_elapsed_share_in_permille() {
  HubNowPlaying nowPlaying = observed(makeTimed("playing", 61, 244));

  TEST_ASSERT_EQUAL_INT(250, nowPlaying.progressPermille(HUB_READ_AT));
  TEST_ASSERT_EQUAL_INT(1000, nowPlaying.progressPermille(HUB_READ_AT + 1000));
}

void test_progress_survives_values_too_large_for_32_bit_math() {
  HubNowPlaying nowPlaying = observed(makeTimed("paused", 3000000000u, 4000000000u));

  TEST_ASSERT_EQUAL_INT(750, nowPlaying.progressPermille(HUB_READ_AT));
}

void test_times_read_as_minutes_or_hours() {
  TEST_ASSERT_EQUAL_STRING("0:00", formatPlaybackTime(0).c_str());
  TEST_ASSERT_EQUAL_STRING("2:04", formatPlaybackTime(124).c_str());
  TEST_ASSERT_EQUAL_STRING("59:59", formatPlaybackTime(3599).c_str());
  TEST_ASSERT_EQUAL_STRING("1:12:30", formatPlaybackTime(4350).c_str());
}

void test_elapsed_and_duration_text_follow_the_clock() {
  HubNowPlaying nowPlaying = observed(makeTimed("playing", 124, 244));

  TEST_ASSERT_EQUAL_STRING("2:14", nowPlaying.elapsedText(HUB_READ_AT + 10).c_str());
  TEST_ASSERT_EQUAL_STRING("4:04", nowPlaying.durationText().c_str());
}

void test_media_type_follows_the_hub() {
  omote_DeviceState snapshot = makePlayback("Midnight City", "playing");
  snapshot.media_player.playback.media_type = omote_MediaType_MEDIA_TYPE_MUSIC;

  TEST_ASSERT_EQUAL(omote_MediaType_MEDIA_TYPE_MUSIC, observed(snapshot).mediaType());
}

void test_art_needs_an_ip_link_and_memory_for_a_cover() {
  TEST_ASSERT_TRUE(remoteFetchesArtwork(true, HubLink::WEBSOCKET));
  TEST_ASSERT_TRUE(remoteFetchesArtwork(true, HubLink::MQTT));
  TEST_ASSERT_FALSE(remoteFetchesArtwork(true, HubLink::ESP_NOW));
  TEST_ASSERT_FALSE(remoteFetchesArtwork(true, HubLink::NONE));
  TEST_ASSERT_FALSE(remoteFetchesArtwork(false, HubLink::WEBSOCKET));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_nothing_is_playing_before_the_first_snapshot);
  RUN_TEST(test_a_snapshot_with_playback_reports_what_is_playing);
  RUN_TEST(test_a_snapshot_without_playback_clears_what_was_playing);
  RUN_TEST(test_state_follows_the_hub_wire_string);
  RUN_TEST(test_idle_and_unknown_states_report_no_state);
  RUN_TEST(test_nothing_observed_shows_the_idle_view);
  RUN_TEST(test_a_snapshot_without_playback_shows_the_idle_view);
  RUN_TEST(test_an_untitled_idle_player_shows_the_idle_view);
  RUN_TEST(test_playback_shows_the_cover_when_the_remote_fetches_art);
  RUN_TEST(test_playback_promotes_the_title_when_the_remote_cannot_fetch_art);
  RUN_TEST(test_playback_without_a_cover_promotes_the_title);
  RUN_TEST(test_playing_advances_from_the_hub_read_instant);
  RUN_TEST(test_paused_playback_holds_its_position);
  RUN_TEST(test_elapsed_stops_at_the_duration);
  RUN_TEST(test_elapsed_holds_until_the_clock_is_set);
  RUN_TEST(test_elapsed_holds_without_a_read_instant);
  RUN_TEST(test_a_clock_behind_the_hub_does_not_rewind);
  RUN_TEST(test_only_timed_playback_ticks);
  RUN_TEST(test_progress_is_the_elapsed_share_in_permille);
  RUN_TEST(test_progress_survives_values_too_large_for_32_bit_math);
  RUN_TEST(test_times_read_as_minutes_or_hours);
  RUN_TEST(test_elapsed_and_duration_text_follow_the_clock);
  RUN_TEST(test_media_type_follows_the_hub);
  RUN_TEST(test_art_needs_an_ip_link_and_memory_for_a_cover);
  return UNITY_END();
}
