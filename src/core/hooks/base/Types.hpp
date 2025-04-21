/**
 * @file Types.hpp
 * @brief Базовые типы и структуры для работы с хуками
 */
#pragma once
#include <windows.h>

#include <cstdint>


/**
 * @brief Структура для хранения оригинальных байтов и адресов
 */
#pragma pack(push, 1)
struct HookContext
{
    void*   targetAddress;    ///< Адрес где установлен хук
    void*   hookFunction;     ///< Адрес функции-перехватчика
    uint8_t originalBytes[5]; ///< Оригинальные байты
};
#pragma pack(pop)

/**
 * @brief Тип ошибки хука
 */
enum class HookError
{
    None,
    InvalidAddress,
    MemoryProtect,
    WriteMemory,
    CreateTrampoline
};