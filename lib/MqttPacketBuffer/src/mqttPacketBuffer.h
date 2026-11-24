#pragma once

#include <cstddef>

// PubSubClient holds a whole packet in this buffer, header and topic included.
static constexpr size_t MQTT_PACKET_BUFFER_BYTES = 1024;
