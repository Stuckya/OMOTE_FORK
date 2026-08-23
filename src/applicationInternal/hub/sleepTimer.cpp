#include "sleepTimer.h"

#include <lvgl.h>
#include <hubSleepTimer.h>

#include "applicationInternal/gui/components/sleepTimerChip.h"
#include "guis/gui_sceneSelection.h"
#include "applicationInternal/gui/guiNotification.h"
#include "applicationInternal/hardware/arduinoLayer.h"
#include "applicationInternal/hub/hubDeviceNames.h"
#include "applicationInternal/hub/hubManager.h"
#include "applicationInternal/hub/protoCodec.h"

namespace Hub {
namespace SleepTimer {

// One second is enough: the sheet shows MM:SS and the chip whole minutes.
static const uint32_t TICK_MS = 1000;
static const uint16_t EXTEND_MINUTES = 15;

static HubSleepTimer timer;
static lv_timer_t* tickTimer = nullptr;
static RepaintCallback repaint = nullptr;

static HubSleepTimer::Stage stageFromWire(omote_SleepTimerStage stage) {
  switch (stage) {
    case omote_SleepTimerStage_SLEEP_TIMER_STAGE_ARMED:
      return HubSleepTimer::Stage::ARMED;
    case omote_SleepTimerStage_SLEEP_TIMER_STAGE_WARNING:
      return HubSleepTimer::Stage::WARNING;
    case omote_SleepTimerStage_SLEEP_TIMER_STAGE_FIRED:
      return HubSleepTimer::Stage::FIRED;
    default:
      // Unset and unknown future stages read as "no timer", so an older or
      // newer hub leaves no countdown stranded on screen.
      return HubSleepTimer::Stage::IDLE;
  }
}

static void render() {
  setLabelSleepTimer(timer.isArmed() ? timer.indicatorText(millis()) : std::string());
  gui_sceneSelection_refreshSleepTimerRow();
  if (repaint != nullptr) {
    repaint();
  }
}

static void stopTicking() {
  if (tickTimer == nullptr) {
    return;
  }
  lv_timer_del(tickTimer);
  tickTimer = nullptr;
}

static void tick(lv_timer_t*) {
  render();
  if (!timer.isArmed()) {
    stopTicking();
  }
}

static void startTicking() {
  if (tickTimer == nullptr) {
    tickTimer = lv_timer_create(tick, TICK_MS, nullptr);
  }
}

static void send(omote_OmoteCommand command, uint16_t minutes) {
  omote_RemoteEvent event = ProtoCodec::createSleepTimerEvent(command, minutes);
  HubManager::getInstance().sendRemoteEvent(event);
}

void arm(uint16_t minutes) {
  send(omote_OmoteCommand_SLEEP_TIMER_SET, HubSleepTimer::snapMinutes(minutes));
}

void extendByDefault() {
  send(omote_OmoteCommand_SLEEP_TIMER_EXTEND, EXTEND_MINUTES);
}

void cancel() {
  send(omote_OmoteCommand_SLEEP_TIMER_CANCEL, 0);
}

bool observed(const omote_SleepTimerStatus& status) {
  timer.observe(stageFromWire(status.stage), status.remaining_seconds, millis());

  const bool fired = timer.takeFired();
  if (timer.takeWarning()) {
    GuiNotification::showSleepWarning();
  }
  render();
  if (timer.isArmed()) {
    startTicking();
  } else {
    stopTicking();
  }
  return fired;
}

bool isArmed() { return timer.isArmed(); }

std::string indicatorText() {
  return timer.isArmed() ? timer.indicatorText(millis()) : std::string();
}

std::string countdownText() { return timer.countdownText(millis()); }

uint32_t remainingSeconds() { return timer.remainingSeconds(millis()); }

void setRepaintCallback(RepaintCallback callback) { repaint = callback; }

}  // namespace SleepTimer
}  // namespace Hub
