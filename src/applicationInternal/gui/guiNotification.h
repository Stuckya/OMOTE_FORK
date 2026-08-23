#pragma once

#include <lvgl.h>
#include <string>
#include <hubPowerSession.h>

// Global notification system that slides down from under the status bar
namespace GuiNotification {

enum class NotificationType {
    VOLUME,
    POWER,
    POWER_SESSION,
    MESSAGE,
    ERROR
};

void init();

void showVolumeNotification(double level, bool is_muted);

void showPowerNotification(bool is_on);

// Renders the session in place: headline plus one segment per device. Holds
// while in progress, auto-hides after the session resolves or times out.
void showPowerSession(const HubPowerSession& session);

void showMessageNotification(const std::string& message);

void showErrorNotification(const std::string& message);

void hideNotification();

bool isNotificationVisible();

} // namespace GuiNotification
