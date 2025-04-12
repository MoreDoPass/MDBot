# Note: Use 'cmake -A Win32 ..' when configuring this project
cmake_minimum_required(VERSION 3.16)
project(MDBot VERSION 1.0.0 LANGUAGES CXX)

# Настройка типа сборки
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Добавляем флаги отладки для MSVC
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od /DEBUG")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")

# Force x86 platform if supported by generator
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_GENERATOR_PLATFORM "Win32" CACHE STRING "Visual Studio platform" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg integration
set(VCPKG_ROOT "C:/Dev/vcpkg" CACHE PATH "Path to vcpkg installation")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x86-windows" CACHE STRING "Vcpkg target triplet")

# Force CMake to find Qt from vcpkg only
set(CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/x86-windows" CACHE PATH "Force vcpkg Qt path")

# Qt configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Source files
set(CORE_SOURCES
    src/core/memory/MemoryManager.cpp
    src/core/hooks/Hook.cpp
    src/core/hooks/RegisterHook.cpp
)

set(CORE_HEADERS
    src/core/memory/MemoryManager.hpp
    src/core/hooks/Hook.hpp
    src/core/hooks/RegisterHook.hpp
)

set(DEBUG_SOURCES
    src/debug/DebugCore.cpp
)

set(DEBUG_HEADERS
    src/debug/DebugCore.hpp
)

set(GUI_SOURCES
    src/gui/MainWindow.cpp
    src/gui/log/LogWindow.cpp
    src/gui/log/LogManager.cpp
    src/gui/debug/DebugWindow.cpp
    src/gui/bot/process/ProcessListDialog.cpp
    src/gui/bot/core/BotCore.cpp
    src/gui/bot/ui/BotTabWidget.cpp
    src/gui/bot/ui/modules/character/CharacterWidget.cpp
)

set(GUI_HEADERS
    src/gui/MainWindow.hpp
    src/gui/log/LogWindow.hpp
    src/gui/log/LogManager.hpp
    src/gui/debug/DebugWindow.hpp
    src/gui/bot/process/ProcessListDialog.hpp
    src/gui/bot/core/BotCore.hpp
    src/gui/bot/ui/BotTabWidget.hpp
    src/gui/bot/ui/modules/character/CharacterWidget.hpp
)

set(SOURCES
    src/main.cpp
    ${GUI_SOURCES}
    ${CORE_SOURCES}
    ${DEBUG_SOURCES}
)

add_library(mdbot_core
    ${CORE_SOURCES}
    ${DEBUG_SOURCES}
    src/gui/log/LogManager.cpp
)

target_include_directories(mdbot_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(mdbot_core PUBLIC Qt6::Core Qt6::Widgets)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${GUI_HEADERS}
    ${CORE_HEADERS}
    ${DEBUG_HEADERS}
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    src
    include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    mdbot_core
)

if(WIN32)
    # Add Windows-specific compile definitions
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
    )

    # Set paths for DLL copying
    set(VCPKG_DEBUG_BIN "C:/Dev/vcpkg/installed/x86-windows/debug/bin")
    set(QT_PLUGINS_DIR "C:/Dev/vcpkg/installed/x86-windows/debug/Qt6/plugins")
    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        # Create required directories
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/logs"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/logs/system"
        
        # Copy Core Qt DLLs
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VCPKG_DEBUG_BIN}/Qt6Cored.dll"
            "${VCPKG_DEBUG_BIN}/Qt6Widgetsd.dll"
            "${VCPKG_DEBUG_BIN}/Qt6Guid.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            
        # Copy Qt platform plugin
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_PLUGINS_DIR}/platforms/qwindowsd.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms"
            
        # Copy Qt dependencies
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VCPKG_DEBUG_BIN}/zlibd1.dll"
            "${VCPKG_DEBUG_BIN}/icuucd74.dll"
            "${VCPKG_DEBUG_BIN}/icuind74.dll"
            "${VCPKG_DEBUG_BIN}/icudtd74.dll"
            "${VCPKG_DEBUG_BIN}/double-conversion.dll"
            "${VCPKG_DEBUG_BIN}/pcre2-16d.dll"
            "${VCPKG_DEBUG_BIN}/zstd.dll"
            "${VCPKG_DEBUG_BIN}/libpng16d.dll"
            "${VCPKG_DEBUG_BIN}/harfbuzz.dll"
            "${VCPKG_DEBUG_BIN}/freetyped.dll"
            "${VCPKG_DEBUG_BIN}/bz2d.dll"
            "${VCPKG_DEBUG_BIN}/brotlidec.dll"
            "${VCPKG_DEBUG_BIN}/brotlicommon.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            
        COMMENT "Copying Qt DLLs and dependencies..."
    )

    # Set the working directory for Visual Studio debugging
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endif()