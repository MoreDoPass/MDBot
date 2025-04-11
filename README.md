# MDBot - WoW 3.3.5a Bot

A bot for World of Warcraft 3.3.5a focused on memory manipulation and process automation.

## Technical Stack
- Modern C++20
- Qt 6.8.2 (Core, Widgets)
- CMake 3.16+
- Windows API

## Features

- Process memory manipulation
- Multi-window support
- Character name reading
- GUI interface with Qt6
- Structured logging with spdlog

## Requirements

- Windows OS
- Visual Studio 2022
- CMake 3.16+
- vcpkg package manager
- C++20 compatible compiler
- Qt 6 (via vcpkg x86-windows)
- spdlog (via vcpkg x86-windows)

## Dependencies

All dependencies must be x86 (32-bit) for WoW 3.3.5a compatibility:
- Qt 6.8.2 (Core, Widgets)
- spdlog 1.12.0 (Logging)

## Project Structure

```
src/
├── core/           # Core functionality
│   └── memory/    # Memory manipulation
└── gui/           # GUI components
    ├── MainWindow
    └── ProcessListDialog

.github/           # Project documentation
├── tasks/         # Task tracking
└── copilot-instructions.md
```

## Building

```bash
# Configure project (MUST use Win32)
cmake -B build -S . -A Win32

# Build project
cmake --build build --config Debug
```

## Memory Management

The project uses Windows API for process manipulation:
- Direct memory access through ReadProcessMemory/WriteProcessMemory
- Process module base address resolution
- Relative address handling (base + offset)
- Safe string operations

## Development Guidelines

- Target architecture: x86 (32-bit only)
- C++20 features
- Qt naming conventions
- Exception safety
- Memory safety practices

## Important Addresses

- Character Name: base + 0x879D18 (12 bytes max)