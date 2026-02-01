#include "ImGuiApp.hpp"
#include "Theme.hpp"
#include "views/TimeEntriesView.hpp"
#include "widgets/QuickAddDialog.hpp"
#include "widgets/SettingsWindow.hpp"
#include "widgets/ExportLogWindow.hpp"
#include "widgets/KTalkImportWindow.hpp"
#include "tray/SystemTray.hpp"
#include "utils/Platform.hpp"
#include "PTSansFont.hpp"
#include "localization/LocalizationManager.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <iostream>

#ifndef _WIN32
#include <gtk/gtk.h>
#endif

namespace timetracker::ui {

static void glfwErrorCallback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

ImGuiApp::ImGuiApp(
    std::shared_ptr<services::TimeTrackingService> timeService,
    std::shared_ptr<services::StatisticsService> statsService,
    std::shared_ptr<services::ExportService> exportService,
    std::shared_ptr<services::YouTrackExportService> youTrackExportService,
    std::shared_ptr<services::KTalkImportService> kTalkImportService,
    std::shared_ptr<services::SettingsService> settingsService,
    std::shared_ptr<repositories::YouTrackExportLogRepository> exportLogRepository)
    : timeService_(std::move(timeService))
    , statsService_(std::move(statsService))
    , exportService_(std::move(exportService))
    , youTrackExportService_(std::move(youTrackExportService))
    , kTalkImportService_(std::move(kTalkImportService))
    , settingsService_(std::move(settingsService))
    , exportLogRepository_(std::move(exportLogRepository)) {}

ImGuiApp::~ImGuiApp() {
    cleanup();
}

bool ImGuiApp::init(int width, int height, const char* title) {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        return false;
    }

#ifdef __APPLE__
    // macOS requires 3.2+ core profile for OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    // On Windows/Linux, request no specific version - let driver provide whatever it supports

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);  // Enable vsync

    // Initialize GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        glfwDestroyWindow(window_);
        glfwTerminate();
        return false;
    }

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    // Load embedded PT Sans font with cyrillic support
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;  // Horizontal oversampling for sharper rendering
    fontConfig.OversampleV = 2;  // Vertical oversampling
    fontConfig.PixelSnapH = true; // Align to pixel boundaries for crispness
    fontConfig.FontDataOwnedByAtlas = false; // Font data is embedded, don't let ImGui free it

    // Load font from embedded byte array
    io.Fonts->AddFontFromMemoryTTF(
        const_cast<unsigned char*>(fonts::PTSansRegular),
        fonts::PTSansRegularSize,
        20.0f,
        &fontConfig,
        io.Fonts->GetGlyphRangesCyrillic()
    );

    // Apply custom theme
    Theme::apply();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
#ifdef __APPLE__
    ImGui_ImplOpenGL3_Init("#version 150");
#else
    // Let ImGui auto-detect GLSL version based on available OpenGL
    ImGui_ImplOpenGL3_Init(nullptr);
#endif

    // Create views
    timeEntriesView_ = std::make_unique<TimeEntriesView>(timeService_, statsService_, exportService_, youTrackExportService_);

    // Set up callbacks for KTalk Import and Settings buttons
    timeEntriesView_->setKTalkImportCallback([this]() {
        kTalkImportWindow_->show();
    });
    timeEntriesView_->setSettingsCallback([this]() {
        settingsWindow_->show();
    });

    // Create quick add dialog
    quickAddDialog_ = std::make_unique<widgets::QuickAddDialog>(timeService_);
    quickAddDialog_->setOnDialogClosed([this]() {
        // Hide main window if it was shown specifically for quick add
        if (windowShownForQuickAdd_) {
            hide();
            windowShownForQuickAdd_ = false;
        }
    });

    // Create settings window
    settingsWindow_ = std::make_unique<widgets::SettingsWindow>(settingsService_);

    // Create export log window
    exportLogWindow_ = std::make_unique<widgets::ExportLogWindow>(exportLogRepository_);

    // Set up Export Log callback for settings window
    settingsWindow_->setExportLogCallback([this]() {
        exportLogWindow_->show();
    });

    // Create KTalk import window
    kTalkImportWindow_ = std::make_unique<widgets::KTalkImportWindow>(kTalkImportService_);

    // Set up close callback
    glfwSetWindowUserPointer(window_, this);
    glfwSetWindowCloseCallback(window_, [](GLFWwindow* window) {
        auto* app = static_cast<ImGuiApp*>(glfwGetWindowUserPointer(window));
        if (app->closeCallback_) {
            glfwSetWindowShouldClose(window, GLFW_FALSE);
            app->closeCallback_();
        }
    });

    return true;
}

void ImGuiApp::run() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

#ifndef _WIN32
        // Process GTK events for system tray on Linux
        while (gtk_events_pending()) {
            gtk_main_iteration();
        }
#endif

        // Skip rendering if window is minimized
        if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED)) {
            continue;
        }

        render();
    }
}

void ImGuiApp::render() {
    // Update system tray periodically (every second)
    if (systemTray_ && systemTray_->isRunning()) {
        double currentTime = glfwGetTime();
        if (currentTime - lastTrayUpdate_ >= 1.0) {
            systemTray_->update();
            lastTrayUpdate_ = currentTime;
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Get window size
    int width, height;
    glfwGetWindowSize(window_, &width, &height);

    // Create main window filling entire screen
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height)));

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoScrollbar;

    ImGui::Begin("Main", nullptr, windowFlags);

    // Main content - Time Entries view
    timeEntriesView_->render();

    ImGui::End();

    // Render quick add dialog
    quickAddDialog_->render();

    // Render settings window
    settingsWindow_->render();

    // Render export log window
    exportLogWindow_->render();

    // Render KTalk import window
    kTalkImportWindow_->render();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window_);
}

void ImGuiApp::requestClose() {
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void ImGuiApp::show() {
    glfwShowWindow(window_);
    glfwFocusWindow(window_);
}

void ImGuiApp::hide() {
    glfwHideWindow(window_);
}

bool ImGuiApp::isVisible() const {
    return glfwGetWindowAttrib(window_, GLFW_VISIBLE) != 0;
}

void ImGuiApp::minimizeToTray() {
    hide();
}

void ImGuiApp::setCloseCallback(std::function<void()> callback) {
    closeCallback_ = std::move(callback);
}

void ImGuiApp::showQuickAddDialog() {
    if (quickAddDialog_) {
        // Track if we need to show the window for quick add
        if (!isVisible()) {
            windowShownForQuickAdd_ = true;
            show();
        }
        quickAddDialog_->show();
    }
}

void ImGuiApp::setSystemTray(tray::SystemTray* tray) {
    systemTray_ = tray;
}

void ImGuiApp::cleanup() {
    if (window_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
    }
}

} // namespace timetracker::ui
