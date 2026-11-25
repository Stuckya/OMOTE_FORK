#pragma once

#include <cstdint>
#include <cstddef>

#if (ENABLE_WIFI_AND_MQTT == 1)
void start_wifi_and_websocket_in_background_HAL(const char* hub_url);
#endif
void init_websocket_HAL(const char* hub_url);
void websocket_loop_HAL();
bool publishWebSocketMessage_HAL(const uint8_t* data, size_t len);
bool send_websocket_message_after_connect_HAL(const uint8_t* data, size_t len);
bool websocket_message_was_sent_after_connect_HAL();
void websocket_shutdown_HAL();
bool websocket_is_connected_HAL();
const char* get_websocket_hub_url_HAL();

typedef void (*tAnnounceWebSocketMessage_cb)(const uint8_t* data, size_t len);

void set_announceWebSocketMessage_cb_HAL(tAnnounceWebSocketMessage_cb pAnnounceWebSocketMessage_cb);
