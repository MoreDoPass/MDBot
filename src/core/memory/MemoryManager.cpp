#include "MemoryManager.hpp"
#include <system_error>
#include <TlHelp32.h>
#include <algorithm> // для std::min
#include <QDebug> // для qDebug

using namespace std; // добавляем using directive

// Инициализация статического члена
std::shared_ptr<MemoryManager> MemoryManager::s_instance;

MemoryManager::MemoryManager(DWORD pid) : processId(pid), baseAddress(0) {
    // Запрашиваем все возможные права доступа к процессу
    processHandle = OpenProcess(
        PROCESS_ALL_ACCESS,  // Запрашиваем полный доступ
        FALSE, 
        processId
    );
    
    if (!processHandle) {
        DWORD error = GetLastError();
        qDebug() << "Failed to open process with error:" << error 
                 << "Process ID:" << processId;
                 
        if (error == ERROR_ACCESS_DENIED) {
            throw std::runtime_error(
                "Access denied when opening process. "
                "The application must be run as administrator to modify memory."
            );
        } else if (error == ERROR_INVALID_PARAMETER) {
            throw std::runtime_error("Invalid process ID specified.");
        }
        
        throw std::system_error(
            error, 
            std::system_category(), 
            "Failed to open process"
        );
    }
    
    qDebug() << "Successfully opened process" << processId 
             << "with handle:" << processHandle;
             
    UpdateBaseAddress();
}

MemoryManager::~MemoryManager() {
    if (processHandle) {
        CloseHandle(processHandle);
    }
}

void MemoryManager::UpdateBaseAddress() {
    baseAddress = GetModuleBaseAddress();
    if (baseAddress == 0) {
        qDebug() << "Failed to get base address for process" << processId;
        qDebug() << "Last error:" << GetLastError();
        ThrowLastError("Failed to get run.exe base address");
    }
    qDebug() << "Successfully got base address:" << QString::number(baseAddress, 16)
             << "for process" << processId;
}

uintptr_t MemoryManager::GetModuleBaseAddress(const wchar_t* moduleName) {
    qDebug() << "Attempting to get module base address for" << QString::fromWCharArray(moduleName)
             << "in process" << processId;
             
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (snapshot == INVALID_HANDLE_VALUE) {
        qDebug() << "Failed to create module snapshot. Error:" << GetLastError();
        ThrowLastError("Failed to create module snapshot");
    }

    MODULEENTRY32W moduleEntry;
    moduleEntry.dwSize = sizeof(moduleEntry);

    if (Module32FirstW(snapshot, &moduleEntry)) {
        do {
            if (_wcsicmp(moduleEntry.szModule, moduleName) == 0) {
                CloseHandle(snapshot);
                qDebug() << "Found module" << QString::fromWCharArray(moduleEntry.szModule)
                         << "at address:" << QString::number(reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr), 16);
                return reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
            }
        } while (Module32NextW(snapshot, &moduleEntry));
    }

    qDebug() << "Module" << QString::fromWCharArray(moduleName) << "not found in process" << processId;
    CloseHandle(snapshot);
    return 0;
}

uintptr_t MemoryManager::ResolveAddress(uintptr_t relativeAddress) {
    if (baseAddress == 0) {
        UpdateBaseAddress();
    }
    return baseAddress + relativeAddress;
}

std::string MemoryManager::ReadString(uintptr_t address, size_t maxLength) {
    std::vector<char> buffer(maxLength + 1); // +1 для нуль-терминатора
    SIZE_T bytesRead;

    if (!ReadProcessMemory(processHandle, (LPCVOID)address, buffer.data(), maxLength, &bytesRead)) {
        ThrowLastError("Failed to read string");
    }

    // Находим нуль-терминатор
    buffer[maxLength] = '\0'; // Гарантируем нуль-терминацию
    size_t length = 0;
    while (length < bytesRead && buffer[length] != '\0') {
        length++;
    }

    return std::string(buffer.data(), length);
}

std::string MemoryManager::ReadStringRelative(uintptr_t relativeAddress, size_t maxLength) {
    return ReadString(ResolveAddress(relativeAddress), maxLength);
}

bool MemoryManager::WriteString(uintptr_t address, const string& str) {
    SIZE_T bytesWritten;
    // Ограничиваем длину строки 12 символами (максимум в WoW)
    size_t writeLength = min(str.length(), size_t(12));
    return WriteProcessMemory(processHandle, (LPVOID)address, str.c_str(), 
                            writeLength + 1, &bytesWritten)  // +1 для нуль-терминатора
           && bytesWritten == writeLength + 1;
}

bool MemoryManager::WriteStringRelative(uintptr_t relativeAddress, const std::string& str) {
    return WriteString(ResolveAddress(relativeAddress), str);
}

uintptr_t MemoryManager::FindPattern(const char* pattern, const char* mask) {
    // TODO: В будущем реализуем поиск паттернов
    // Пока что возвращаем 0, так как функция еще не реализована
    return 0;
}

bool MemoryManager::IsValidAddress(uintptr_t address) const {
    MEMORY_BASIC_INFORMATION mbi;
    return VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi)) != 0;
}

DWORD MemoryManager::GetMemoryProtection(uintptr_t address) const {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi))) {
        return mbi.Protect;
    }
    return 0;
}

bool MemoryManager::SetMemoryProtection(uintptr_t address, size_t size, DWORD protection) {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi))) {
        qDebug() << "Current memory protection at" << QString::number(address, 16) 
                 << "is:" << QString::number(mbi.Protect, 16);
    }
    
    DWORD oldProtect;
    bool result = VirtualProtectEx(processHandle, (LPVOID)address, size, protection, &oldProtect);
    
    if (!result) {
        DWORD error = GetLastError();
        qDebug() << "VirtualProtectEx failed with error:" << error 
                 << "at address:" << QString::number(address, 16)
                 << "requested protection:" << QString::number(protection, 16);
    } else {
        qDebug() << "Successfully changed memory protection at" << QString::number(address, 16)
                 << "from:" << QString::number(oldProtect, 16)
                 << "to:" << QString::number(protection, 16);
    }
    
    return result;
}

bool MemoryManager::EnsureMemoryAccess(uintptr_t address, size_t size, DWORD requiredAccess) {
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi))) {
        return false;
    }

    if ((mbi.Protect & requiredAccess) == requiredAccess) {
        return true;
    }

    DWORD oldProtect;
    return VirtualProtectEx(processHandle, (LPVOID)address, size, 
                          mbi.Protect | requiredAccess, &oldProtect);
}

void MemoryManager::ThrowLastError(const char* message) const {
    throw std::system_error(GetLastError(), std::system_category(), message);
}