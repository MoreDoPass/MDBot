/**
 * @file LogManager.hpp
 * @brief Централизованная система логирования для MDBot
 * 
 * Этот файл содержит реализацию системы логирования, которая обеспечивает:
 * - Многоуровневое логирование (Debug, Info, Warning, Error)
 * - Категоризацию сообщений
 * - Поддержку идентификаторов ботов
 * - Потокобезопасность через QMutex
 */
#pragma once
#include <QObject>
#include <QString>
#include <QMap>
#include <QDateTime>
#include <QMutex>
#include <QSettings>

/**
 * @enum LogLevel
 * @brief Уровни важности сообщений лога
 */
enum class LogLevel {
    Debug,   ///< Отладочные сообщения для разработчиков
    Info,    ///< Информационные сообщения о нормальной работе
    Warning, ///< Предупреждения о потенциальных проблемах
    Error    ///< Критические ошибки, требующие внимания
};

/**
 * @struct LogMessage
 * @brief Структура, представляющая одно сообщение лога
 */
struct LogMessage {
    QDateTime timestamp; ///< Временная метка создания сообщения
    LogLevel level;     ///< Уровень важности сообщения
    QString category;   ///< Категория сообщения (например, "Network", "Database")
    QString botId;      ///< Идентификатор бота, если сообщение связано с конкретным ботом
    QString message;    ///< Текст сообщения
};

/**
 * @class LogManager
 * @brief Синглтон-менеджер системы логирования
 * 
 * LogManager реализует паттерн Singleton и предоставляет централизованный 
 * интерфейс для логирования в приложении. Класс потокобезопасен и позволяет
 * управлять категориями логов и общим состоянием логирования.
 */
class LogManager : public QObject {
    Q_OBJECT

public:
    // Стандартные категории логов
    static inline const QString CATEGORY_SYSTEM = "System";        ///< Системные сообщения
    static inline const QString CATEGORY_MEMORY = "Memory";        ///< Операции с памятью
    static inline const QString CATEGORY_CORE = "Core";           ///< Ядро бота
    static inline const QString CATEGORY_HOOKS = "Hooks";         ///< Хуки и инъекции
    static inline const QString CATEGORY_UI = "UI";               ///< Пользовательский интерфейс
    static inline const QString CATEGORY_COMBAT = "Combat";       ///< Боевая система
    static inline const QString CATEGORY_CHARACTER = "Character"; ///< Информация о персонаже
    static inline const QString CATEGORY_NETWORK = "Network";     ///< Сетевые операции
    static inline const QString CATEGORY_MOVEMENT = "Movement";   ///< Перемещение персонажа
    static inline const QString CATEGORY_WINDOW = "Window";       ///< Операции с окнами
    static inline const QString CATEGORY_CONFIG = "Config";       ///< Настройки и конфигурация

    /**
     * @brief Получить единственный экземпляр LogManager
     * @return Ссылка на экземпляр LogManager
     */
    static LogManager& instance();

    /**
     * @brief Основной метод логирования
     * @param level Уровень важности сообщения
     * @param message Текст сообщения
     * @param category Категория сообщения (по умолчанию "System")
     * @param botId Идентификатор бота (опционально)
     */
    void log(LogLevel level, const QString& message, const QString& category = "System", const QString& botId = QString());

    /**
     * @brief Логирование отладочного сообщения
     * @param message Текст сообщения
     * @param category Категория сообщения (по умолчанию "System")
     * @param botId Идентификатор бота (опционально)
     */
    void debug(const QString& message, const QString& category = "System", const QString& botId = QString());

    /**
     * @brief Логирование информационного сообщения
     * @param message Текст сообщения
     * @param category Категория сообщения (по умолчанию "System")
     * @param botId Идентификатор бота (опционально)
     */
    void info(const QString& message, const QString& category = "System", const QString& botId = QString());

    /**
     * @brief Логирование предупреждения
     * @param message Текст сообщения
     * @param category Категория сообщения (по умолчанию "System")
     * @param botId Идентификатор бота (опционально)
     */
    void warning(const QString& message, const QString& category = "System", const QString& botId = QString());

    /**
     * @brief Логирование ошибки
     * @param message Текст сообщения
     * @param category Категория сообщения (по умолчанию "System")
     * @param botId Идентификатор бота (опционально)
     */
    void error(const QString& message, const QString& category = "System", const QString& botId = QString());

    /**
     * @brief Включить или выключить логирование для определенной категории
     * @param category Имя категории
     * @param enabled true для включения, false для выключения
     */
    void setCategoryEnabled(const QString& category, bool enabled);

    /**
     * @brief Проверить, включено ли логирование для категории
     * @param category Имя категории
     * @return true если категория включена, иначе false
     */
    bool isCategoryEnabled(const QString& category) const;

    /**
     * @brief Включить или выключить логирование глобально
     * @param enabled true для включения, false для выключения
     */
    void setLoggingEnabled(bool enabled);

    /**
     * @brief Проверить, включено ли логирование глобально
     * @return true если логирование включено, иначе false
     */
    bool isLoggingEnabled() const;

    /**
     * @brief Получить список всех стандартных категорий
     * @return QStringList со всеми стандартными категориями
     */
    static QStringList getDefaultCategories() {
        return {
            CATEGORY_SYSTEM,
            CATEGORY_MEMORY,
            CATEGORY_CORE,
            CATEGORY_HOOKS,
            CATEGORY_UI,
            CATEGORY_COMBAT,
            CATEGORY_CHARACTER,
            CATEGORY_NETWORK,
            CATEGORY_MOVEMENT,
            CATEGORY_WINDOW,
            CATEGORY_CONFIG
        };
    }

signals:
    /**
     * @brief Сигнал, испускаемый при добавлении нового сообщения в лог
     * @param message Структура с информацией о сообщении
     */
    void messageLogged(const LogMessage& message);

    /**
     * @brief Сигнал об изменении состояния логирования
     * @param enabled Новое состояние (true - включено, false - выключено)
     */
    void loggingStateChanged(bool enabled);

    /**
     * @brief Сигнал об изменении настроек категорий
     */
    void categoriesChanged();

private:
    /** @brief Приватный конструктор (паттерн Singleton) */
    LogManager();
    
    /** @brief Запрет копирования */
    LogManager(const LogManager&) = delete;
    
    /** @brief Запрет присваивания */
    LogManager& operator=(const LogManager&) = delete;

    mutable QMutex m_mutex;            ///< Мьютекс для потокобезопасности
    QMap<QString, bool> m_enabledCategories; ///< Карта включенных/выключенных категорий
    bool m_loggingEnabled;             ///< Флаг глобального состояния логирования
};