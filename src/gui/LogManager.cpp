/**
 * @file LogManager.cpp
 * @brief Реализация системы логирования MDBot
 */

#include "LogManager.hpp"
#include <QMutexLocker>
#include <QDateTime>
#include <QDebug>

/**
 * @brief Реализация паттерна Singleton для получения единственного экземпляра LogManager
 * @return Ссылка на глобальный экземпляр LogManager
 * 
 * Использует static переменную для гарантии единственности экземпляра
 * и его автоматической инициализации при первом обращении.
 */
LogManager& LogManager::instance() {
    static LogManager instance;
    return instance;
}

/**
 * @brief Конструктор LogManager
 * 
 * Инициализирует базовый класс QObject и устанавливает начальное состояние логирования.
 * По умолчанию логирование включено после создания объекта.
 */
LogManager::LogManager() : QObject(nullptr), m_loggingEnabled(false) {
    qDebug() << "LogManager constructor start";
    m_loggingEnabled = true;
    qDebug() << "LogManager constructor finished";
}

/**
 * @brief Основная функция логирования
 * @param level Уровень важности сообщения
 * @param message Текст сообщения
 * @param category Категория сообщения
 * @param botId Идентификатор бота
 * 
 * Проверяет, включено ли логирование глобально и для конкретной категории.
 * Если все проверки пройдены, создает структуру LogMessage и эмитит сигнал messageLogged.
 * Потокобезопасность обеспечивается через QMutexLocker.
 */
void LogManager::log(LogLevel level, const QString& message, const QString& category, const QString& botId) {
    if (!m_loggingEnabled) return;

    bool categoryEnabled = false;
    {
        QMutexLocker locker(&m_mutex);
        categoryEnabled = m_enabledCategories.value(category, false);
    }
    
    if (!categoryEnabled) return;

    LogMessage logMessage;
    logMessage.timestamp = QDateTime::currentDateTime();
    logMessage.level = level;
    logMessage.category = category;
    logMessage.botId = botId;
    logMessage.message = message;

    emit messageLogged(logMessage);
}

/**
 * @brief Вспомогательный метод для отправки отладочных сообщений
 * @param message Текст сообщения
 * @param category Категория сообщения
 * @param botId Идентификатор бота
 */
void LogManager::debug(const QString& message, const QString& category, const QString& botId) {
    log(LogLevel::Debug, message, category, botId);
}

/**
 * @brief Вспомогательный метод для отправки информационных сообщений
 * @param message Текст сообщения
 * @param category Категория сообщения
 * @param botId Идентификатор бота
 */
void LogManager::info(const QString& message, const QString& category, const QString& botId) {
    log(LogLevel::Info, message, category, botId);
}

/**
 * @brief Вспомогательный метод для отправки предупреждений
 * @param message Текст сообщения
 * @param category Категория сообщения
 * @param botId Идентификатор бота
 */
void LogManager::warning(const QString& message, const QString& category, const QString& botId) {
    log(LogLevel::Warning, message, category, botId);
}

/**
 * @brief Вспомогательный метод для отправки сообщений об ошибках
 * @param message Текст сообщения
 * @param category Категория сообщения
 * @param botId Идентификатор бота
 */
void LogManager::error(const QString& message, const QString& category, const QString& botId) {
    log(LogLevel::Error, message, category, botId);
}

/**
 * @brief Управление состоянием категории логирования
 * @param category Категория для настройки
 * @param enabled Новое состояние категории
 * 
 * Потокобезопасно устанавливает состояние категории и эмитит сигнал
 * об изменении настроек категорий.
 */
void LogManager::setCategoryEnabled(const QString& category, bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_enabledCategories[category] = enabled;
    emit categoriesChanged();
}

/**
 * @brief Проверка состояния категории логирования
 * @param category Категория для проверки
 * @return Состояние категории (true - включена, false - выключена)
 * 
 * Потокобезопасно возвращает текущее состояние указанной категории.
 */
bool LogManager::isCategoryEnabled(const QString& category) const {
    QMutexLocker locker(&m_mutex);
    return m_enabledCategories.value(category, false);
}

/**
 * @brief Управление глобальным состоянием логирования
 * @param enabled Новое состояние (true - включить, false - выключить)
 * 
 * Потокобезопасно устанавливает глобальное состояние логирования и
 * эмитит сигнал об изменении состояния.
 */
void LogManager::setLoggingEnabled(bool enabled) {
    QMutexLocker locker(&m_mutex);
    if (m_loggingEnabled != enabled) {
        m_loggingEnabled = enabled;
        emit loggingStateChanged(enabled);
    }
}

/**
 * @brief Получение текущего глобального состояния логирования
 * @return true если логирование включено, false если выключено
 * 
 * Потокобезопасно возвращает текущее глобальное состояние логирования.
 */
bool LogManager::isLoggingEnabled() const {
    QMutexLocker locker(&m_mutex);
    return m_loggingEnabled;
}