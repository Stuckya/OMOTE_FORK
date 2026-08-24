#include <esp_now.h>
#include <WiFi.h>
#include <string>
#include <sstream>
#include "espnow_hal_esp32.h"
#include <esp_wifi.h>
#include "espNowRxQueue.h"
#include "secrets.h"

// Function to get MAC address for ESP32
std::string getMACaddress() {
  return std::string(WiFi.macAddress().c_str());
}

// Define the MAC address of the Raspberry Pi hub
// This should be defined in secrets.h as ESPNOW_HUB_MAC
// Example: #define ESPNOW_HUB_MAC {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC}
uint8_t hub_mac[6] = ESPNOW_HUB_MAC;
esp_now_peer_info_t hub_peer;

// Callbacks for ESP-NOW received data
tAnnounceEspNowMessage_cb thisAnnounceEspNowMessage_cb = NULL;

static EspNowRxQueue rxQueue;
static uint32_t reportedDrops = 0;

// Invoked by the ESP-NOW driver on the WiFi task, and the frame buffer is only
// valid until it returns. Copy the bytes into the queue and dispatch them from
// espnow_loop_HAL() instead: this callback's consumers end up in LVGL, which
// corrupts its allocator when entered from anywhere but the main loop.
void onDataReceived(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  (void)mac_addr;
  if (data_len <= 0) {
    return;
  }
  rxQueue.push(data, (size_t)data_len);
}

void set_announceEspNowMessage_cb_HAL(tAnnounceEspNowMessage_cb pAnnounceEspNowMessage_cb) {
  thisAnnounceEspNowMessage_cb = pAnnounceEspNowMessage_cb;
}

void init_espnow_HAL(void) {
  Serial.println("Starting ESP-NOW");
  // Set WiFi mode to station (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(8, WIFI_SECOND_CHAN_NONE);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register callbacks
  esp_now_register_recv_cb(onDataReceived);
  
  // Register hub as peer
  memcpy(hub_peer.peer_addr, hub_mac, 6);
  // Setting channel0 defaults to existing channel setting
  hub_peer.channel = 0;  
  hub_peer.encrypt = false;
  
  // Add peer
  if (esp_now_add_peer(&hub_peer) != ESP_OK) {
    Serial.println("Failed to add hub peer");
    return;
  }
  
  Serial.println("ESP-NOW initialized successfully");
}

void espnow_loop_HAL() {
  const uint32_t drops = rxQueue.droppedFrames();
  if (drops != reportedDrops) {
    Serial.printf("ESP-NOW: dropped %u inbound frame(s)\r\n", (unsigned)(drops - reportedDrops));
    reportedDrops = drops;
  }

  if (thisAnnounceEspNowMessage_cb == NULL) {
    return;
  }

  // Bounded per call so a saturated link cannot hold the main loop.
  EspNowRxQueue::Frame frame;
  for (size_t drained = 0; drained < EspNowRxQueue::CAPACITY; drained++) {
    if (!rxQueue.pop(frame)) {
      return;
    }
    thisAnnounceEspNowMessage_cb(frame.bytes, frame.length);
  }
}

bool publishEspNowMessage_HAL(const uint8_t* data, size_t len) {
  if (len > 250) {
    Serial.println("Error: ESP-NOW message exceeds maximum size");
    return false;
  }
  
  // Send the binary message directly.
  esp_err_t result = esp_now_send(hub_peer.peer_addr, data, len);
  
  if (result == ESP_OK) {
    return true;
  }
  
  Serial.println("ESP-NOW failed to send message");
  return false;
}

void espnow_shutdown_HAL() {
  // Unregister peer
  esp_now_del_peer(hub_peer.peer_addr);

  // Deinitialize ESP-NOW
  esp_now_deinit();

  Serial.println("ESP-NOW shutdown complete");
}
