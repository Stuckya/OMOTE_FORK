#pragma once

#include <lvgl.h>
#include <cstdint>
#include <string>

// Modal pairing overlay, rendered on lv_layer_top() above the tabview. Driven by
// PairingManager from the hub's PairingStatus steps; it is a focused/modal flow
// (no page indicator, no tab swipe), not a tab in the gui list.

void gui_pairing_show_awaiting(const std::string &deviceName,
                               const std::string &message,
                               uint32_t expectedPinLength, bool requiresPin);
void gui_pairing_show_verifying();
void gui_pairing_show_success(const std::string &deviceName);
void gui_pairing_show_failed(const std::string &message);
void gui_pairing_hide();
