#pragma once

#include <ctime>
#include <string>

void updateHardwareStatusAndShowOnGUI(void);
#if (ENABLE_HUB_COMMUNICATION > 0)
void updateTimeOnGUI(void);
void setTime(uint32_t timestamp_utc, int32_t seconds_west_of_utc);
// A UTC instant as the local 12-hour wall clock the status bar shows ("10:23
// PM"); empty when the device clock has never been set. The one place that
// knows the hub's UTC offset and how to render it.
std::string formatLocalClockTime(time_t utc_seconds);
#endif
