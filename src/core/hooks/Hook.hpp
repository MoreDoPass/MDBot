#pragma once
#include <windows.h>
#include <cstdint>

/**
 * @struct HookData
 * @brief Структура данных для передачи в удаленный поток
 * @details Содержит все необходимые данные для установки хука в удаленном процессе
 */
#pragma pack(push, 1)
struct HookData {
    void* targetAddress;     ///< Адрес где нужно установить хук
    void* hookFunction;      ///< Адрес функции-перехватчика
    BYTE originalBytes[5];   ///< Оригинальные байты для сохранения
    BYTE hookBytes[5];       ///< Байты для установки хука
};
#pragma pack(pop)

class Hook {
public:
    virtual ~Hook() = default;
    
    virtual bool install() = 0;
    virtual bool uninstall() = 0;
    virtual bool isInstalled() const = 0;

protected:
    BYTE m_originalBytes[5];  ///< Для хранения оригинальных байтов
    void* m_hookAddress;      ///< Адрес где установлен хук
    bool m_installed;         ///< Статус установки
    
    /**
     * @brief Функция для установки хука через CreateRemoteThread
     * @param processHandle Handle процесса
     * @param targetAddress Адрес для установки хука
     * @param hookFunction Адрес функции-перехватчика
     * @return true если хук успешно установлен
     */
    static bool __cdecl InstallHookRemote(HANDLE processHandle, void* targetAddress, void* hookFunction);
    
    /**
     * @brief Функция для удаления хука через CreateRemoteThread
     * @param processHandle Handle процесса
     * @param targetAddress Адрес установленного хука
     * @param originalBytes Оригинальные байты для восстановления
     * @return true если хук успешно удален
     */
    static bool __cdecl UninstallHookRemote(HANDLE processHandle, void* targetAddress, const BYTE* originalBytes);
};