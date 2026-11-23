/*
  If you add additional overrides here, you have to
  1. first add #undef
  2. add new #define
*/
#undef WIFI_SSID
#undef WIFI_PASSWORD
#undef MQTT_SERVER
#undef ESPNOW_HUB_MAC
#undef WEBSOCKET_HUB_URL

#define WIFI_SSID                  "YourWifiSSID"              // override here
#define WIFI_PASSWORD              "YourWifiPassword"          // override here
#define MQTT_SERVER                "IPAddressOfYourBroker"     // override here

// Optional. Pins one access point when several broadcast the same SSID.
// If that access point does not answer, the remote joins the strongest one.
// Copy the BSSID from the remote's "WiFi connected" log line. The UniFi app can show a different BSSID for the same radio.
// #define WIFI_BSSID   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
// #define WIFI_CHANNEL 11

// Optional. A fixed address skips DHCP on every wake.
// Reserve the address on the router, or DHCP can hand it to another device.
// WIFI_STATIC_DNS is only needed when MQTT_SERVER or WEBSOCKET_HUB_URL names a host instead of an address.
// #define WIFI_STATIC_IP      "192.168.1.50"
// #define WIFI_STATIC_GATEWAY "192.168.1.1"
// #define WIFI_STATIC_SUBNET  "255.255.255.0"
// #define WIFI_STATIC_DNS     "192.168.1.1"

// MAC address of the hub
// For unicast communication, use the hub's actual MAC address
#define ESPNOW_HUB_MAC {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}

// WebSocket hub URL (replace with your hub's IP and port)
#define WEBSOCKET_HUB_URL "ws://192.168.1.50:8765"
