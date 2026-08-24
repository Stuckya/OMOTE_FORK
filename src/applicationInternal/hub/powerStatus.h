#pragma once

#include <string>
#include "remote_messages.pb.h"

// Tracks the power commands a scene sends as one session and drives the
// aggregate power banner from the hub's observed device state.
namespace Hub {
namespace PowerStatus {

// A command just left for the hub. `cached` is the remote's last snapshot of
// the device (or null); a device already in the target state is confirmed
// up front, since the hub publishes changes only and will never push it.
void commandSent(const std::string& deviceId, omote_OmoteCommand command,
                 const omote_DeviceState* cached);

// The power command has actually left for the hub. The session's deadline runs
// from here: a queued send can wait seconds for the link on wake, and the hub
// cannot start verifying until it arrives.
void commandTransmitted(omote_OmoteCommand command);

void observed(const omote_DeviceState& state);

// A hub error shown by the session banner instead of the generic error
// notification while a session is on screen. Returns true when consumed.
bool noteError(const std::string& message);

// True from the first power command until the banner has had its final say.
bool isActive();

}  // namespace PowerStatus
}  // namespace Hub
