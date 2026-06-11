#include "deviceManager.h"
#include "hubManager.h"
#include "protoCodec.h"
#include "applicationInternal/omote_log.h"
#include "applicationInternal/hardware/hardwarePresenter.h"
#include "guis/gui_devices.h"
#include "guis/gui_settings.h"

#include <algorithm>

namespace Hub {

DeviceManager& DeviceManager::getInstance() {
    static DeviceManager instance;
    return instance;
}

void DeviceManager::sendCommand(const std::string& device,
                                omote_OmoteCommand command) {
    omote_RemoteEvent event = ProtoCodec::createRemoteEvent(
        device, command, omote_OmoteCommandType_SHORT);
    HubManager::getInstance().sendRemoteEvent(event);
}

void DeviceManager::requestDeviceList() {
    pending_ = Pending::kList;
    sendCommand("HUB", omote_OmoteCommand_LIST_DEVICES);
    omote_log_i("Requested device list\r\n");
}

void DeviceManager::startScan() {
    pending_ = Pending::kScan;
    scanResults_.clear();
    sendCommand("HUB", omote_OmoteCommand_DEVICE_SCAN_START);
    omote_log_i("Requested device scan\r\n");
}

void DeviceManager::cancelScan() {
    pending_ = Pending::kNone;
    sendCommand("HUB", omote_OmoteCommand_DEVICE_SCAN_CANCEL);
    omote_log_i("Cancelled device scan\r\n");
}

void DeviceManager::forgetDevice(const std::string& deviceId) {
    sendCommand(deviceId, omote_OmoteCommand_DEVICE_FORGET);
    paired_.erase(
        std::remove_if(paired_.begin(), paired_.end(),
                       [&](const DeviceEntry& d) { return d.deviceId == deviceId; }),
        paired_.end());
    omote_log_i("Requested forget for %s\r\n", deviceId.c_str());
}

const DeviceEntry* DeviceManager::findDevice(const std::string& deviceId) const {
    for (const DeviceEntry& d : paired_) {
        if (d.deviceId == deviceId) return &d;
    }
    for (const DeviceEntry& d : scanResults_) {
        if (d.deviceId == deviceId) return &d;
    }
    return nullptr;
}

void DeviceManager::handleDeviceList(const omote_DeviceList& list) {
    std::vector<DeviceEntry> entries;
    entries.reserve(list.devices_count);
    for (pb_size_t i = 0; i < list.devices_count; ++i) {
        const omote_DeviceInfo& info = list.devices[i];
        DeviceEntry e;
        e.deviceId = info.device_id;
        e.name = info.name;
        e.address = info.address;
        e.model = info.model;
        e.pairing = info.pairing;
        e.paired = info.paired;
        e.pairedAt = info.paired_at;
        entries.push_back(e);
    }

    omote_log_i("Device list: %d devices, scan_complete=%d, pending=%d\r\n",
                static_cast<int>(entries.size()),
                static_cast<int>(list.scan_complete),
                static_cast<int>(pending_));

    if (pending_ == Pending::kScan) {
        scanResults_ = entries;
        // Progress updates refresh the results in place; the final list also
        // clears the outstanding request.
        if (list.scan_complete) pending_ = Pending::kNone;
        gui_devices_show_scan_results(scanResults_);
        return;
    }

    // Stored-device listing (or an unsolicited push): refresh the paired cache
    // and let the Settings tab redraw its Devices rows if it is on screen.
    paired_.clear();
    for (const DeviceEntry& e : entries) {
        if (e.paired) paired_.push_back(e);
    }
    if (pending_ == Pending::kList) pending_ = Pending::kNone;
    gui_settings_refresh_devices();
}

}  // namespace Hub
