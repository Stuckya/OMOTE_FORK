#pragma once

#include <string>
#include <cstdint>
#include <cstddef>

// Function to get MAC address for Windows/Linux/macOS
std::string getMACaddress();

void init_espnow_HAL(void);
void espnow_loop_HAL();
bool publishEspNowMessage_HAL(const uint8_t* data, size_t len);
void espnow_shutdown_HAL();

typedef void (*tAnnounceEspNowMessage_cb)(const uint8_t* data, size_t len);

// The callback is invoked from espnow_loop_HAL(), never from the mock hub's own
// thread, so handlers may touch LVGL and other main-loop-only state. Inbound
// frames are buffered between the two.
void set_announceEspNowMessage_cb_HAL(tAnnounceEspNowMessage_cb pAnnounceEspNowMessage_cb);
