@echo off
setlocal enabledelayedexpansion

echo [INFO] Starting clean build process...

:: Проверяем наличие CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake not found. Please install CMake and add it to PATH
    pause
    exit /b 1
)

:: Удаляем старую сборку
if exist build (
    echo [INFO] Removing old build directory...
    rmdir /s /q build
)

:: Создаем новую директорию для сборки
echo [INFO] Creating new build directory...
mkdir build
cd build

:: Конфигурируем проект
echo [INFO] Configuring project...
cmake -G "Visual Studio 17 2022" -A Win32 ..
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed
    cd ..
    pause
    exit /b 1
)

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