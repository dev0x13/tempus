#include "ImGuiApp.hpp"
#include "Theme.hpp"
#include "views/OverviewView.hpp"
#include "views/TimeEntriesView.hpp"
#include "widgets/QuickAddDialog.hpp"
#include "tray/SystemTray.hpp"
#include "utils/Platform.hpp"

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
    std::shared_ptr<services::SettingsService> settingsService)
    : timeService_(std::move(timeService))
    , statsService_(std::move(statsService))
    , exportService_(std::move(exportService))
    , youTrackExportService_(std::move(youTrackExportService))
    , settingsService_(std::move(settingsService)) {}

ImGuiApp::~ImGuiApp() {
    cleanup();
}

bool ImGuiApp::init(int width, int height, const char* title) {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        return false;
    }

    // GL 3.3 + GLSL 130
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

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

    // Apply custom theme
    Theme::apply();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Create views
    overviewView_ = std::make_unique<OverviewView>(timeService_);
    timeEntriesView_ = std::make_unique<TimeEntriesView>(timeService_, statsService_, exportService_, youTrackExportService_);

    // Create quick add dialog
    quickAddDialog_ = std::make_unique<widgets::QuickAddDialog>(timeService_);
    quickAddDialog_->setOnTrackingStarted([this]() {
        // Refresh overview when tracking starts
        if (overviewView_) {
            // Views will refresh on next render
        }
    });

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
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("Main", nullptr, windowFlags);

    // Settings button in top right corner
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 100.0f);
    ImGui::SetCursorPosY(10.0f);
    if (ImGui::Button("Settings", ImVec2(90.0f, 0.0f))) {
        // Open settings.json in default editor
        auto settingsPath = settingsService_->getSettingsPath();
        std::string errorMessage;
        if (!utils::Platform::openFileInEditor(settingsPath, errorMessage)) {
            std::cerr << "Failed to open settings file: " << errorMessage << std::endl;
            // Could show an ImGui popup here in the future
        }
    }

    // Tab bar
    if (ImGui::BeginTabBar("MainTabs")) {
        if (ImGui::BeginTabItem("Overview")) {
            currentTab_ = 0;
            overviewView_->render();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Time Entries")) {
            currentTab_ = 1;
            timeEntriesView_->render();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();

    // Render quick add dialog
    quickAddDialog_->render();

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
        quickAddDialog_->show();
        // Also show the window if it's hidden
        if (!isVisible()) {
            show();
        }
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
