#pragma once

#include <atomic>

// The main loop starts an HTTP GET into a buffer it owns, a background task runs it, and the main
// loop takes the result. The buffer belongs to the task from begin() until take() returns true.
class HttpGetHandover {
public:
  // Main loop.
  bool begin() {
    Stage idle = Stage::Idle;
    return stage_.compare_exchange_strong(idle, Stage::Fetching);
  }

  // Background task, once the body is in the buffer or the GET has failed.
  void finish(bool ok) { stage_.store(ok ? Stage::Succeeded : Stage::Failed); }

  // Main loop. False while the GET runs, and once a result has been taken.
  bool take(bool& ok) {
    const Stage stage = stage_.load();
    if (stage != Stage::Succeeded && stage != Stage::Failed) {
      return false;
    }
    ok = stage == Stage::Succeeded;
    stage_.store(Stage::Idle);
    return true;
  }

  bool inFlight() const { return stage_.load() == Stage::Fetching; }

private:
  enum class Stage { Idle, Fetching, Succeeded, Failed };

  std::atomic<Stage> stage_{Stage::Idle};
};
