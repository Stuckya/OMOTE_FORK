#pragma once

#include <mutex>
#include <string>
#include <vector>
#include "remote_messages.pb.h"

namespace Hub {

struct DeviceEntry {
    std::string deviceId;
    std::string name;
    std::string address;
    std::string model;
    omote_PairingRequirement pairing =
        omote_PairingRequirement_PAIRING_REQUIREMENT_UNSPECIFIED;
    bool paired = false;
    uint64_t pairedAt = 0;
};

// Owns the device list/scan/forget conversation with the hub and caches the
// last known devices. DEVICE_LIST has no request id, so requests are serialized.
class DeviceManager {
public:
    static DeviceManager& getInstance();

    void requestDeviceList();
    void startScan();
    void cancelScan();

    void forgetDevice(const std::string& deviceId);

    void handleDeviceList(const omote_DeviceList& list);

    void handleAck();

    // Called once per main loop, outside transport dispatch.
    void process();

    const std::vector<DeviceEntry>& pairedDevices() const { return paired_; }
    const DeviceEntry* findDevice(const std::string& deviceId) const;

private:
    DeviceManager() = default;
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    enum class Pending { kNone, kList, kScan, kCancelScan, kForget };

    struct DeviceListResponse {
        std::vector<DeviceEntry> entries;
        bool scanComplete = false;
    };

    struct InboundEvent {
        enum class Kind { kAck, kDeviceList };
        Kind kind = Kind::kAck;
        DeviceListResponse deviceList;
    };

    bool sendCommand(const std::string& device, omote_OmoteCommand command);
    bool sendListRequest();
    bool sendScanRequest();
    bool sendForgetRequest(const std::string& deviceId);
    void applyAck();
    void applyDeviceList(const DeviceListResponse& response);
    void flushDeferredRequests();
    void erasePairedDevice(const std::string& deviceId);
    void replacePairedDevices(const std::vector<DeviceEntry>& entries);

    Pending pending_ = Pending::kNone;
    bool refreshQueued_ = false;
    bool scanQueued_ = false;
    bool forgetQueued_ = false;
    bool ignoreNextListResponse_ = false;
    std::string queuedForgetDeviceId_;
    std::vector<DeviceEntry> paired_;
    std::vector<DeviceEntry> scanResults_;

    std::mutex inboundMutex_;
    std::vector<InboundEvent> inboundEvents_;
};

}  // namespace Hub
