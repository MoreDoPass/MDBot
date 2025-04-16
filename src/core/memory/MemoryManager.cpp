#include "MemoryManager.hpp"
#include <QDebug>
#include <TlHelp32.h>
#include <algorithm>
#include <system_error>


using namespace std;

MemoryManager::MemoryManager(DWORD pid) : processId(pid), baseAddress(0)
{
    // Запрашиваем все возможные права доступа к процессу
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, // Запрашиваем полный доступ
                                FALSE,
                                processId);

    if (!processHandle)
    {
        DWORD error = GetLastError();
        qDebug() << "Failed to open process with error:" << error << "Process ID:" << processId;

        if (error == ERROR_ACCESS_DENIED)
        {
            throw std::runtime_error("Access denied when opening process. "
                                     "The application must be run as administrator to modify memory.");
        }
        else if (error == ERROR_INVALID_PARAMETER)
        {
            throw std::runtime_error("Invalid process ID specified.");
        }

        throw std::system_error(error, std::system_category(), "Failed to open process");
    }

    qDebug() << "Successfully opened process" << processId << "with handle:" << processHandle;

    UpdateBaseAddress();
}

MemoryManager::~MemoryManager()
{
    if (processHandle)
    {
        CloseHandle(processHandle);
        processHandle = nullptr;
    }
}

// Move конструктор
MemoryManager::MemoryManager(MemoryManager&& other) noexcept
    : processHandle(other.processHandle), processId(other.processId), baseAddress(other.baseAddress)
{
    // Обнуляем handle в другом объекте
    other.processHandle = nullptr;
    other.processId     = 0;
    other.baseAddress   = 0;
}

// Move оператор присваивания
MemoryManager& MemoryManager::operator=(MemoryManager&& other) noexcept
{
    if (this != &other)
    {
        // Закрываем текущий handle если есть
        if (processHandle)
        {
            CloseHandle(processHandle);
        }

        // Перемещаем данные
        processHandle = other.processHandle;
        processId     = other.processId;
        baseAddress   = other.baseAddress;

        // Обнуляем в другом объекте
        other.processHandle = nullptr;
        other.processId     = 0;
        other.baseAddress   = 0;
    }
    return *this;
}

void MemoryManager::UpdateBaseAddress()
{
    baseAddress = GetModuleBaseAddress();
    if (baseAddress == 0)
    {
        qDebug() << "Failed to get base address for process" << processId;
        qDebug() << "Last error:" << GetLastError();
        ThrowLastError("Failed to get run.exe base address");
    }
    qDebug() << "Successfully got base address:" << QString::number(baseAddress, 16) << "for process" << processId;
}

uintptr_t MemoryManager::GetModuleBaseAddress(const wchar_t* moduleName)
{
    qDebug() << "Attempting to get module base address for" << QString::fromWCharArray(moduleName) << "in process"
             << processId;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        qDebug() << "Failed to create module snapshot. Error:" << GetLastError();
        ThrowLastError("Failed to create module snapshot");
    }

    MODULEENTRY32W moduleEntry;
    moduleEntry.dwSize = sizeof(moduleEntry);

    if (Module32FirstW(snapshot, &moduleEntry))
    {
        do
        {
            if (_wcsicmp(moduleEntry.szModule, moduleName) == 0)
            {
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

uintptr_t MemoryManager::ResolveAddress(uintptr_t relativeAddress)
{
    if (baseAddress == 0)
    {
        UpdateBaseAddress();
    }
    return baseAddress + relativeAddress;
}

bool MemoryManager::IsValidAddress(uintptr_t address) const
{
    MEMORY_BASIC_INFORMATION mbi;
    return VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi)) != 0;
}

void MemoryManager::ThrowLastError(const char* message) const
{
    throw std::system_error(GetLastError(), std::system_category(), message);
}
// clang-format off
#pragma region Read/Write Operations
    #pragma region Read Operations
    std::string MemoryManager::ReadString(uintptr_t address, size_t maxLength)
    {
        std::vector<char> buffer(maxLength + 1); // +1 для нуль-терминатора
        SIZE_T            bytesRead;

        if (!ReadProcessMemory(processHandle, (LPCVOID)address, buffer.data(), maxLength, &bytesRead))
        {
            ThrowLastError("Failed to read string");
        }

        // Находим нуль-терминатор
        buffer[maxLength] = '\0'; // Гарантируем нуль-терминацию
        size_t length     = 0;
        while (length < bytesRead && buffer[length] != '\0')
        {
            length++;
        }

        return std::string(buffer.data(), length);
    }
    std::string MemoryManager::ReadStringRelative(uintptr_t relativeAddress, size_t maxLength)
    {
        return ReadString(ResolveAddress(relativeAddress), maxLength);
    }
    #pragma endregion Read Operations
    #pragma region Write Operations
    bool MemoryManager::WriteString(uintptr_t address, const string& str)
    {
        SIZE_T bytesWritten;
        // Ограничиваем длину строки 12 символами (максимум в WoW)
        size_t writeLength = min(str.length(), size_t(12));
        return WriteProcessMemory(
                processHandle, (LPVOID)address, str.c_str(), writeLength + 1, &bytesWritten) // +1 для нуль-терминатора
            && bytesWritten == writeLength + 1;
    }
    bool MemoryManager::WriteStringRelative(uintptr_t relativeAddress, const std::string& str)
    {
        return WriteString(ResolveAddress(relativeAddress), str);
    }
    #pragma endregion Write Operations
#pragma endregion Read / Write Operations
// clang-format on
#pragma region Memory Operations
void* MemoryManager::AllocateMemory(void* address, size_t size, DWORD protection)
{
    qDebug() << "Attempting to allocate" << size << "bytes at address"
             << QString::number(reinterpret_cast<quintptr>(address), 16) << "with protection"
             << QString::number(protection, 16);

    void* allocatedAddress = VirtualAllocEx(processHandle, address, size, MEM_COMMIT | MEM_RESERVE, protection);

    if (!allocatedAddress)
    {
        DWORD error = GetLastError();
        qDebug() << "VirtualAllocEx failed with error:" << error;
        return nullptr;
    }

    qDebug() << "Successfully allocated memory at" << QString::number(reinterpret_cast<quintptr>(allocatedAddress), 16);

    return allocatedAddress;
}

bool MemoryManager::FreeMemory(void* address)
{
    qDebug() << "Attempting to free memory at address" << QString::number(reinterpret_cast<quintptr>(address), 16);

    if (!VirtualFreeEx(processHandle, address, 0, MEM_RELEASE))
    {
        DWORD error = GetLastError();
        qDebug() << "VirtualFreeEx failed with error:" << error;
        return false;
    }

    qDebug() << "Successfully freed memory at" << QString::number(reinterpret_cast<quintptr>(address), 16);

    return true;
}

bool MemoryManager::SetMemoryProtection(uintptr_t address, size_t size, DWORD protection)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi)))
    {
        qDebug() << "Current memory protection at" << QString::number(address, 16)
                 << "is:" << QString::number(mbi.Protect, 16);
    }

    DWORD oldProtect;
    bool  result = VirtualProtectEx(processHandle, (LPVOID)address, size, protection, &oldProtect);

    if (!result)
    {
        DWORD error = GetLastError();
        qDebug() << "VirtualProtectEx failed with error:" << error << "at address:" << QString::number(address, 16)
                 << "requested protection:" << QString::number(protection, 16);
    }
    else
    {
        qDebug() << "Successfully changed memory protection at" << QString::number(address, 16)
                 << "from:" << QString::number(oldProtect, 16) << "to:" << QString::number(protection, 16);
    }

    return result;
}

DWORD MemoryManager::GetMemoryProtection(uintptr_t address) const
{
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi)))
    {
        return mbi.Protect;
    }
    return 0;
}

bool MemoryManager::EnsureMemoryAccess(uintptr_t address, size_t size, DWORD requiredAccess)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi)))
    {
        return false;
    }

    if ((mbi.Protect & requiredAccess) == requiredAccess)
    {
        return true;
    }

    DWORD oldProtect;
    return VirtualProtectEx(processHandle, (LPVOID)address, size, mbi.Protect | requiredAccess, &oldProtect);
}
#pragma endregion Memory Operations