/**
 * @file Trampoline.hpp
 * @brief Базовый класс для работы с трамплином
 */
#pragma once
#include <memory>
#include <vector>

#include "core/memory/MemoryManager.hpp"


/**
 * @class Trampoline
 * @brief Класс для управления трамплином
 * @details Трамплин - это область памяти, куда копируются оригинальные
 * инструкции и добавляется код возврата в оригинальную функцию
 */
class Trampoline
{
  public:
    explicit Trampoline(std::shared_ptr<MemoryManager> memory);
    ~Trampoline();

    /**
     * @brief Выделяет память под трамплин
     * @param size Необходимый размер
     * @return true если память выделена успешно
     */
    bool allocate(size_t size);

    /**
     * @brief Освобождает память трамплина
     */
    void free();

    /**
     * @brief Копирует код в трамплин
     * @param code Код для копирования
     * @param size Размер кода
     * @return true если копирование успешно
     */
    bool write(const void* code, size_t size);

    /**
     * @brief Получает адрес трамплина
     */
    void* getAddress() const { return m_address; }

    /**
     * @brief Проверяет выделен ли трамплин
     */
    bool isAllocated() const { return m_address != nullptr; }

  private:
    std::shared_ptr<MemoryManager> m_memory;           ///< Менеджер памяти
    void*                          m_address{nullptr}; ///< Адрес трамплина
    size_t                         m_size{0};          ///< Размер трамплина
};