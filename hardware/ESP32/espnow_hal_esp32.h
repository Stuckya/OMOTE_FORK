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

// Copies inbound bytes for main-loop dispatch.
void receiveEspNowFrame_HAL(const uint8_t* data, size_t len);

typedef void (*tAnnounceEspNowMessage_cb)(const uint8_t* data, size_t len);

// Registered callbacks run only from espnow_loop_HAL().
void set_announceEspNowMessage_cb_HAL(tAnnounceEspNowMessage_cb pAnnounceEspNowMessage_cb);
