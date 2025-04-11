#include "RegisterHook.hpp"
#include "gui/LogManager.hpp"
#include "core/memory/MemoryManager.hpp"
#include <mutex>
#include <QDebug>

// Защита статического указателя мьютексом
static std::mutex g_hookMutex;
static void* g_hookFunction = nullptr;

// Инициализация статических членов
RegisterHook* RegisterHook::s_instance = nullptr;
std::shared_mutex RegisterHook::s_instanceMutex;

// Функция для получения instance в ассемблерном коде
extern "C" RegisterHook* __stdcall GetHookInstance() {
    return RegisterHook::getInstance();
}

/**
 * @brief Ассемблерная функция перехвата
 * @details Сохраняет состояние регистров, создает структуру Registers,
 * вызывает callback и восстанавливает выполнение оригинальной функции
 */
static __declspec(naked) void HookFunction()
{
    __asm {
        // Сохраняем все регистры и флаги
        pushfd
        pushad
        
        // Проверяем, что instance существует
        push ebx                // Сохраняем ebx
        call GetHookInstance   // Получаем instance через функцию
        mov ebx, eax          // Сохраняем результат в ebx
        test ebx, ebx
        jz cleanup            // Если нет, пропускаем callback
        
        // Выделяем место под структуру Registers (8 * 4 = 32 байта)
        // Выравниваем стек по 16 байт для совместимости с SSE
        sub esp, 32
        and esp, 0FFFFFFF0h
        
        // Заполняем структуру значениями регистров
        mov ebx, esp        // ebx = указатель на структуру
        
        // Сохраняем регистры в структуру (с учетом сдвига стека от pushad/pushfd)
        mov eax, [esp + 36] // Original EAX (offset due to pushad + pushfd)
        mov [ebx], eax      // Registers.eax
        mov eax, [esp + 40] // Original ECX
        mov [ebx + 4], eax  // Registers.ecx
        mov eax, [esp + 44] // Original EDX
        mov [ebx + 8], eax  // Registers.edx
        mov eax, [esp + 48] // Original EBX
        mov [ebx + 12], eax // Registers.ebx
        mov eax, [esp + 52] // Original ESP
        mov [ebx + 16], eax // Registers.esp
        mov eax, [esp + 56] // Original EBP
        mov [ebx + 20], eax // Registers.ebp
        mov eax, [esp + 60] // Original ESI
        mov [ebx + 24], eax // Registers.esi
        mov eax, [esp + 64] // Original EDI
        mov [ebx + 28], eax // Registers.edi
        
        // Вызываем callback с указателем на структуру
        push ebx            // Аргумент - указатель на структуру
        call GetHookInstance
        test eax, eax      // Проверяем валидность instance
        jz skip_callback
        mov ecx, [eax]     // this указатель
        test ecx, ecx      // Проверяем валидность this
        jz skip_callback
        mov ecx, [ecx + 28] // m_callback (смещение 28 байт)
        test ecx, ecx      // Проверяем валидность callback
        jz skip_callback
        call ecx           // Вызов callback
        
    skip_callback:
        add esp, 4         // Очищаем аргумент
        add esp, 32        // Освобождаем структуру

    cleanup:
        pop ebx           // Восстанавливаем ebx
        
        // Восстанавливаем регистры и флаги
        popad
        popfd
        
        // Прыгаем на оригинальный код функции
        call GetHookInstance
        test eax, eax     // Проверяем валидность instance
        jz original_code
        mov eax, [eax]    // this указатель
        test eax, eax     // Проверяем валидность this
        jz original_code
        mov eax, [eax]    // m_targetFunction (смещение 0)
        test eax, eax     // Проверяем валидность адреса
        jz original_code
        add eax, 5        // Пропускаем инструкцию jump
        jmp eax
    
    original_code:
        ret              // Если что-то пошло не так, просто возвращаемся
    }
}

RegisterHook::RegisterHook(void* targetFunction, Register registersToHook, RegisterCallback callback)
    : m_targetFunction(targetFunction)
    , m_registers(registersToHook)
    , m_callback(std::move(callback))
{
    std::unique_lock<std::shared_mutex> lock(s_instanceMutex);
    s_instance = this;
    g_hookFunction = (void*)HookFunction;
    m_hookAddress = targetFunction;
    m_installed = false;
}

RegisterHook::~RegisterHook()
{
    if (m_installed) {
        uninstall();
    }
    
    std::unique_lock<std::shared_mutex> lock(s_instanceMutex);
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool RegisterHook::install()
{
    if (m_installed || !m_targetFunction || !g_hookFunction) {
        LogManager::instance().error("Cannot install hook - invalid state", "Hooks");
        return false;
    }

    try {
        HANDLE processHandle = MemoryManager::instance().GetProcessHandle();
        
        LogManager::instance().debug(
            QString("Installing hook at 0x%1")
                .arg(QString::number((quintptr)m_targetFunction, 16)),
            "Hooks"
        );
        
        bool success = InstallHookRemote(
            processHandle,
            m_targetFunction,
            g_hookFunction
        );
        
        if (success) {
            m_installed = true;
            LogManager::instance().debug("Hook installed successfully", "Hooks");
        }
        
        return success;
    }
    catch (const std::exception& e) {
        LogManager::instance().error(
            QString("Exception while installing hook: %1").arg(e.what()),
            "Hooks"
        );
        return false;
    }
}

bool RegisterHook::uninstall()
{
    if (!m_installed || !m_targetFunction) {
        return false;
    }

    try {
        HANDLE processHandle = MemoryManager::instance().GetProcessHandle();
        
        LogManager::instance().debug(
            QString("Uninstalling hook at 0x%1")
                .arg(QString::number((quintptr)m_targetFunction, 16)),
            "Hooks"
        );
        
        bool success = UninstallHookRemote(
            processHandle,
            m_targetFunction,
            m_originalBytes
        );
        
        if (success) {
            m_installed = false;
            LogManager::instance().debug("Hook uninstalled successfully", "Hooks");
        }
        
        return success;
    }
    catch (const std::exception& e) {
        LogManager::instance().error(
            QString("Exception while uninstalling hook: %1").arg(e.what()),
            "Hooks"
        );
        return false;
    }
}

bool RegisterHook::isInstalled() const
{
    return m_installed;
}