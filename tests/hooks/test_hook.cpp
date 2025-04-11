/**
 * @file test_hook.cpp
 * @brief Тесты для проверки системы хуков
 */
#include <gtest/gtest.h>
#include "core/hooks/RegisterHook.hpp"
#include "gui/LogManager.hpp"

// Тестовая функция, которую будем хукать
void __stdcall TestFunction() {
    // Просто пустая функция для тестов
}

/**
 * @brief Тестовый набор для проверки хуков
 */
TEST(HookTest, InstallationTest) {
    // Создаем тестовый хук
    RegisterHook hook(&TestFunction, Register::EAX, [](const Registers& regs) {
        LogManager::instance().debug(
            QString("Test hook called with EAX=0x%1")
                .arg(QString::number(regs.eax, 16)),
            "Test"
        );
    });

    // Проверяем установку хука
    EXPECT_TRUE(hook.install());
    EXPECT_TRUE(hook.isInstalled());

    // Проверяем удаление хука
    EXPECT_TRUE(hook.uninstall());
    EXPECT_FALSE(hook.isInstalled());
}