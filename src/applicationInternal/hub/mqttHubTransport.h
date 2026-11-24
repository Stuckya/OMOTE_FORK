#pragma once

#include <mqttPacketBuffer.h>
#include "hubTransportBase.h"

#if (ENABLE_WIFI_AND_MQTT == 1)
class MqttHubTransport : public HubTransportBase {
public:
  MqttHubTransport();
  
  bool init() override;
  void process() override;
  bool sendRemoteEvent(const omote_RemoteEvent& event) override;
  bool isReady() override;
  // Leaves room for the PUBLISH fixed header and the "remote_responses" topic.
  static constexpr size_t PUBLISH_HEADER_AND_TOPIC_BYTES = 32;
  static constexpr size_t INBOUND_COMMAND_RESULT_BYTES = MQTT_PACKET_BUFFER_BYTES - PUBLISH_HEADER_AND_TOPIC_BYTES;

  size_t maxInboundCommandResultBytes() const override { return INBOUND_COMMAND_RESULT_BYTES; }
  void shutdown() override;

private:
  // MQTT-specific members
  bool mqttConnected;
  std::string baseTopic;
};
#endif 