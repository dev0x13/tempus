# Tempus

Tempus is a desktop application for tracking time spent on various activities. It provides an intuitive interface for managing time entries, viewing statistics, and integrating with external services.

Tempus is heavily inspired by [Hamster](https://github.com/projecthamster/hamster).

## Features

- Start/stop time tracking with a single click
- Manual time entry with custom start and end times
- All data is stored and processed locally, network connection is only needed for external export/import
- Activity breakdown and statistics
- CSV export
- YouTrack integration for exporting work items
- KTalk meeting import
- System tray integration with quick actions
- Multi-language support (English and Russian)

## Tech Stack

- **Language**: C++20
- **UI Framework**: Dear ImGui
- **Graphics**: OpenGL, GLFW, GLEW
- **Database**: SQLite (via SQLiteCpp)
- **HTTP Client**: cpr
- **Build System**: CMake
- **Package Manager**: Conan 2.x

## System Requirements

### All Platforms

- CMake 3.20 or higher
- Conan 2.x
- C++20 compatible compiler
- OpenGL 3.3+ capable graphics

### Linux

- GCC 11+ or Clang 14+
- GTK 3
- libayatana-appindicator3 (for system tray)
- X11 or Wayland

Install dependencies on Ubuntu/Debian:

```bash
sudo apt install build-essential cmake libgtk-3-dev libayatana-appindicator3-dev libgl1-mesa-dev
```

### Windows

- Visual Studio 2022 or MinGW with GCC 11+
- Windows 10 or higher

### macOS

- Xcode 14+ or Clang 14+
- macOS 11 (Big Sur) or higher
- GTK 3
- pkg-config

## Building

### 0. macOS-only: install GTK-3 and pkg-config

```bash
brew install gtk+3
brew install pkg-config
```

### 1. Install Conan dependencies

```bash
conan install . --build=missing -s build_type=Release
```

### 2. Configure with CMake

```bash
cmake --preset conan-release
```

### 3. Build

```bash
cmake --build --preset conan-release
```

### 4. Run

```bash
./build/tempus
```

On Windows:

```bash
build\Release\tempus.exe
```

## License

All rights reserved.
