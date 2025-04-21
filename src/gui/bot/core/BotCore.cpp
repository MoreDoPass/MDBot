#include "BotCore.hpp"

#include <QDebug>
#include <QThread>

#include "core/memory/MemoryManager.hpp" // Добавляем правильный include
#include "gui/log/LogManager.hpp"


BotCore::BotCore(DWORD processId, QObject* parent) : QObject(parent)
{
    m_context.processId = processId; // Сохраняем ID процесса

    try
    {
        m_memory = std::make_shared<MemoryManager>(processId);
        LogManager::instance().info(QString("MemoryManager initialized for process %1").arg(processId), "Core");
    }
    catch (const std::exception& e)
    {
        LogManager::instance().error(QString("Failed to initialize MemoryManager: %1").arg(e.what()), "Core", "Memory");
        throw; // Пробрасываем исключение дальше
    }
}

BotCore::~BotCore()
{
    LogManager::instance().debug("BotCore destructor called", "Core");
    disable();
}

bool BotCore::initialize()
{
    if (m_initialized)
    {
        LogManager::instance().debug("BotCore already initialized", "Core");
        return true;
    }

    // Проверяем что MemoryManager создан успешно
    if (!m_memory)
    {
        LogManager::instance().error("MemoryManager not initialized", "Core", "Memory");
        throw std::runtime_error("MemoryManager was not properly initialized");
    }

    // Находим handle окна
    if (!findWindowHandle())
    {
        QString error = QString("Could not find WoW window for process %1. Is it running?").arg(m_context.processId);
        LogManager::instance().error(error, "Core", "Window");
        throw std::runtime_error(error.toStdString());
    }

    // Устанавливаем хуки
    if (!setupHooks())
    {
        QString error =
            QString("Failed to set up hooks for process %1. Memory access denied?").arg(m_context.processId);
        LogManager::instance().error(error, "Core", "Hooks");
        throw std::runtime_error(error.toStdString());
    }

    m_initialized = true;
    LogManager::instance().info(QString("BotCore initialized for process: %1, window: 0x%2")
                                    .arg(m_context.processId)
                                    .arg(QString::number((quintptr)m_context.windowHandle, 16)),
                                "Core");
    return true;
}

void BotCore::enable()
{
    if (!m_enabled)
    {
        LogManager::instance().info("Bot enabled", "Core");
        m_enabled = true;
        emit stateChanged(true);
    }
}

void BotCore::disable()
{
    if (m_enabled)
    {
        LogManager::instance().info("Bot disabled", "Core");
        m_enabled = false;
        emit stateChanged(false);
    }
}

namespace
{
    struct EnumWindowsData
    {
        DWORD targetPid;
        HWND  result;
    };

    /**
     * @brief Проверяет, является ли окно окном WoW
     * @param className Имя класса окна
     * @param windowTitle Заголовок окна
     * @return true если это окно WoW
     *
     * Поддерживает различные версии клиента WoW:
     * - Класс GxWindowClassD3d для клиента 3.3.5a
     * - Класс Window для некоторых других версий
     */
    bool isWoWWindow(const char* className, const char* windowTitle)
    {
        // Проверяем заголовок окна
        if (strstr(windowTitle, "World of Warcraft") == nullptr)
        {
            return false;
        }

        // Проверяем класс окна
        // GxWindowClassD3d - стандартный класс для WoW 3.3.5a
        // Window - для некоторых других версий
        return (strcmp(className, "GxWindowClassD3d") == 0 || strcmp(className, "Window") == 0);
    }

    BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam)
    {
        auto* data      = reinterpret_cast<EnumWindowsData*>(lParam);
        DWORD windowPid = 0;
        GetWindowThreadProcessId(hwnd, &windowPid);

        if (windowPid == data->targetPid)
        {
            // Получаем имя класса окна
            char className[256];
            GetClassNameA(hwnd, className, sizeof(className));

            // Получаем заголовок окна
            char windowTitle[256];
            GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

            LogManager::instance().debug(
                QString("Found window - PID: %1, Class: %2, Title: %3").arg(windowPid).arg(className).arg(windowTitle),
                "Core");

            if (isWoWWindow(className, windowTitle))
            {
                data->result = hwnd;
                return FALSE; // Прекращаем поиск
            }
        }
        return TRUE; // Продолжаем поиск
    }
} // namespace

bool BotCore::findWindowHandle()
{
    EnumWindowsData data = {m_context.processId, nullptr};
    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&data));

    if (data.result)
    {
        m_context.windowHandle = data.result;

        // Проверяем, что окно все еще существует и видимо
        if (!IsWindow(data.result) || !IsWindowVisible(data.result))
        {
            LogManager::instance().error(QString("Window handle 0x%1 for process %2 is invalid or not visible")
                                             .arg(QString::number((quintptr)data.result, 16))
                                             .arg(m_context.processId),
                                         "Core",
                                         "Window");
            return false;
        }

        LogManager::instance().info(QString("Found WoW window handle: 0x%1 for process %2")
                                        .arg(QString::number((quintptr)data.result, 16))
                                        .arg(m_context.processId),
                                    "Core");
        return true;
    }

    LogManager::instance().error(QString("Failed to find WoW window for process %1. Make sure the game window is open.")
                                     .arg(m_context.processId),
                                 "Core",
                                 "Window");
    return false;
}

bool BotCore::setupHooks()
{
    // ПРИМЕЧАНИЕ: Функционал RegisterHook временно отключен для тестирования
    LogManager::instance().info("Hook setup skipped - RegisterHook functionality is temporarily disabled", "Core");
    return true;

    /* Оригинальный код закомментирован
    // Получаем базовый адрес run.exe
    uintptr_t runBase = m_memory->GetModuleBaseAddress(L"run.exe");
    if (!runBase)
    {
        LogManager::instance().error("Failed to get run.exe base address", "Core", "Hooks");
        return false;
    }

    LogManager::instance().debug(QString("Found run.exe base address: 0x%1").arg(QString::number(runBase, 16)), "Core");

    // Вычисляем адрес функции для хука
    uintptr_t targetAddress  = runBase + CharacterData::PLAYER_FUNC_OFFSET;
    void*     targetFunction = reinterpret_cast<void*>(targetAddress);

    LogManager::instance().debug(QString("Calculated hook target address: 0x%1 (base: 0x%2 + offset: 0x%3)")
                                     .arg(QString::number(targetAddress, 16))
                                     .arg(QString::number(runBase, 16))
                                     .arg(QString::number(CharacterData::PLAYER_FUNC_OFFSET, 16)),
                                 "Core");

    // Проверяем валидность адреса в целевом процессе
    if (!m_memory->IsValidAddress(targetAddress))
    {
        LogManager::instance().error(
            QString("Invalid hook target address: 0x%1").arg(QString::number(targetAddress, 16)), "Core", "Hooks");
        return false;
    }

    try
    {
        // Проверяем и сохраняем текущие права доступа
        DWORD currentProtection = m_memory->GetMemoryProtection(targetAddress);
        LogManager::instance().debug(QString("Current memory protection at 0x%1: 0x%2")
                                         .arg(QString::number(targetAddress, 16))
                                         .arg(QString::number(currentProtection, 16)),
                                     "Core");

        // Если нет прав на запись, пытаемся изменить
        if (!(currentProtection & PAGE_EXECUTE_READWRITE))
        {
            LogManager::instance().debug(
                QString("Attempting to change memory protection to PAGE_EXECUTE_READWRITE at 0x%1")
                    .arg(QString::number(targetAddress, 16)),
                "Core");

            if (!m_memory->SetMemoryProtection(targetAddress, 5, PAGE_EXECUTE_READWRITE))
            {
                DWORD error = GetLastError();
                LogManager::instance().error(
                    QString("Failed to change memory protection. Error: %1").arg(error), "Core", "Hooks");
                return false;
            }

            // Проверяем что права действительно изменились
            DWORD newProtection = m_memory->GetMemoryProtection(targetAddress);
            LogManager::instance().debug(QString("Memory protection after change at 0x%1: 0x%2")
                                             .arg(QString::number(targetAddress, 16))
                                             .arg(QString::number(newProtection, 16)),
                                         "Core");
        }

        // Создаем хук
        m_registerHook = std::make_unique<RegisterHook>(targetFunction,
                                                        m_memory, // Передаем shared_ptr на MemoryManager
                                                        Register::EAX,
                                                        [this](const Registers& regs) { onRegistersUpdated(regs); });

        bool success = m_registerHook->install();
        if (success)
        {
            LogManager::instance().info(
                QString("Successfully installed hook at address: 0x%1").arg(QString::number(targetAddress, 16)),
                "Core");
        }
        else
        {
            LogManager::instance().error(
                QString("Failed to install hook at address: 0x%1").arg(QString::number(targetAddress, 16)),
                "Core",
                "Hooks");
        }

        return success;
    }
    catch (const std::exception& e)
    {
        LogManager::instance().error(QString("Exception while setting up hooks: %1").arg(e.what()), "Core", "Hooks");
        return false;
    }
    */
}
//
// void BotCore::onRegistersUpdated(const Registers& regs)
// {
//     // Если структура найдена (EAX не 0)
//     if (regs.eax != 0)
//     {
//         try
//         {
//             // Проверяем что адрес валиден
//             if (!m_memory->IsValidAddress(regs.eax))
//             {
//                 LogManager::instance().warning(
//                     QString("Invalid EAX address: 0x%1").arg(QString::number(regs.eax, 16)), "Core", "Memory");
//                 return;
//             }
//
//             LogManager::instance().debug(
//                 QString("Updating character data from EAX: 0x%1").arg(QString::number(regs.eax, 16)), "Core");
//
//             // Читаем данные персонажа из памяти
//             m_context.character.currentHealth = m_memory->Read<uint32_t>(regs.eax +
//             CharacterData::CURRENT_HP_OFFSET); m_context.character.currentMana = m_memory->Read<uint32_t>(regs.eax +
//             CharacterData::CURRENT_MANA_OFFSET); m_context.character.maxHealth = m_memory->Read<uint32_t>(regs.eax +
//             CharacterData::MAX_HP_OFFSET); m_context.character.maxMana = m_memory->Read<uint32_t>(regs.eax +
//             CharacterData::MAX_MANA_OFFSET); m_context.character.level = m_memory->Read<uint32_t>(regs.eax +
//             CharacterData::LEVEL_OFFSET); m_context.character.eaxRegister = regs.eax;
//
//             LogManager::instance().debug(QString("Character data updated - Level: %1, HP: %2/%3, Mana: %4/%5")
//                                              .arg(m_context.character.level)
//                                              .arg(m_context.character.currentHealth)
//                                              .arg(m_context.character.maxHealth)
//                                              .arg(m_context.character.currentMana)
//                                              .arg(m_context.character.maxMana),
//                                          "Core");
//
//             emit contextUpdated();
//         }
//         catch (const std::exception& e)
//         {
//             LogManager::instance().error(QString("Error updating character data: %1").arg(e.what()), "Core",
//             "Memory");
//         }
//     }
// }