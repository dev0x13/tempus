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
- libsecret (for storing API tokens in the system keyring)
- X11 or Wayland

Install dependencies on Ubuntu/Debian:

```bash
sudo apt install build-essential cmake libgtk-3-dev libayatana-appindicator3-dev libsecret-1-dev libgl1-mesa-dev
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

## Configuration

Both integrations are configured in **Settings**.

### YouTrack export

Fill in the YouTrack base URL and a permanent token. Activity names that already look like
`PROJECT-123` are exported as-is; anything else needs an entry in the **Aliases** table mapping the
activity name to an issue ID.

### KTalk import

1. **Space address** — the root of your KTalk space, e.g. `https://example.ktalk.ru`.
2. **Session token** — press **Copy command**, then on a browser tab where you are logged in to
   KTalk press <kbd>F12</kbd>, paste the command into the Console and press <kbd>Enter</kbd>. The
   token is placed in your clipboard; paste it into the token field.
3. Press **Test connection** to confirm both values work.

KTalk session tokens expire roughly every 30 days. When one does, the import fails with an
authentication error and offers a link back to Settings — repeat step 2.

Importing the same range twice is safe: a meeting already present is reported as skipped rather than
added again. A meeting counts as present when an entry with the same activity and the same title
overlaps it, so trimming or moving an imported entry afterwards — by hand, or by auto-fill resolving
an overlap — does not make the next import duplicate it.

### Where credentials are stored

API tokens are kept in the operating system's own facility rather than in the database: DPAPI on
Windows, Keychain on macOS, and the Secret Service (libsecret) on Linux, falling back to an
encrypted key file when no keyring is available. The `settings` table holds only a reference.

One consequence on Windows: the stored tokens are bound to your Windows user account, so copying
`data.db` to another machine or user carries the settings but not the tokens, which have to be
entered again.

## License

All rights reserved.
