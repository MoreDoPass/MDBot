/**
 * @file MemoryManager.hpp
 * @brief Менеджер для работы с памятью процесса WoW
 * @details Предоставляет интерфейс для безопасной работы с памятью целевого процесса
 */

#pragma once
#include <windows.h>
#include <TlHelp32.h>

#include <memory>
#include <string>
#include <vector>

#include <stdexcept>


/**
 * @class MemoryManager
 * @brief Класс для управления памятью процесса WoW
 * @details Обеспечивает безопасный доступ к памятью процесса WoW, включая чтение и запись
 * различных типов данных, работу со строками и массивами, а также поиск паттернов в памяти.
 * Каждый экземпляр класса работает с отдельным процессом WoW.
 */
class MemoryManager
{
  public:
    /**
     * @brief Конструктор менеджера памяти
     * @param processId ID процесса WoW
     * @throw std::runtime_error если не удалось получить handle процесса
     */
    explicit MemoryManager(DWORD processId);

    /**
     * @brief Деструктор
     * @details Освобождает handle процесса
     */
    ~MemoryManager();

    // Запрещаем копирование и присваивание
    MemoryManager(const MemoryManager&)            = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    // Разрешаем перемещение
    MemoryManager(MemoryManager&&) noexcept;
    MemoryManager& operator=(MemoryManager&&) noexcept;

    /**
     * @brief Получает handle процесса
     * @return Handle процесса WoW
     */
    HANDLE GetProcessHandle() const { return processHandle; }

    /**
     * @brief Получает ID процесса
     * @return ID процесса WoW
     */
    DWORD GetProcessId() const { return processId; }

    /**
     * @brief Получает базовый адрес модуля
     * @param moduleName Имя модуля (по умолчанию "run.exe")
     * @return Базовый адрес модуля или 0 в случае ошибки
     */
    uintptr_t GetModuleBaseAddress(const wchar_t* moduleName = L"run.exe");

    /**
     * @brief Преобразует относительный адрес в абсолютный
     * @param relativeAddress Относительный адрес от базового адреса модуля
     * @return Абсолютный адрес в памяти процесса
     */
    uintptr_t ResolveAddress(uintptr_t relativeAddress);

    /**
     * @brief Читает значение из памяти процесса
     * @tparam T Тип читаемого значения
     * @param address Абсолютный адрес для чтения
     * @return Прочитанное значение
     * @throw std::runtime_error в случае ошибки чтения
     */
    template <typename T>
    T Read(uintptr_t address);

    /**
     * @brief Записывает значение в память процесса
     * @tparam T Тип записываемого значения
     * @param address Абсолютный адрес для записи
     * @param value Значение для записи
     * @return true если запись успешна
     */
    template <typename T>
    bool Write(uintptr_t address, const T& value);

    /**
     * @brief Читает значение по относительному адресу
     * @tparam T Тип читаемого значения
     * @param relativeAddress Относительный адрес от базы модуля
     * @return Прочитанное значение
     */
    template <typename T>
    T ReadRelative(uintptr_t relativeAddress)
    {
        return Read<T>(ResolveAddress(relativeAddress));
    }

    /**
     * @brief Записывает значение по относительному адресу
     * @tparam T Тип записываемого значения
     * @param relativeAddress Относительный адрес от базы модуля
     * @param value Значение для записи
     * @return true если запись успешна
     */
    template <typename T>
    bool WriteRelative(uintptr_t relativeAddress, const T& value)
    {
        return Write<T>(ResolveAddress(relativeAddress), value);
    }

    /**
     * @brief Читает строку из памяти процесса
     * @param address Адрес строки (абсолютный или относительный)
     * @param maxLength Максимальная длина строки (по умолчанию 12 для WoW)
     * @param isRelative Если true, адрес считается относительным от базового адреса модуля
     * @return Прочитанная строка
     */
    std::string ReadString(uintptr_t address, size_t maxLength = 12, bool isRelative = false);

    /**
     * @brief Записывает строку в память процесса
     * @param address Адрес для записи (абсолютный или относительный)
     * @param str Строка для записи
     * @param isRelative Если true, адрес считается относительным от базового адреса модуля
     * @return true если запись успешна
     */
    bool WriteString(uintptr_t address, const std::string& str, bool isRelative = false);

    /**
     * @brief Читает массив значений из памяти
     * @tparam T Тип элементов массива
     * @param address Абсолютный адрес начала массива
     * @param count Количество элементов для чтения
     * @return Вектор прочитанных значений
     */
    template <typename T>
    std::vector<T> ReadArray(uintptr_t address, size_t count);

    /**
     * @brief Записывает массив значений в память
     * @tparam T Тип элементов массива
     * @param address Абсолютный адрес для записи
     * @param array Вектор значений для записи
     * @return true если запись успешна
     */
    template <typename T>
    bool WriteArray(uintptr_t address, const std::vector<T>& array);

    /**
     * @brief Ищет паттерн в памяти процесса
     * @param pattern Байтовый паттерн для поиска
     * @param mask Маска паттерна (x - проверять байт, ? - пропустить)
     * @return Адрес найденного паттерна или 0
     */
    uintptr_t FindPattern(const char* pattern, const char* mask);

    /**
     * @brief Проверяет валидность адреса
     * @param address Проверяемый адрес
     * @return true если адрес валиден
     */
    bool IsValidAddress(uintptr_t address) const;

    /**
     * @brief Получает права доступа к региону памяти
     * @param address Адрес региона памяти
     * @return Права доступа (флаги PAGE_*)
     */
    DWORD GetMemoryProtection(uintptr_t address) const;

    /**
     * @brief Изменяет права доступа к региону памяти
     * @param address Адрес региона памяти
     * @param size Размер региона
     * @param protection Новые права доступа
     * @return true если изменение успешно
     */
    bool SetMemoryProtection(uintptr_t address, size_t size, DWORD protection);

    /**
     * @brief Проверяет валидность строки в памяти
     * @param str Строка для проверки
     * @return true если строка валидна (содержит только допустимые символы)
     */
    static bool IsValidCharacterName(const std::string& str)
    {
        if (str.empty() || str.length() > 12)
        {
            return false;
        }

        // Проверяем допустимые символы (буквы, цифры и некоторые специальные символы)
        for (char c : str)
        {
            if (!isalnum(c) && c != '_' && c != '-')
            {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Выделяет память в процессе
     * @param address Предпочтительный адрес для выделения (может быть nullptr)
     * @param size Размер выделяемой памяти в байтах
     * @param protection Права доступа для выделенной памяти (по умолчанию PAGE_EXECUTE_READWRITE)
     * @return Указатель на выделенную память или nullptr в случае ошибки
     */
    void* AllocateMemory(void* address = nullptr, size_t size = 0x1000, DWORD protection = PAGE_EXECUTE_READWRITE);

    /**
     * @brief Освобождает ранее выделенную память
     * @param address Адрес для освобождения
     * @return true если память успешно освобождена
     */
    bool FreeMemory(void* address);

    /**
     * @brief Базовый метод для чтения памяти процесса
     * @param address Адрес для чтения
     * @param buffer Буфер для записи данных
     * @param size Размер читаемых данных
     * @return true если чтение успешно
     */
    bool ReadMemory(uintptr_t address, void* buffer, size_t size);

    /**
     * @brief Базовый метод для записи в память процесса
     * @param address Адрес для записи
     * @param buffer Буфер с данными для записи
     * @param size Размер записываемых данных
     * @return true если запись успешна
     */
    bool WriteMemory(uintptr_t address, const void* buffer, size_t size);

  private:
    HANDLE    processHandle; ///< Handle процесса WoW
    DWORD     processId;     ///< ID процесса WoW
    uintptr_t baseAddress;   ///< Базовый адрес run.exe

    /**
     * @brief Проверяет и при необходимости изменяет права доступа к памяти
     * @param address Адрес памяти
     * @param size Размер региона
     * @param requiredAccess Требуемые права доступа
     * @return true если доступ обеспечен
     */
    bool EnsureMemoryAccess(uintptr_t address, size_t size, DWORD requiredAccess);

    /**
     * @brief Выбрасывает исключение с информацией об ошибке Windows
     * @param message Сообщение об ошибке
     * @throw std::runtime_error
     */
    void ThrowLastError(const char* message) const;

    /**
     * @brief Обновляет базовый адрес модуля
     */
    void UpdateBaseAddress();
};

// Реализация шаблонных методов
template <typename T>
T MemoryManager::Read(uintptr_t address)
{
    T      value;
    SIZE_T bytesRead;

    if (!ReadProcessMemory(processHandle, (LPCVOID)address, &value, sizeof(T), &bytesRead) || bytesRead != sizeof(T))
    {
        ThrowLastError("Failed to read memory");
    }

    return value;
}

template <typename T>
bool MemoryManager::Write(uintptr_t address, const T& value)
{
    SIZE_T bytesWritten;
    return WriteProcessMemory(processHandle, (LPVOID)address, &value, sizeof(T), &bytesWritten)
           && bytesWritten == sizeof(T);
}

template <typename T>
std::vector<T> MemoryManager::ReadArray(uintptr_t address, size_t count)
{
    std::vector<T> result(count);
    SIZE_T         bytesRead;

    if (!ReadProcessMemory(processHandle, (LPCVOID)address, result.data(), count * sizeof(T), &bytesRead)
        || bytesRead != count * sizeof(T))
    {
        ThrowLastError("Failed to read array");
    }

    return result;
}

template <typename T>
bool MemoryManager::WriteArray(uintptr_t address, const std::vector<T>& array)
{
    SIZE_T bytesWritten;
    return WriteProcessMemory(processHandle, (LPVOID)address, array.data(), array.size() * sizeof(T), &bytesWritten)
           && bytesWritten == array.size() * sizeof(T);
}