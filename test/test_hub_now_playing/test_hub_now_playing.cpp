#include <unity.h>
#include <cstring>
#include <hubNowPlaying.h>

void setUp() {}
void tearDown() {}

static omote_DeviceState makeSnapshotWithoutPlayback() {
  omote_DeviceState snapshot = omote_DeviceState_init_zero;
  strncpy(snapshot.device_id, "APPLE_TV", sizeof(snapshot.device_id) - 1);
  return snapshot;
}

static omote_DeviceState makePlayback(const char* title, const char* state) {
  omote_DeviceState snapshot = makeSnapshotWithoutPlayback();
  snapshot.has_media_player = true;
  snapshot.media_player.has_playback = true;
  strncpy(snapshot.media_player.playback.title, title,
          sizeof(snapshot.media_player.playback.title) - 1);
  strncpy(snapshot.media_player.playback.state, state,
          sizeof(snapshot.media_player.playback.state) - 1);
  return snapshot;
}

static omote_DeviceState makeTimedPlayback(uint32_t position, uint32_t duration) {
  omote_DeviceState snapshot = makePlayback("Blue in Green", "playing");
  snapshot.media_player.playback.position = position;
  snapshot.media_player.playback.duration = duration;
  return snapshot;
}

static HubNowPlaying::State stateFor(const char* wireState) {
  HubNowPlaying nowPlaying;
  nowPlaying.observe(makePlayback("Blue in Green", wireState));
  return nowPlaying.state();
}

void test_nothing_is_playing_before_the_first_snapshot() {
  HubNowPlaying nowPlaying;

  TEST_ASSERT_EQUAL_STRING("", nowPlaying.title().c_str());
  TEST_ASSERT_TRUE(HubNowPlaying::State::NONE == nowPlaying.state());
  TEST_ASSERT_FALSE(nowPlaying.hasProgress());
}

void test_a_snapshot_with_playback_reports_what_is_playing() {
  HubNowPlaying nowPlaying;
  omote_DeviceState snapshot = makePlayback("Blue in Green", "playing");
  strncpy(snapshot.media_player.playback.artist, "Miles Davis",
          sizeof(snapshot.media_player.playback.artist) - 1);
  strncpy(snapshot.media_player.playback.album, "Kind of Blue",
          sizeof(snapshot.media_player.playback.album) - 1);

  nowPlaying.observe(snapshot);

  TEST_ASSERT_EQUAL_STRING("Blue in Green", nowPlaying.title().c_str());
  TEST_ASSERT_EQUAL_STRING("Miles Davis", nowPlaying.artist().c_str());
  TEST_ASSERT_EQUAL_STRING("Kind of Blue", nowPlaying.album().c_str());
  TEST_ASSERT_TRUE(HubNowPlaying::State::PLAYING == nowPlaying.state());
}

void test_a_snapshot_without_playback_clears_what_was_playing() {
  HubNowPlaying nowPlaying;
  nowPlaying.observe(makeTimedPlayback(30, 120));

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

void test_progress_is_the_played_share_of_the_duration() {
  HubNowPlaying nowPlaying;

  nowPlaying.observe(makeTimedPlayback(30, 120));

  TEST_ASSERT_TRUE(nowPlaying.hasProgress());
  TEST_ASSERT_EQUAL_INT(25, nowPlaying.progressPercent());
}

void test_progress_is_hidden_when_the_duration_is_unknown() {
  HubNowPlaying nowPlaying;

  nowPlaying.observe(makeTimedPlayback(30, 0));

  TEST_ASSERT_FALSE(nowPlaying.hasProgress());
  TEST_ASSERT_EQUAL_INT(0, nowPlaying.progressPercent());
}

void test_progress_stops_at_the_end_when_the_position_overruns() {
  HubNowPlaying nowPlaying;

  nowPlaying.observe(makeTimedPlayback(150, 120));

  TEST_ASSERT_EQUAL_INT(100, nowPlaying.progressPercent());
}

void test_progress_survives_a_position_too_large_for_32_bit_math() {
  HubNowPlaying nowPlaying;

  nowPlaying.observe(makeTimedPlayback(3000000000u, 4000000000u));

  TEST_ASSERT_EQUAL_INT(75, nowPlaying.progressPercent());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_nothing_is_playing_before_the_first_snapshot);
  RUN_TEST(test_a_snapshot_with_playback_reports_what_is_playing);
  RUN_TEST(test_a_snapshot_without_playback_clears_what_was_playing);
  RUN_TEST(test_state_follows_the_hub_wire_string);
  RUN_TEST(test_idle_and_unknown_states_report_no_state);
  RUN_TEST(test_progress_is_the_played_share_of_the_duration);
  RUN_TEST(test_progress_is_hidden_when_the_duration_is_unknown);
  RUN_TEST(test_progress_stops_at_the_end_when_the_position_overruns);
  RUN_TEST(test_progress_survives_a_position_too_large_for_32_bit_math);
  return UNITY_END();
}
