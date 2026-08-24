#include "powerStatus.h"
#include <lvgl.h>
#include <hubPowerSession.h>
#include "applicationInternal/gui/guiNotification.h"
#include "applicationInternal/hardware/arduinoLayer.h"
#include "applicationInternal/hub/hubDeviceNames.h"

namespace Hub {
namespace PowerStatus {

static const uint32_t TICK_MS = 250;
// Keeps a finished session's identity long enough for the banner's final hold,
// so the hub's late anonymous power replies stay folded into it.
static const unsigned long LINGER_MS = 4000;

static HubPowerSession session;
static lv_timer_t* tickTimer = nullptr;
static unsigned long finishedAt = 0;

static bool inProgress() {
  return session.phase() == HubPowerSession::Phase::IN_PROGRESS;
}

static void render() {
  GuiNotification::showPowerSession(session);
}

static void stopTicking() {
  if (!tickTimer) {
    return;
  }
  lv_timer_del(tickTimer);
  tickTimer = nullptr;
}

static void tick(lv_timer_t*) {
  const unsigned long now = millis();
  const HubPowerSession::Phase before = session.phase();
  if (session.tick(now) != before) {
    finishedAt = now;
    render();
  }
  if (inProgress()) {
    return;
  }
  if (now - finishedAt < LINGER_MS) {
    return;
  }
  session.reset();
  stopTicking();
}

static void startTicking() {
  if (!tickTimer) {
    tickTimer = lv_timer_create(tick, TICK_MS, nullptr);
  }
}

void commandSent(const std::string& deviceId, omote_OmoteCommand command,
                 const omote_DeviceState* cached) {
  if (command != omote_OmoteCommand_POWER_ON && command != omote_OmoteCommand_POWER_OFF) {
    return;
  }
  const bool targetOn = command == omote_OmoteCommand_POWER_ON;
  const bool alreadyThere = cached != nullptr && cached->is_on == targetOn;
  const unsigned long now = millis();

  session.expect(deviceId, hubDeviceDisplayName(deviceId), targetOn, alreadyThere, now);
  finishedAt = now;
  render();
  startTicking();
}

void commandTransmitted(omote_OmoteCommand command) {
  if (command != omote_OmoteCommand_POWER_ON && command != omote_OmoteCommand_POWER_OFF) {
    return;
  }
  session.noteTransmitted(millis());
}

void observed(const omote_DeviceState& state) {
  if (!session.observe(state.device_id, state.is_on)) {
    return;
  }
  finishedAt = millis();
  render();
}

bool noteError(const std::string& message) {
  if (!session.noteError(message)) {
    return false;
  }
  render();
  return true;
}

bool isActive() {
  return session.phase() != HubPowerSession::Phase::IDLE;
}

}  // namespace PowerStatus
}  // namespace Hub
