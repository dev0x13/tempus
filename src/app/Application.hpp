#pragma once

#include "database/Database.hpp"
#include "repositories/ActivityRepository.hpp"
#include "repositories/FactRepository.hpp"
#include "repositories/YouTrackExportLogRepository.hpp"
#include "services/TimeTrackingService.hpp"
#include "services/StatisticsService.hpp"
#include "services/ExportService.hpp"
#include "services/SettingsService.hpp"
#include "services/YouTrackExportService.hpp"
#include "services/KTalkImportService.hpp"
#include "ui/ImGuiApp.hpp"
#include "tray/SystemTray.hpp"
#include <memory>
#include <atomic>

namespace timetracker::app {

class Application {
public:
    Application();
    ~Application();

    // Non-copyable, non-movable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    // Initialize all components
    bool init();

    // Run the application
    void run();

    // Request shutdown
    void quit();

private:
    // Settings
    std::shared_ptr<services::SettingsService> settingsService_;

    // Database
    std::shared_ptr<database::Database> db_;

    // Repositories
    std::shared_ptr<repositories::ActivityRepository> activityRepo_;
    std::shared_ptr<repositories::FactRepository> factRepo_;
    std::shared_ptr<repositories::YouTrackExportLogRepository> exportLogRepo_;

    // Services
    std::shared_ptr<services::TimeTrackingService> timeService_;
    std::shared_ptr<services::StatisticsService> statsService_;
    std::shared_ptr<services::ExportService> exportService_;
    std::shared_ptr<services::YouTrackExportService> youTrackExportService_;
    std::shared_ptr<services::KTalkImportService> kTalkImportService_;

    // UI
    std::unique_ptr<ui::ImGuiApp> uiApp_;

    // System tray
    std::unique_ptr<tray::SystemTray> systemTray_;

    std::atomic<bool> running_{false};

    void initSettings();
    void initDatabase();
    void initRepositories();
    void initServices();
    void initUI();
    void initSystemTray();
};

} // namespace timetracker::app
