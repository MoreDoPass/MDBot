/**
 * @file Hook.hpp
 * @brief Базовый класс для всех хуков
 */
#pragma once
#include <memory>
#include <vector>

#include "core/memory/MemoryManager.hpp"
#include "Types.hpp"


/**
 * @class Hook
 * @brief Базовый класс для всех типов хуков
 * @details Предоставляет базовый функционал для установки/снятия хуков
 */
class Hook
{
  public:
    explicit Hook(std::shared_ptr<MemoryManager> memory);
    virtual ~Hook() = default;

    /**
     * @brief Устанавливает хук
     * @return true если установка успешна
     */
    virtual bool install() = 0;

    /**
     * @brief Снимает хук
     * @return true если снятие успешно
     */
    virtual bool uninstall() = 0;

    /**
     * @brief Проверяет установлен ли хук
     */
    bool isInstalled() const { return m_installed; }

    /**
     * @brief Получает последнюю ошибку
     */
    HookError getLastError() const { return m_lastError; }

  protected:
    /**
     * @brief Изменяет права доступа к памяти
     */
    bool setMemoryProtection(void* address, size_t size, DWORD protection);

    /**
     * @brief Сохраняет оригинальные байты
     */
    bool saveOriginalBytes(void* address, size_t size);

    /**
     * @brief Восстанавливает оригинальные байты
     */
    bool restoreOriginalBytes();

    /**
     * @brief Устанавливает ошибку
     */
    void setError(HookError error);

  protected:
    std::shared_ptr<MemoryManager> m_memory;                     ///< Менеджер памяти
    bool                           m_installed{false};           ///< Флаг установки
    HookError                      m_lastError{HookError::None}; ///< Последняя ошибка
    void*                          m_targetAddress{nullptr};     ///< Целевой адрес
    std::vector<uint8_t>           m_originalBytes;              ///< Оригинальные байты
};