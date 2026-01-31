cmake_minimum_required(VERSION 3.20)
project(time-tracker VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find packages from Conan
find_package(imgui REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(SQLiteCpp REQUIRED)
find_package(OpenGL REQUIRED)
find_package(portable-file-dialogs REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(cpr REQUIRED)
find_package(ICU REQUIRED COMPONENTS uc)

# ImGui backends
set(IMGUI_BACKENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/imgui_impl_opengl3.cpp
)

# Source files
set(SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/database/Database.cpp
    src/repositories/ActivityRepository.cpp
    src/repositories/FactRepository.cpp
    src/repositories/YouTrackExportLogRepository.cpp
    src/services/TimeTrackingService.cpp
    src/services/StatisticsService.cpp
    src/services/ExportService.cpp
    src/services/SettingsService.cpp
    src/services/YouTrackExportService.cpp
    src/ui/ImGuiApp.cpp
    src/ui/views/OverviewView.cpp
    src/ui/views/TimeEntriesView.cpp
    src/ui/widgets/DatePicker.cpp
    src/ui/widgets/ActivityAutocomplete.cpp
    src/ui/widgets/TimeInput.cpp
    src/ui/widgets/QuickAddDialog.cpp
    src/ui/widgets/SettingsWindow.cpp
    src/ui/widgets/ExportLogWindow.cpp
    src/tray/SystemTray.cpp
    src/utils/TimeUtils.cpp
    ${IMGUI_BACKENDS}
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui::imgui
    glfw
    GLEW::GLEW
    SQLiteCpp
    OpenGL::GL
    portable-file-dialogs::portable-file-dialogs
    nlohmann_json::nlohmann_json
    cpr::cpr
    ICU::uc
)

# Platform-specific settings
if(WIN32)
    # Hide console window in Release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux-specific dependencies for system tray
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(AYATANA REQUIRED ayatana-appindicator3-0.1)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${AYATANA_INCLUDE_DIRS}
        ${GTK3_INCLUDE_DIRS})

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${AYATANA_LIBRARIES}
        ${GTK3_LIBRARIES})
endif()

# Copy assets to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
