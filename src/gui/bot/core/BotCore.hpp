#pragma once
#include <windows.h>
#include <QString>
#include <QObject>
#include "core/hooks/RegisterHook.hpp"
#include "core/memory/MemoryManager.hpp"
#include "character/CharacterData.hpp"

/**
 * @brief Структура контекста для каждого окна WoW
 * @details Содержит основную информацию о процессе игры и состоянии персонажа:
 * - Идентификатор процесса
 * - Хэндл окна
 * - Текущие данные персонажа
 */
struct BotContext {
    DWORD processId{0};       ///< ID процесса WoW
    HWND windowHandle{0};     ///< Handle окна WoW
    CharacterData character;   ///< Данные персонажа в текущем окне
};

/**
 * @brief Основной класс управления ботом
 * @details Отвечает за:
 * - Инициализацию и управление состоянием бота
 * - Работу с памятью процесса WoW
 * - Установку и обработку хуков
 * - Обновление данных контекста
 */
class BotCore : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Конструктор ядра бота
     * @param processId ID процесса WoW
     * @param parent Родительский QObject
     */
    explicit BotCore(DWORD processId, QObject* parent = nullptr);
    
    /**
     * @brief Деструктор
     * @details Отключает бота и освобождает ресурсы
     */
    ~BotCore();

    /**
     * @brief Инициализация ядра бота
     * @return true если инициализация прошла успешно
     * @details Выполняет:
     * - Поиск окна процесса
     * - Установку хуков
     * - Подготовку менеджера памяти
     */
    bool initialize();
    
    /**
     * @brief Проверка статуса инициализации
     * @return true если бот инициализирован
     */
    bool isInitialized() const { return m_initialized; }
    
    /**
     * @brief Получение ID процесса
     * @return ID процесса WoW
     */
    DWORD getProcessId() const { return m_context.processId; }
    
    /**
     * @brief Проверка активности бота
     * @return true если бот активен
     */
    bool isEnabled() const { return m_enabled; }
    
    /**
     * @brief Получение текущего контекста
     * @return Константная ссылка на контекст
     */
    const BotContext& context() const { return m_context; }
    
    /**
     * @brief Обработчик обновления регистров
     * @param regs Структура с значениями регистров
     * @details Вызывается хуком при обновлении регистров процесса
     */
    void onRegistersUpdated(const Registers& regs);

public slots:
    /**
     * @brief Включение бота
     * @details Активирует хуки и начинает обработку данных
     */
    void enable();
    
    /**
     * @brief Выключение бота
     * @details Деактивирует хуки и останавливает обработку
     */
    void disable();

signals:
    /**
     * @brief Сигнал изменения состояния
     * @param enabled Новое состояние бота
     */
    void stateChanged(bool enabled);
    
    /**
     * @brief Сигнал обновления контекста
     * @details Испускается при изменении данных в контексте
     */
    void contextUpdated();

private:
    /**
     * @brief Поиск хэндла окна процесса
     * @return true если окно найдено
     */
    bool findWindowHandle();
    
    /**
     * @brief Установка хуков
     * @return true если хуки установлены успешно
     */
    bool setupHooks();

private:
    BotContext m_context;                          ///< Контекст бота
    bool m_initialized{false};                     ///< Флаг инициализации
    bool m_enabled{false};                         ///< Флаг активности
    std::shared_ptr<MemoryManager> m_memory;       ///< Менеджер памяти
    std::unique_ptr<RegisterHook> m_registerHook;  ///< Хук регистров
};