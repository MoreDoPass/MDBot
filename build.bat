@echo off
setlocal enabledelayedexpansion

echo [INFO] Starting build process...

:: Проверяем наличие CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake not found. Please install CMake and add it to PATH
    pause
    exit /b 1
)

:: Проверяем наличие директории build
if not exist build (
    echo [ERROR] Build directory not found. Please run clean_build.bat first
    pause
    exit /b 1
)

:: Переходим в директорию сборки
cd build

:: Собираем проект
echo [INFO] Building project...
cmake --build . --config Debug
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed
    cd ..
    pause
    exit /b 1
)

cd ..
echo [SUCCESS] Build completed successfully!
pause