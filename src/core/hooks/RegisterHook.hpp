/**
 * @file RegisterHook.hpp
 * @brief Модуль для перехвата и модификации регистров процессора
 * @details Предоставляет механизм для установки хуков на функции с возможностью 
 * отслеживания и модификации значений регистров процессора
 */

#pragma once
#include "Hook.hpp"
#include <windows.h>
#include <cstdint>
#include <functional>
#include <mutex>
#include <shared_mutex>

/**
 * @enum Register
 * @brief Битовые флаги для отслеживаемых регистров процессора
 * @details Каждый бит представляет отдельный регистр. Флаги можно комбинировать через побитовое ИЛИ
 */
enum class Register : uint32_t {
    None = 0,        ///< Нет регистров для отслеживания
    EAX  = 1 << 0,   ///< Аккумулятор, используется для арифметических операций
    EBX  = 1 << 1,   ///< База, используется как указатель на данные
    ECX  = 1 << 2,   ///< Счетчик, используется в циклах
    EDX  = 1 << 3,   ///< Данные, используется для арифметики и ввода-вывода
    ESI  = 1 << 4,   ///< Индекс источника, используется для операций со строками
    EDI  = 1 << 5,   ///< Индекс назначения, используется для операций со строками
    EBP  = 1 << 6,   ///< Указатель базы кадра стека
    ESP  = 1 << 7    ///< Указатель стека
};

/**
 * @brief Оператор побитового ИЛИ для комбинирования флагов регистров
 * @param a Первый флаг
 * @param b Второй флаг
 * @return Скомбинированные флаги регистров
 */
inline Register operator|(Register a, Register b) {
    return static_cast<Register>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
    );
}

/**
 * @brief Оператор побитового И для проверки флагов регистров
 * @param a Первый флаг
 * @param b Второй флаг
 * @return Результат побитового И между флагами
 */
inline Register operator&(Register a, Register b) {
    return static_cast<Register>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
    );
}

/**
 * @struct Registers
 * @brief Структура для хранения значений регистров процессора
 * @details Содержит значения всех основных 32-битных регистров общего назначения
 */
struct Registers {
    uint32_t eax{0}; ///< Значение регистра EAX
    uint32_t ebx{0}; ///< Значение регистра EBX
    uint32_t ecx{0}; ///< Значение регистра ECX
    uint32_t edx{0}; ///< Значение регистра EDX
    uint32_t esi{0}; ///< Значение регистра ESI
    uint32_t edi{0}; ///< Значение регистра EDI
    uint32_t ebp{0}; ///< Значение регистра EBP
    uint32_t esp{0}; ///< Значение регистра ESP
};

/**
 * @typedef RegisterCallback
 * @brief Тип функции обратного вызова для обработки значений регистров
 * @details Вызывается при срабатывании хука с текущими значениями регистров
 */
using RegisterCallback = std::function<void(const Registers&)>;

/**
 * @class RegisterHook
 * @brief Класс для установки хуков на функции и отслеживания регистров
 * @details Позволяет перехватывать вызовы функций и отслеживать/модифицировать
 * значения регистров процессора в момент вызова
 */
class RegisterHook : public Hook {
public:
    /**
     * @brief Конструктор класса RegisterHook
     * @param targetFunction Указатель на целевую функцию для перехвата
     * @param registersToHook Флаги регистров, которые нужно отслеживать
     * @param callback Функция обратного вызова для обработки регистров
     */
    RegisterHook(void* targetFunction, Register registersToHook, RegisterCallback callback);
    
    /**
     * @brief Деструктор класса RegisterHook
     * @details Автоматически удаляет установленный хук при уничтожении объекта
     */
    ~RegisterHook();

    /**
     * @brief Устанавливает хук на целевую функцию
     * @return true если хук успешно установлен, false в противном случае
     */
    bool install() override;

    /**
     * @brief Удаляет установленный хук
     * @return true если хук успешно удален, false в противном случае
     */
    bool uninstall() override;

    /**
     * @brief Проверяет, установлен ли хук
     * @return true если хук установлен, false в противном случае
     */
    bool isInstalled() const override;

    /**
     * @brief Указатель на текущий экземпляр для доступа из хука
     */
    static RegisterHook* getInstance() {
        std::shared_lock<std::shared_mutex> lock(s_instanceMutex);
        return s_instance;
    }

protected:
    static RegisterHook* s_instance;
    static std::shared_mutex s_instanceMutex;

private:
    void* m_targetFunction{nullptr};  ///< Указатель на целевую функцию
    Register m_registers;             ///< Флаги отслеживаемых регистров
    RegisterCallback m_callback;      ///< Функция обратного вызова
    bool m_installed{false};          ///< Флаг установки хука
};