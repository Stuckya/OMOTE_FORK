#pragma once

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

// Owns the device list/scan/forget conversation with the hub (LIST_DEVICES,
// DEVICE_SCAN_*, DEVICE_FORGET) and caches the last known devices. A single
// DEVICE_LIST response kind answers both a stored-device listing and a scan,
// so the manager tracks which request is outstanding to route the reply.
class DeviceManager {
public:
    static DeviceManager& getInstance();

    void requestDeviceList();
    void startScan();
    void cancelScan();

    // Sends DEVICE_FORGET and removes the device from the cache optimistically;
    // the next LIST_DEVICES refresh (settings tab recreation) self-corrects.
    void forgetDevice(const std::string& deviceId);

    void handleDeviceList(const omote_DeviceList& list);

    const std::vector<DeviceEntry>& pairedDevices() const { return paired_; }
    const DeviceEntry* findDevice(const std::string& deviceId) const;

private:
    DeviceManager() = default;
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    enum class Pending { kNone, kList, kScan };

    void sendCommand(const std::string& device, omote_OmoteCommand command);

    Pending pending_ = Pending::kNone;
    std::vector<DeviceEntry> paired_;
    std::vector<DeviceEntry> scanResults_;
};

}  // namespace Hub
