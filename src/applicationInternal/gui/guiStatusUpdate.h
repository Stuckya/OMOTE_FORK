#pragma once

void updateHardwareStatusAndShowOnGUI(void);
#if (ENABLE_HUB_COMMUNICATION > 0)
void updateTimeOnGUI(void);
void setTime(uint32_t timestamp_utc, int32_t seconds_west_of_utc);
// The offset the hub last sent; callers rendering a local wall-clock time need
// it because the device clock itself stays in UTC.
int32_t get_secondsWestOfUtc(void);
bool isDeviceClockSet(void);
#endif
