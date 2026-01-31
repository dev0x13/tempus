#pragma once

#include "services/TimeTrackingService.hpp"
#include "services/StatisticsService.hpp"
#include "services/ExportService.hpp"
#include "services/YouTrackExportService.hpp"
#include "services/SettingsService.hpp"
#include <memory>
#include <functional>

struct GLFWwindow;

namespace timetracker::tray {
class SystemTray;
}

namespace timetracker::ui {

// Forward declarations
class OverviewView;
class TimeEntriesView;

namespace widgets {
class QuickAddDialog;
class SettingsWindow;
}

class ImGuiApp {
public:
    ImGuiApp(
        std::shared_ptr<services::TimeTrackingService> timeService,
        std::shared_ptr<services::StatisticsService> statsService,
        std::shared_ptr<services::ExportService> exportService,
        std::shared_ptr<services::YouTrackExportService> youTrackExportService,
        std::shared_ptr<services::SettingsService> settingsService);
    ~ImGuiApp();

    // Non-copyable, non-movable
    ImGuiApp(const ImGuiApp&) = delete;
    ImGuiApp& operator=(const ImGuiApp&) = delete;
    ImGuiApp(ImGuiApp&&) = delete;
    ImGuiApp& operator=(ImGuiApp&&) = delete;

    // Initialize window and ImGui
    bool init(int width = 900, int height = 600, const char* title = "Time Tracker");

    // Main loop - returns when window closes
    void run();

    // Request window to close
    void requestClose();

    // Show/hide window
    void show();
    void hide();
    bool isVisible() const;

    // Minimize to tray
    void minimizeToTray();

    // Set callback for window close (can be used to minimize to tray instead)
    void setCloseCallback(std::function<void()> callback);

    // Get GLFW window handle
    GLFWwindow* getWindow() const { return window_; }

    // Show quick add dialog
    void showQuickAddDialog();

    // Set system tray for periodic updates
    void setSystemTray(tray::SystemTray* tray);

private:
    GLFWwindow* window_{nullptr};
    std::function<void()> closeCallback_;

    // Services
    std::shared_ptr<services::TimeTrackingService> timeService_;
    std::shared_ptr<services::StatisticsService> statsService_;
    std::shared_ptr<services::ExportService> exportService_;
    std::shared_ptr<services::YouTrackExportService> youTrackExportService_;
    std::shared_ptr<services::SettingsService> settingsService_;

    // Views
    std::unique_ptr<OverviewView> overviewView_;
    std::unique_ptr<TimeEntriesView> timeEntriesView_;

    // Quick add dialog
    std::unique_ptr<widgets::QuickAddDialog> quickAddDialog_;

    // Settings window
    std::unique_ptr<widgets::SettingsWindow> settingsWindow_;

    // System tray (not owned)
    tray::SystemTray* systemTray_{nullptr};

    // Tray update timing
    double lastTrayUpdate_{0.0};

    int currentTab_{0};

    // Track if window was shown specifically for quick add dialog
    bool windowShownForQuickAdd_{false};

    void render();
    void cleanup();
};

} // namespace timetracker::ui
