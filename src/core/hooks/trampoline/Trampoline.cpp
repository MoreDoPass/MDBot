#include "Trampoline.hpp"

#include "gui/log/LogManager.hpp"


Trampoline::Trampoline(std::shared_ptr<MemoryManager> memory) : m_memory(std::move(memory)) {}

Trampoline::~Trampoline()
{
    free();
}

bool Trampoline::allocate(size_t size)
{
    // Освобождаем старый трамплин если был
    free();

    // Выделяем память с правами execute
    m_address = m_memory->AllocateMemory(nullptr, size, PAGE_EXECUTE_READWRITE);
    if (!m_address)
    {
        LogManager::instance().error(QString("Failed to allocate trampoline memory of size %1").arg(size), "Hooks");
        return false;
    }

    m_size = size;
    LogManager::instance().debug(QString("Allocated trampoline at 0x%1 with size %2")
                                     .arg(QString::number(reinterpret_cast<uintptr_t>(m_address), 16))
                                     .arg(m_size),
                                 "Hooks");
    return true;
}

void Trampoline::free()
{
    if (m_address)
    {
        m_memory->FreeMemory(m_address);
        m_address = nullptr;
        m_size    = 0;
    }
}

bool Trampoline::write(const void* code, size_t size)
{
    if (!m_address || !code || size > m_size)
    {
        return false;
    }

    if (!m_memory->WriteMemory(reinterpret_cast<uintptr_t>(m_address), code, size))
    {
        LogManager::instance().error(QString("Failed to write code to trampoline at 0x%1")
                                         .arg(QString::number(reinterpret_cast<uintptr_t>(m_address), 16)),
                                     "Hooks");
        return false;
    }

    return true;
}