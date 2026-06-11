#include "deviceManager.h"
#include "hubManager.h"
#include "protoCodec.h"
#include "applicationInternal/omote_log.h"
#include "guis/gui_devices.h"
#include "guis/gui_settings.h"

#include <algorithm>

namespace Hub {

DeviceManager& DeviceManager::getInstance() {
    static DeviceManager instance;
    return instance;
}

bool DeviceManager::sendCommand(const std::string& device,
                                omote_OmoteCommand command) {
    omote_RemoteEvent event = ProtoCodec::createRemoteEvent(
        device, command, omote_OmoteCommandType_SHORT);
    return HubManager::getInstance().sendRemoteEvent(event);
}

void DeviceManager::requestDeviceList() {
    if (pending_ == Pending::kList) return;
    if (pending_ != Pending::kNone) {
        refreshQueued_ = true;
        omote_log_i("Queued device list refresh\r\n");
        return;
    }

    sendListRequest();
}

void DeviceManager::startScan() {
    scanResults_.clear();
    if (pending_ == Pending::kScan) return;
    if (pending_ != Pending::kNone) {
        scanQueued_ = true;
        omote_log_i("Queued device scan\r\n");
        return;
    }

    sendScanRequest();
}

void DeviceManager::cancelScan() {
    scanQueued_ = false;
    if (pending_ == Pending::kScan) {
        pending_ = Pending::kCancelScan;
        if (!sendCommand("HUB", omote_OmoteCommand_DEVICE_SCAN_CANCEL)) {
            pending_ = Pending::kNone;
            refreshQueued_ = true;
            return;
        }
        omote_log_i("Cancelled device scan\r\n");
        return;
    }
    if (pending_ == Pending::kList) {
        omote_log_i("Cancelled queued device scan\r\n");
        return;
    }
    if (pending_ == Pending::kCancelScan) return;

    omote_log_i("Cancelled device scan\r\n");
}

void DeviceManager::forgetDevice(const std::string& deviceId) {
    if (pending_ != Pending::kNone) {
        queuedForgetDeviceId_ = deviceId;
        forgetQueued_ = true;
        erasePairedDevice(deviceId);
        omote_log_i("Queued forget for %s\r\n", deviceId.c_str());
        return;
    }

    sendForgetRequest(deviceId);
}

bool DeviceManager::sendForgetRequest(const std::string& deviceId) {
    pending_ = Pending::kForget;
    if (!sendCommand(deviceId, omote_OmoteCommand_DEVICE_FORGET)) {
        pending_ = Pending::kNone;
        return false;
    }

    erasePairedDevice(deviceId);
    omote_log_i("Requested forget for %s\r\n", deviceId.c_str());
    return true;
}

void DeviceManager::handleAck() {
    InboundEvent event;
    event.kind = InboundEvent::Kind::kAck;
    std::lock_guard<std::mutex> lock(inboundMutex_);
    inboundEvents_.push_back(event);
}

void DeviceManager::process() {
    std::vector<InboundEvent> events;
    {
        std::lock_guard<std::mutex> lock(inboundMutex_);
        events.swap(inboundEvents_);
    }

    for (const InboundEvent& event : events) {
        if (event.kind == InboundEvent::Kind::kAck) {
            applyAck();
        } else {
            applyDeviceList(event.deviceList);
        }
    }

    flushDeferredRequests();
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
    DeviceListResponse response;
    response.entries.reserve(list.devices_count);
    response.scanComplete = list.scan_complete;
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
        response.entries.push_back(e);
    }

    InboundEvent event;
    event.kind = InboundEvent::Kind::kDeviceList;
    event.deviceList = response;
    std::lock_guard<std::mutex> lock(inboundMutex_);
    inboundEvents_.push_back(event);
}

bool DeviceManager::sendListRequest() {
    pending_ = Pending::kList;
    if (!sendCommand("HUB", omote_OmoteCommand_LIST_DEVICES)) {
        pending_ = Pending::kNone;
        return false;
    }
    omote_log_i("Requested device list\r\n");
    return true;
}

bool DeviceManager::sendScanRequest() {
    pending_ = Pending::kScan;
    scanResults_.clear();
    if (!sendCommand("HUB", omote_OmoteCommand_DEVICE_SCAN_START)) {
        pending_ = Pending::kNone;
        return false;
    }
    omote_log_i("Requested device scan\r\n");
    return true;
}

void DeviceManager::applyAck() {
    if (pending_ == Pending::kCancelScan) {
        pending_ = Pending::kNone;
        ignoreNextListResponse_ = true;
        refreshQueued_ = true;
        return;
    }

    if (pending_ == Pending::kForget) {
        pending_ = Pending::kNone;
        refreshQueued_ = true;
    }
}

void DeviceManager::applyDeviceList(const DeviceListResponse& response) {
    omote_log_i("Device list: %d devices, scan_complete=%d, pending=%d\r\n",
                static_cast<int>(response.entries.size()),
                static_cast<int>(response.scanComplete),
                static_cast<int>(pending_));

    if (pending_ == Pending::kScan) {
        scanResults_ = response.entries;
        if (response.scanComplete) pending_ = Pending::kNone;
        gui_devices_show_scan_results(scanResults_);
        return;
    }

    if (pending_ == Pending::kList) {
        if (ignoreNextListResponse_) {
            ignoreNextListResponse_ = false;
            pending_ = Pending::kNone;
            refreshQueued_ = true;
            omote_log_d("Ignored first device list after scan cancel\r\n");
            return;
        }

        replacePairedDevices(response.entries);
        pending_ = Pending::kNone;
        gui_settings_refresh_devices();
        return;
    }

    omote_log_d("Ignored device list with no matching request\r\n");
}

void DeviceManager::flushDeferredRequests() {
    if (pending_ != Pending::kNone) return;

    if (forgetQueued_) {
        const std::string deviceId = queuedForgetDeviceId_;
        queuedForgetDeviceId_.clear();
        forgetQueued_ = false;
        sendForgetRequest(deviceId);
        return;
    }

    if (scanQueued_) {
        scanQueued_ = false;
        sendScanRequest();
        return;
    }

    if (!refreshQueued_) return;
    refreshQueued_ = false;
    sendListRequest();
}

void DeviceManager::erasePairedDevice(const std::string& deviceId) {
    paired_.erase(
        std::remove_if(paired_.begin(), paired_.end(),
                       [&](const DeviceEntry& d) { return d.deviceId == deviceId; }),
        paired_.end());
}

void DeviceManager::replacePairedDevices(const std::vector<DeviceEntry>& entries) {
    paired_.clear();
    for (const DeviceEntry& e : entries) {
        if (forgetQueued_ && e.deviceId == queuedForgetDeviceId_) continue;
        if (e.paired) paired_.push_back(e);
    }
}

}  // namespace Hub
