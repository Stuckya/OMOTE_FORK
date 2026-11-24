#include "http_hal_esp32.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <esp_heap_caps.h>
#include <string>

namespace {

const uint32_t HTTP_GET_STACK_BYTES = 8192;
const UBaseType_t HTTP_GET_PRIORITY = 1;
const uint32_t CONNECT_TIMEOUT_MS = 2000;
const uint32_t READ_TIMEOUT_MS = 3000;

struct HttpGet {
  std::string url;
  uint8_t* body = nullptr;
  size_t expectedLength = 0;
  HttpGetHandover* handover = nullptr;
};

// The handover allows one GET at a time, so the main loop never rewrites this while a task reads it.
HttpGet pendingGet;

bool readBody(Stream* stream, uint8_t* body, size_t length) {
  size_t received = 0;
  const unsigned long startedAt = millis();
  while (received < length && millis() - startedAt < READ_TIMEOUT_MS) {
    received += stream->readBytes(body + received, length - received);
  }
  return received == length;
}

bool runGet(const HttpGet& get) {
  HTTPClient http;
  http.setConnectTimeout(CONNECT_TIMEOUT_MS);
  http.setTimeout(READ_TIMEOUT_MS);
  if (!http.begin(get.url.c_str())) {
    return false;
  }
  const bool ok = http.GET() == HTTP_CODE_OK && http.getSize() == (int)get.expectedLength &&
                  readBody(http.getStreamPtr(), get.body, get.expectedLength);
  http.end();
  return ok;
}

void httpGetTask(void*) {
  pendingGet.handover->finish(runGet(pendingGet));
  vTaskDelete(NULL);
}

}  // namespace

bool start_http_get_HAL(const char* url, uint8_t* body, size_t expectedLength, HttpGetHandover* handover) {
  if (!handover->begin()) {
    return false;
  }
  pendingGet.url = url;
  pendingGet.body = body;
  pendingGet.expectedLength = expectedLength;
  pendingGet.handover = handover;
  if (xTaskCreate(httpGetTask, "httpGet", HTTP_GET_STACK_BYTES, NULL, HTTP_GET_PRIORITY, NULL) != pdPASS) {
    handover->finish(false);
  }
  return true;
}

uint8_t* allocate_large_buffer_HAL(size_t bytes) {
  return (uint8_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void free_large_buffer_HAL(uint8_t* buffer) {
  heap_caps_free(buffer);
}
