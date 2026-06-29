#pragma once

#include <string>
#include <vector>

#include "applicationInternal/hub/deviceManager.h"

// Modal device-management overlay on lv_layer_top(), sibling of the pairing
// modal: scan (spinner / results / empty) and device detail with a confirmed
// destructive Forget. Driven by DeviceManager; entry point is the Settings
// Devices group.

void gui_devices_show_scanning();
void gui_devices_show_scan_results(const std::vector<Hub::DeviceEntry> &devices);
void gui_devices_show_detail(const Hub::DeviceEntry &device);
void gui_devices_hide();

// Tears down the overlay WITHOUT resuming the tab tree — used when the pairing
// modal takes over the screen from a scan-result row.
void gui_devices_dismiss_overlay();
