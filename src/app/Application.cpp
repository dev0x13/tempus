#include "Application.hpp"
#include "utils/Platform.hpp"
#include <iostream>

namespace timetracker::app {

Application::Application() = default;

Application::~Application() {
    quit();
}

bool Application::init() {
    try {
        initDatabase();
        initRepositories();
        initServices();
        initUI();
        initSystemTray();

        running_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize application: " << e.what() << std::endl;
        return false;
    }
}

void Application::run() {
    if (!running_) return;

    // Run the UI main loop
    uiApp_->run();
}

void Application::quit() {
    running_ = false;

    if (uiApp_) {
        uiApp_->requestClose();
    }
}

void Application::initDatabase() {
    // Ensure data directory exists
    utils::Platform::ensureDataDirectoryExists();

    // Get database path
    auto dbPath = utils::Platform::getDatabasePath();

    // Create database
    db_ = std::make_unique<database::Database>(dbPath.string());
}

void Application::initRepositories() {
    activityRepo_ = std::make_shared<repositories::ActivityRepository>(*db_);
    factRepo_ = std::make_shared<repositories::FactRepository>(*db_);
}

void Application::initServices() {
    timeService_ = std::make_shared<services::TimeTrackingService>(activityRepo_, factRepo_);
    statsService_ = std::make_shared<services::StatisticsService>(factRepo_);
    exportService_ = std::make_shared<services::ExportService>(factRepo_);
}

void Application::initUI() {
    uiApp_ = std::make_unique<ui::ImGuiApp>(timeService_, statsService_, exportService_);

    if (!uiApp_->init(900, 600, "Time Tracker")) {
        throw std::runtime_error("Failed to initialize UI");
    }

    // Close callback - system tray not implemented yet, so just close normally
    // TODO: When system tray is implemented, minimize to tray instead
}

void Application::initSystemTray() {
    systemTray_ = std::make_unique<tray::SystemTray>(timeService_);

    if (!systemTray_->init()) {
        std::cerr << "Warning: Failed to initialize system tray" << std::endl;
        return;
    }

    // Set callbacks
    systemTray_->setShowWindowCallback([this]() {
        uiApp_->show();
    });

    systemTray_->setExitCallback([this]() {
        quit();
    });
}

} // namespace timetracker::app
