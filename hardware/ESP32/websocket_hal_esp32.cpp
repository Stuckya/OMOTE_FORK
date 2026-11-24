#include <WiFi.h>
#include <WebSocketsClient.h>
#include <atomic>
#include <string>
#include <socketHandover.h>
#include <firstContact.h>
#include <esp_netif.h>
#include <esp_netif_net_stack.h>
#include <lwip/etharp.h>
#include <lwip/tcpip.h>
#include "websocket_hal_esp32.h"
#include "mqtt_hal_esp32.h"
#include "secrets.h"

WebSocketsClient webSocket;
tAnnounceWebSocketMessage_cb thisAnnounceWebSocketMessage_cb = NULL;
// Written by whichever task drives the socket, read by the main loop.
std::atomic<bool> isConnected{false};

namespace {
SocketHandover socketHandover;
std::string hubUrl;
// Read and written only by the task that holds the socket.
bool socketBegun = false;
bool firstAttemptMade = false;
FirstContact firstContact;
unsigned int addressAsks = 0;
}
const unsigned long WEBSOCKET_RECONNECT_INTERVAL_MS = 500;
const unsigned long HEARTBEAT_PING_INTERVAL_MS = 5000;
// TCP re-sends a lost packet 2 to 3 s later. A shorter wait drops the connection over one lost packet.
const unsigned long HEARTBEAT_PONG_TIMEOUT_MS = 4000;
const uint8_t HEARTBEAT_MISSED_PONGS_BEFORE_DISCONNECT = 2;
static_assert(HEARTBEAT_PONG_TIMEOUT_MS < HEARTBEAT_PING_INTERVAL_MS, "each ping restarts the wait for a pong");

void onWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%lu ms] WebSocket Disconnected\n", millis());
      isConnected = false;
      break;
      
    case WStype_CONNECTED:
      Serial.printf("[%lu ms] WebSocket Connected\n", millis());
      isConnected = true;
      break;
      
    case WStype_BIN:
      if (thisAnnounceWebSocketMessage_cb != NULL) {
        thisAnnounceWebSocketMessage_cb(payload, length);
      }
      break;
      
    case WStype_ERROR:
      Serial.printf("[%lu ms] WebSocket Error\n", millis());
      break;
      
    default:
      break;
  }
}

void set_announceWebSocketMessage_cb_HAL(tAnnounceWebSocketMessage_cb pAnnounceWebSocketMessage_cb) {
  thisAnnounceWebSocketMessage_cb = pAnnounceWebSocketMessage_cb;
}

struct HubEndpoint {
  String host;
  uint16_t port = 80;
  String path = "/";
};

static bool parseHubUrl(const char* hub_url, HubEndpoint& hub) {
  String urlStr(hub_url);
  int hostStart = urlStr.indexOf("://");
  if (hostStart < 0) {
    return false;
  }
  hostStart += 3;

  int portStart = urlStr.indexOf(':', hostStart);
  int pathStart = urlStr.indexOf('/', hostStart);

  if (portStart > 0) {
    hub.host = urlStr.substring(hostStart, portStart);
    if (pathStart > 0) {
      hub.port = urlStr.substring(portStart + 1, pathStart).toInt();
      hub.path = urlStr.substring(pathStart);
    } else {
      hub.port = urlStr.substring(portStart + 1).toInt();
    }
  } else if (pathStart > 0) {
    hub.host = urlStr.substring(hostStart, pathStart);
    hub.path = urlStr.substring(pathStart);
  } else {
    hub.host = urlStr.substring(hostStart);
  }
  return true;
}

static void beginSocket(const char* hub_url) {
  Serial.printf("Initializing WebSocket Client connection to %s\n", hub_url);

  HubEndpoint hub;
  if (!parseHubUrl(hub_url, hub)) {
    Serial.println("Error: Invalid WebSocket Server URL format");
    return;
  }

  Serial.printf("Connecting to host: %s, port: %d, path: %s\n", hub.host.c_str(), hub.port, hub.path.c_str());
  
  // Timestamped: a failed first attempt costs a full WEBSOCKET_RECONNECT_INTERVAL_MS before the retry.
  Serial.printf("[%lu ms] WebSocket first connect attempt\n", millis());
  webSocket.begin(hub.host, hub.port, hub.path);
  webSocket.onEvent(onWebSocketEvent);
  
  webSocket.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL_MS);
  
  webSocket.enableHeartbeat(HEARTBEAT_PING_INTERVAL_MS, HEARTBEAT_PONG_TIMEOUT_MS,
                            HEARTBEAT_MISSED_PONGS_BEFORE_DISCONNECT);
  
  Serial.printf("[%lu ms] WebSocket Client initialized (reconnect interval %lu ms)\n",
                millis(), WEBSOCKET_RECONNECT_INTERVAL_MS);
}

// The client measures its reconnect interval from a last-failure stamp that starts at 0, so it
// would hold the first attempt back until that much uptime has passed.
static void makeFirstConnectAttempt() {
  webSocket.setReconnectInterval(0);
  webSocket.loop();
  webSocket.setReconnectInterval(WEBSOCKET_RECONNECT_INTERVAL_MS);
}

// On the remote's own subnet the first packet goes to the hub itself, otherwise to the gateway.
static ip4_addr_t firstHopToHub() {
  HubEndpoint hub;
  IPAddress hubAddress;
  const uint32_t mask = WiFi.subnetMask();
  const bool hubIsOnThisSubnet = parseHubUrl(hubUrl.c_str(), hub) && hubAddress.fromString(hub.host) &&
                                 ((uint32_t)hubAddress & mask) == ((uint32_t)WiFi.localIP() & mask);
  ip4_addr_t firstHop;
  firstHop.addr = hubIsOnThisSubnet ? (uint32_t)hubAddress : (uint32_t)WiFi.gatewayIP();
  return firstHop;
}

// lwIP asks for an address once, then only on its one-second timer. If the access point drops that
// one request, the first connect stalls for as long. FirstContact asks again sooner.
static bool pathToHubIsOpen() {
  static const ip4_addr_t firstHop = firstHopToHub();
  esp_netif_t* wifi = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  struct netif* station = wifi == nullptr ? nullptr : (struct netif*)esp_netif_get_netif_impl(wifi);
  if (station == nullptr || ip4_addr_isany_val(firstHop)) {
    return true;
  }
  struct eth_addr* hardwareAddress = nullptr;
  const ip4_addr_t* entry = nullptr;
  LOCK_TCPIP_CORE();
  const bool addressKnown = etharp_find_addr(station, &firstHop, &hardwareAddress, &entry) >= 0;
  const FirstContact::Step step = firstContact.next(millis(), addressKnown);
  if (step == FirstContact::Step::AskForAddress) {
    etharp_query(station, &firstHop, NULL);
    addressAsks++;
  }
  UNLOCK_TCPIP_CORE();
  return step == FirstContact::Step::Connect;
}

static void driveSocket() {
  if (hubUrl.empty()) {
    return;
  }
  if (firstAttemptMade) {
    webSocket.loop();
    return;
  }
  // The first address request leaves before the setup, whose prints take about 12 ms.
  const bool pathIsOpen = pathToHubIsOpen();
  if (!socketBegun) {
    beginSocket(hubUrl.c_str());
    socketBegun = true;
  } else if (pathIsOpen) {
    makeFirstConnectAttempt();
    firstAttemptMade = true;
    Serial.printf("[%lu ms] Hub connect started after %u address requests\n", millis(), addressAsks);
  }
}

#if (ENABLE_WIFI_AND_MQTT == 1)
// The main loop and the display driver run on core 1.
const BaseType_t BACKGROUND_CONNECT_CORE = 0;
const UBaseType_t BACKGROUND_CONNECT_PRIORITY = 1;
// The connect was measured at about 3 kB of stack.
const uint32_t BACKGROUND_CONNECT_STACK_BYTES = 8192;

static void connectInBackground(void*) {
  init_mqtt_HAL();
  while (socketHandover.backgroundShouldKeepConnecting(isConnected)) {
    if (getIsWifiConnected_HAL()) {
      driveSocket();
    }
    vTaskDelay(1);
  }
  socketHandover.giveBack();
  vTaskDelete(NULL);
}

void start_wifi_and_websocket_in_background_HAL(const char* hub_url) {
  if (!socketHandover.lendToBackground()) {
    return;
  }
  hubUrl = hub_url;
  const BaseType_t created = xTaskCreatePinnedToCore(connectInBackground, "hubConnect", BACKGROUND_CONNECT_STACK_BYTES,
                                                     NULL, BACKGROUND_CONNECT_PRIORITY, NULL, BACKGROUND_CONNECT_CORE);
  if (created != pdPASS) {
    socketHandover.giveBack();
    init_mqtt_HAL();
  }
}
#endif

void init_websocket_HAL(const char* hub_url) {
  if (!socketHandover.mainLoopHoldsSocket()) {
    return;
  }
  hubUrl = hub_url;
}

void websocket_loop_HAL() {
  if (!socketHandover.mainLoopHoldsSocket()) {
    socketHandover.askForSocketBack();
    return;
  }
  driveSocket();
}

bool publishWebSocketMessage_HAL(const uint8_t* data, size_t len) {
  if (!websocket_is_connected_HAL()) {
    Serial.println("WebSocket Client not connected, cannot send message");
    return false;
  }
  
  if (len > 1024) {
    Serial.println("Error: WebSocket message exceeds reasonable size");
    return false;
  }
  
  bool result = webSocket.sendBIN(data, len);
  
  if (!result) {
    Serial.println("WebSocket Client failed to send message");
  }
  
  return result;
}

static void takeSocketBackFromBackground() {
  socketHandover.askForSocketBack();
  while (!socketHandover.mainLoopHoldsSocket()) {
    delay(1);
  }
}

void websocket_shutdown_HAL() {
  takeSocketBackFromBackground();
  Serial.println("Shutting down WebSocket Client");
  webSocket.disconnect();
  delay(50);  // Allow TCP stack to flush close frame before WiFi teardown
  isConnected = false;
  socketBegun = false;
}

bool websocket_is_connected_HAL() {
  return isConnected && socketHandover.mainLoopHoldsSocket();
}

const char* get_websocket_hub_url_HAL() {
  return WEBSOCKET_HUB_URL;
}

