#include "SystemTray.hpp"

namespace timetracker::tray {

SystemTray::SystemTray(std::shared_ptr<services::TimeTrackingService> timeService)
    : timeService_(std::move(timeService)) {}

bool SystemTray::init() {
    // Stub - system tray not implemented yet
    return true;
}

void SystemTray::update() {
    // Stub - no-op
}

void SystemTray::setShowWindowCallback(std::function<void()> callback) {
    showWindowCallback_ = std::move(callback);
}

void SystemTray::setExitCallback(std::function<void()> callback) {
    exitCallback_ = std::move(callback);
}

bool SystemTray::isRunning() const {
    // Stub - tray not actually running
    return false;
}

} // namespace timetracker::tray
