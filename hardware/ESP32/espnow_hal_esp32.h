#pragma once

#include <string>
#include <cstdint>
#include <cstddef>

// Function to get MAC address for ESP32
std::string getMACaddress();

void init_espnow_HAL(void);
void espnow_loop_HAL();
bool publishEspNowMessage_HAL(const uint8_t* data, size_t len);
void espnow_shutdown_HAL();

// Accepts a frame from the radio. Safe to call from the receiving thread: the
// frame is buffered and dispatched later from espnow_loop_HAL().
void receiveEspNowFrame_HAL(const uint8_t* data, size_t len);

typedef void (*tAnnounceEspNowMessage_cb)(const uint8_t* data, size_t len);

// The callback is invoked from espnow_loop_HAL(), never from the radio thread
// that receives the frame, so handlers may touch LVGL and other main-loop-only
// state. Inbound frames are buffered between the two.
void set_announceEspNowMessage_cb_HAL(tAnnounceEspNowMessage_cb pAnnounceEspNowMessage_cb);
