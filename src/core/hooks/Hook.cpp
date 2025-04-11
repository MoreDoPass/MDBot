#include "Hook.hpp"
#include "gui/LogManager.hpp"
#include <QDebug>

namespace {
    /**
     * @brief Код для выполнения в удаленном потоке
     * @param lpParameter Указатель на структуру HookData
     * @return Результат установки хука
     */
    DWORD WINAPI RemoteHookProc(LPVOID lpParameter)
    {
        __try {
            auto* hookData = static_cast<HookData*>(lpParameter);
            
            // Проверяем валидность адресов
            if (!hookData || !hookData->targetAddress || !hookData->hookFunction) {
                return ERROR_INVALID_PARAMETER;
            }
            
            // Сохраняем оригинальные байты
            if (IsBadReadPtr(hookData->targetAddress, 5)) {
                return ERROR_INVALID_ADDRESS;
            }
            memcpy(hookData->originalBytes, hookData->targetAddress, 5);
            
            // Создаем инструкцию перехода (E9 + relative address)
            hookData->hookBytes[0] = 0xE9;
            DWORD relativeAddress = (DWORD)hookData->hookFunction - (DWORD)hookData->targetAddress - 5;
            memcpy(&hookData->hookBytes[1], &relativeAddress, 4);

            // Изменяем права доступа
            DWORD oldProtect;
            if (!VirtualProtect(hookData->targetAddress, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                return GetLastError();
            }

            // Устанавливаем хук
            memcpy(hookData->targetAddress, hookData->hookBytes, 5);

            // Восстанавливаем права доступа
            DWORD temp;
            VirtualProtect(hookData->targetAddress, 5, oldProtect, &temp);

            // Сбрасываем кэш инструкций
            FlushInstructionCache(GetCurrentProcess(), hookData->targetAddress, 5);

            return ERROR_SUCCESS;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }

    /**
     * @brief Код для удаления хука в удаленном потоке
     * @param lpParameter Указатель на структуру HookData
     * @return Результат удаления хука
     */
    DWORD WINAPI RemoteUnhookProc(LPVOID lpParameter)
    {
        __try {
            auto* hookData = static_cast<HookData*>(lpParameter);
            
            // Проверяем валидность адресов
            if (!hookData || !hookData->targetAddress) {
                return ERROR_INVALID_PARAMETER;
            }

            // Изменяем права доступа
            DWORD oldProtect;
            if (!VirtualProtect(hookData->targetAddress, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                return GetLastError();
            }

            // Восстанавливаем оригинальные байты
            if (IsBadWritePtr(hookData->targetAddress, 5)) {
                return ERROR_INVALID_ADDRESS;
            }
            memcpy(hookData->targetAddress, hookData->originalBytes, 5);

            // Восстанавливаем права доступа
            DWORD temp;
            VirtualProtect(hookData->targetAddress, 5, oldProtect, &temp);

            // Сбрасываем кэш инструкций
            FlushInstructionCache(GetCurrentProcess(), hookData->targetAddress, 5);

            return ERROR_SUCCESS;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }
}

bool __cdecl Hook::InstallHookRemote(HANDLE processHandle, void* targetAddress, void* hookFunction)
{
    // Выделяем память в удаленном процессе для структуры HookData
    HookData hookData = { targetAddress, hookFunction };
    void* remoteData = VirtualAllocEx(
        processHandle, 
        nullptr, 
        sizeof(HookData), 
        MEM_COMMIT | MEM_RESERVE, 
        PAGE_READWRITE
    );

    if (!remoteData) {
        DWORD error = GetLastError();
        LogManager::instance().error(
            QString("Failed to allocate memory in target process. Error: %1")
                .arg(error),
            "Hooks"
        );
        return false;
    }

    // Копируем структуру в удаленный процесс
    if (!WriteProcessMemory(processHandle, remoteData, &hookData, sizeof(HookData), nullptr)) {
        DWORD error = GetLastError();
        LogManager::instance().error(
            QString("Failed to write hook data to target process. Error: %1")
                .arg(error),
            "Hooks"
        );
        VirtualFreeEx(processHandle, remoteData, 0, MEM_RELEASE);
        return false;
    }

    // Выделяем память для кода процедуры
    void* remoteCode = VirtualAllocEx(
        processHandle,
        nullptr,
        1024, // Достаточно для кода функции
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );

    if (!remoteCode) {
        DWORD error = GetLastError();
        LogManager::instance().error(
            QString("Failed to allocate memory for remote code. Error: %1")
                .arg(error),
            "Hooks"
        );
        VirtualFreeEx(processHandle, remoteData, 0, MEM_RELEASE);
        return false;
    }

    // Копируем код процедуры в удаленный процесс
    if (!WriteProcessMemory(processHandle, remoteCode, RemoteHookProc, 1024, nullptr)) {
        DWORD error = GetLastError();
        LogManager::instance().error(
            QString("Failed to write remote code. Error: %1")
                .arg(error),
            "Hooks"
        );
        VirtualFreeEx(processHandle, remoteCode, 0, MEM_RELEASE);
        VirtualFreeEx(processHandle, remoteData, 0, MEM_RELEASE);
        return false;
    }

    // Создаем удаленный поток для выполнения кода
    HANDLE thread = CreateRemoteThread(
        processHandle,
        nullptr,
        0,
        (LPTHREAD_START_ROUTINE)remoteCode,
        remoteData,
        0,
        nullptr
    );

    if (!thread) {
        DWORD error = GetLastError();
        LogManager::instance().error(
            QString("Failed to create remote thread. Error: %1")
                .arg(error),
            "Hooks"
        );
        VirtualFreeEx(processHandle, remoteCode, 0, MEM_RELEASE);
        VirtualFreeEx(processHandle, remoteData, 0, MEM_RELEASE);
        return false;
    }

    // Ждем завершения потока
    WaitForSingleObject(thread, INFINITE);

    // Получаем код возврата
    DWORD exitCode;
    GetExitCodeThread(thread, &exitCode);

    // Освобождаем ресурсы
    CloseHandle(thread);
    VirtualFreeEx(processHandle, remoteCode, 0, MEM_RELEASE);
    VirtualFreeEx(processHandle, remoteData, 0, MEM_RELEASE);

    if (exitCode != 0) {
        LogManager::instance().error(
            QString("Remote hook installation failed with error: %1")
                .arg(exitCode),
            "Hooks"
        );
        return false;
    }

    return true;
}

bool __cdecl Hook::UninstallHookRemote(HANDLE processHandle, void* targetAddress, const BYTE* originalBytes)
{
    // Выделяем память в удаленном процессе для структуры HookData
    HookData hookData = { targetAddress };
    memcpy(hookData.originalBytes, originalBytes, 5);
    
    void* remoteData = VirtualAllocEx(
        processHandle,
        nullptr,
        sizeof(HookData),
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!remoteData) {
        DWORD error = GetLastError();
        LogManager::instance().error(
            QString("Failed to allocate memory in target process. Error: %1")
                .arg(error),
            "Hooks"
        );
        return false;
    }

    // Копируем структуру в удаленный процесс
    if (!WriteProcessMemory(processHandle, remoteData, &hookData, sizeof(HookData), nullptr)) {
        DWORD error = GetLastError();
        LogManager::instance().error(
            QString("Failed to write hook data to target process. Error: %1")
                .arg(error),
            "Hooks"
        );
        VirtualFreeEx(processHandle, remoteData, 0, MEM_RELEASE);
        return false;
    }

    // Выделяем память для кода процедуры
    void* remoteCode = VirtualAllocEx(
        processHandle,
        nullptr,
        1024,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );

    if (!remoteCode) {
        DWORD error = GetLastError();
        LogManager::instance().error(
            QString("Failed to allocate memory for remote code. Error: %1")
                .arg(error),
            "Hooks"
        );
        VirtualFreeEx(processHandle, remoteData, 0, MEM_RELEASE);
        return false;
    }

    // Копируем код процедуры в удаленный процесс
    if (!WriteProcessMemory(processHandle, remoteCode, RemoteUnhookProc, 1024, nullptr)) {
        DWORD error = GetLastError();
        LogManager::instance().error(
            QString("Failed to write remote code. Error: %1")
                .arg(error),
            "Hooks"
        );
        VirtualFreeEx(processHandle, remoteCode, 0, MEM_RELEASE);
        VirtualFreeEx(processHandle, remoteData, 0, MEM_RELEASE);
        return false;
    }

    // Создаем удаленный поток для выполнения кода
    HANDLE thread = CreateRemoteThread(
        processHandle,
        nullptr,
        0,
        (LPTHREAD_START_ROUTINE)remoteCode,
        remoteData,
        0,
        nullptr
    );

    if (!thread) {
        DWORD error = GetLastError();
        LogManager::instance().error(
            QString("Failed to create remote thread. Error: %1")
                .arg(error),
            "Hooks"
        );
        VirtualFreeEx(processHandle, remoteCode, 0, MEM_RELEASE);
        VirtualFreeEx(processHandle, remoteData, 0, MEM_RELEASE);
        return false;
    }

    // Ждем завершения потока
    WaitForSingleObject(thread, INFINITE);

    // Получаем код возврата
    DWORD exitCode;
    GetExitCodeThread(thread, &exitCode);

    // Освобождаем ресурсы
    CloseHandle(thread);
    VirtualFreeEx(processHandle, remoteCode, 0, MEM_RELEASE);
    VirtualFreeEx(processHandle, remoteData, 0, MEM_RELEASE);

    if (exitCode != 0) {
        LogManager::instance().error(
            QString("Remote hook uninstallation failed with error: %1")
                .arg(exitCode),
            "Hooks"
        );
        return false;
    }

    return true;
}