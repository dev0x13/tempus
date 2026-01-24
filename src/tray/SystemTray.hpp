#pragma once

#include "services/TimeTrackingService.hpp"
#include <memory>
#include <functional>
#include <string>

namespace timetracker::tray {

// Stub implementation - system tray support to be implemented later
class SystemTray {
public:
    explicit SystemTray(std::shared_ptr<services::TimeTrackingService> timeService);
    ~SystemTray() = default;

    // Non-copyable, non-movable
    SystemTray(const SystemTray&) = delete;
    SystemTray& operator=(const SystemTray&) = delete;
    SystemTray(SystemTray&&) = delete;
    SystemTray& operator=(SystemTray&&) = delete;

    // Initialize the system tray (stub - always returns true)
    bool init();

    // Update the tray menu (stub - no-op)
    void update();

    // Set callbacks (stub - stores but doesn't use)
    void setShowWindowCallback(std::function<void()> callback);
    void setExitCallback(std::function<void()> callback);

    // Check if tray is running (stub - always returns false)
    bool isRunning() const;

private:
    std::shared_ptr<services::TimeTrackingService> timeService_;
    std::function<void()> showWindowCallback_;
    std::function<void()> exitCallback_;
};

} // namespace timetracker::tray
