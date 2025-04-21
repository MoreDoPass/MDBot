#include "Hook.hpp"

#include "gui/log/LogManager.hpp"


Hook::Hook(std::shared_ptr<MemoryManager> memory) : m_memory(std::move(memory)) {}

bool Hook::setMemoryProtection(void* address, size_t size, DWORD protection)
{
    if (!m_memory->SetMemoryProtection(reinterpret_cast<uintptr_t>(address), size, protection))
    {
        setError(HookError::MemoryProtect);
        LogManager::instance().error(QString("Failed to change memory protection at 0x%1")
                                         .arg(QString::number(reinterpret_cast<uintptr_t>(address), 16)),
                                     "Hooks");
        return false;
    }
    return true;
}

bool Hook::saveOriginalBytes(void* address, size_t size)
{
    m_originalBytes.resize(size);
    if (!m_memory->ReadMemory(reinterpret_cast<uintptr_t>(address), m_originalBytes.data(), size))
    {
        setError(HookError::WriteMemory);
        LogManager::instance().error(QString("Failed to read original bytes at 0x%1")
                                         .arg(QString::number(reinterpret_cast<uintptr_t>(address), 16)),
                                     "Hooks");
        return false;
    }
    return true;
}

bool Hook::restoreOriginalBytes()
{
    if (m_originalBytes.empty() || !m_targetAddress)
    {
        return false;
    }

    DWORD oldProtect;
    if (!setMemoryProtection(m_targetAddress, m_originalBytes.size(), PAGE_EXECUTE_READWRITE))
    {
        return false;
    }

    if (!m_memory->WriteMemory(
            reinterpret_cast<uintptr_t>(m_targetAddress), m_originalBytes.data(), m_originalBytes.size()))
    {
        setError(HookError::WriteMemory);
        LogManager::instance().error(QString("Failed to restore original bytes at 0x%1")
                                         .arg(QString::number(reinterpret_cast<uintptr_t>(m_targetAddress), 16)),
                                     "Hooks");

        // Восстанавливаем права доступа
        setMemoryProtection(m_targetAddress, m_originalBytes.size(), oldProtect);
        return false;
    }

    // Восстанавливаем права доступа
    if (!setMemoryProtection(m_targetAddress, m_originalBytes.size(), oldProtect))
    {
        return false;
    }

    return true;
}

void Hook::setError(HookError error)
{
    m_lastError = error;
}