#pragma once

#include <cstdint>
#include <string>
#include "remote_messages.pb.h"

// Arms, extends, and cancels the hub's sleep timer, and keeps the remote's view
// of it fresh between pushes.
namespace Hub {
namespace SleepTimer {

void arm(uint16_t minutes);
void extendByDefault();
void cancel();

// Folds in a pushed status. Returns true when the hub has just fired, which
// is the caller's cue to open the power session for the devices going off.
bool observed(const omote_SleepTimerStatus& status);

bool isArmed();
// Still inside the final minute; asked before replaying a deferred warning.
bool isWarning();
// "42m" for the status bar chip; empty when nothing is armed.
std::string indicatorText();
// "41:32" for the armed sheet.
std::string countdownText();
uint32_t remainingSeconds();

// Redraws whatever the sheet is showing; registered by the sheet while open.
typedef void (*RepaintCallback)();
void setRepaintCallback(RepaintCallback callback);

}  // namespace SleepTimer
}  // namespace Hub
