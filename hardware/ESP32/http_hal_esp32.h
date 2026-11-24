#pragma once

#include <cstddef>
#include <cstdint>
#include <httpGetHandover.h>

// Runs one GET of url on its own task and reports through handover. The response must be 200 with
// exactly expectedLength bytes; anything else fails the GET.
bool start_http_get_HAL(const char* url, uint8_t* body, size_t expectedLength, HttpGetHandover* handover);
uint8_t* allocate_large_buffer_HAL(size_t bytes);
void free_large_buffer_HAL(uint8_t* buffer);
