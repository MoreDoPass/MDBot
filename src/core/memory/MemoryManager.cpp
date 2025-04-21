#include "MemoryManager.hpp"

#include <TlHelp32.h>

#include <algorithm>

#include <QDebug>

#include <system_error>


using namespace std;


#pragma region Initialization & Lifecycle
MemoryManager::MemoryManager(DWORD pid) : processId(pid), baseAddress(0)
{
    // Запрашиваем все необходимые права для работы с процессом
    processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | // Для получения информации о процессе
                                    PROCESS_VM_READ |       // Для чтения памяти
                                    PROCESS_VM_WRITE |      // Для записи в память
                                    PROCESS_VM_OPERATION |  // Для VirtualAllocEx/VirtualFreeEx
                                    PROCESS_CREATE_THREAD | // Для CreateRemoteThread
                                    PROCESS_SUSPEND_RESUME, // Для управления потоками
                                FALSE,
                                pid);

    if (!processHandle)
    {
        DWORD error = GetLastError();
        qDebug() << "Failed to open process" << pid << "with error:" << error;
        ThrowLastError("Failed to open process with required access rights");
    }

    qDebug() << "Successfully opened process" << pid
             << "with handle:" << QString::number(reinterpret_cast<quintptr>(processHandle), 16);

    // Получаем базовый адрес при создании
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
#pragma endregion Initialization& Lifecycle

#pragma region Module Operations
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
#pragma endregion Module Operations

#pragma region Error Handling & Validation
bool MemoryManager::IsValidAddress(uintptr_t address) const
{
    MEMORY_BASIC_INFORMATION mbi;
    return VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi)) != 0;
}

void MemoryManager::ThrowLastError(const char* message) const
{
    throw std::system_error(GetLastError(), std::system_category(), message);
}
#pragma endregion Error Handling& Validation

#pragma region Read/Write Operations
#pragma region Read Operations
bool MemoryManager::ReadMemory(uintptr_t address, void* buffer, size_t size)
{
    SIZE_T bytesRead;
    if (!ReadProcessMemory(processHandle, (LPCVOID)address, buffer, size, &bytesRead))
    {
        qDebug() << "Failed to read memory at" << QString::number(address, 16) << "of size" << size
                 << "Error:" << GetLastError();
        return false;
    }
    return bytesRead == size;
}

std::string MemoryManager::ReadString(uintptr_t address, size_t maxLength, bool isRelative)
{
    uintptr_t         finalAddress = isRelative ? ResolveAddress(address) : address;
    std::vector<char> buffer(maxLength + 1); // +1 для нуль-терминатора

    if (!ReadMemory(finalAddress, buffer.data(), maxLength))
    {
        ThrowLastError("Failed to read string");
    }

    // Находим нуль-терминатор
    buffer[maxLength] = '\0'; // Гарантируем нуль-терминацию
    size_t length     = 0;
    while (length < maxLength && buffer[length] != '\0')
    {
        length++;
    }

    return std::string(buffer.data(), length);
}
#pragma endregion Read Operations

#pragma region Write Operations
bool MemoryManager::WriteMemory(uintptr_t address, const void* buffer, size_t size)
{
    SIZE_T bytesWritten;
    if (!WriteProcessMemory(processHandle, (LPVOID)address, buffer, size, &bytesWritten))
    {
        qDebug() << "Failed to write memory at" << QString::number(address, 16) << "of size" << size
                 << "Error:" << GetLastError();
        return false;
    }
    return bytesWritten == size;
}

bool MemoryManager::WriteString(uintptr_t address, const std::string& str, bool isRelative)
{
    uintptr_t finalAddress = isRelative ? ResolveAddress(address) : address;
    // Ограничиваем длину строки 12 символами (максимум в WoW)
    size_t writeLength = std::min(str.length(), size_t(12));

    return WriteMemory(finalAddress, str.c_str(), writeLength + 1); // +1 для нуль-терминатора
}
#pragma endregion Write Operations
#pragma endregion Read / Write Operations

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
