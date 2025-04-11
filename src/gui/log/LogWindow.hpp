/**
 * @file LogWindow.hpp
 * @brief Графический интерфейс для отображения и управления логами MDBot
 * 
 * Окно логирования предоставляет следующие возможности:
 * - Фильтрация логов по категориям через древовидный список
 * - Отображение логов в текстовом виде с временными метками
 * - Управление включением/выключением логирования
 * - Экспорт и очистка логов
 * - Поиск по логам
 */

#pragma once

#include "LogManager.hpp"
#include <QMainWindow>
#include <QTreeWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QSet>
#include <QCheckBox>
#include <QLineEdit>

/**
 * @class LogWindow
 * @brief Главное окно системы логирования
 * 
 * LogWindow отвечает за визуализацию логов и предоставление пользовательского
 * интерфейса для управления системой логирования. Окно разделено на две основные части:
 * - Левая панель с деревом фильтров по категориям
 * - Основная область с текстом логов
 */
class LogWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Конструктор окна логирования
     * @param parent Родительский виджет
     */
    explicit LogWindow(QWidget* parent = nullptr);

    /**
     * @brief Деструктор окна логирования
     */
    ~LogWindow() = default;

private slots:
    /**
     * @brief Обработчик изменения состояния логирования
     * @param enabled Новое состояние логирования
     */
    void onLoggingStateChanged(bool enabled);

    /**
     * @brief Обработчик нового сообщения в логе
     * @param message Структура с данными сообщения
     */
    void onMessageLogged(const LogMessage& message);

    /**
     * @brief Обработчик изменения настроек категорий
     */
    void onCategoriesChanged();

private slots:
    /**
     * @brief Обработчик выбора элемента в дереве фильтров
     * @param item Выбранный элемент
     * @param column Номер колонки
     */
    void filterByTreeItem(QTreeWidgetItem* item, int column);

    /**
     * @brief Включение/выключение логирования
     * @param enabled Новое состояние
     */
    void toggleLogging(bool enabled);

    /**
     * @brief Экспорт логов в файл
     */
    void exportLogs();

    /**
     * @brief Очистка всех логов
     */
    void clearLogs();

    /**
     * @brief Поиск по логам
     */
    void findInLogs();

    /**
     * @brief Сброс всех категорий к состоянию по умолчанию
     */
    void resetCategories();

private:
    /**
     * @brief Создание базового интерфейса окна
     */
    void createBasicUI();

    /**
     * @brief Создание панели инструментов
     */
    void initializeToolBar();

    /**
     * @brief Создание дерева категорий
     */
    void initializeFilterTree();

    /**
     * @brief Инициализация менеджера логирования
     */
    void initializeManager();

    /**
     * @brief Установка всех необходимых соединений сигнал-слот
     */
    void initializeConnections();

    /**
     * @brief Проверяет, нужно ли показывать сообщение
     * @param message Проверяемое сообщение
     * @return true если сообщение должно быть показано
     */
    bool shouldShowMessage(const LogMessage& message) const;

    /**
     * @brief Форматирование сообщения лога
     * @param message Структура сообщения для форматирования
     * @return Отформатированная строка для отображения
     */
    QString formatLogMessage(const LogMessage& message) const;

    /**
     * @brief Обновление элементов дерева категорий
     */
    void updateCategoryTreeItems();

    // UI элементы
    QToolBar* m_toolBar;                ///< Панель инструментов
    QTextEdit* m_logView;               ///< Область отображения логов
    QTreeWidget* m_filterTree;          ///< Дерево фильтров
    QTreeWidgetItem* m_categoriesRoot;  ///< Корневой элемент дерева категорий
    QCheckBox* m_toggleLoggingAction;   ///< Действие включения/выключения логов
    QLineEdit* m_searchField;           ///< Поле для поиска по логам

    // Состояние
    QSet<QString> m_enabledCategories;   ///< Множество включенных категорий
    QSet<QString> m_enabledBots;         ///< Множество включенных ботов
    bool m_isInitialized;                ///< Флаг инициализации окна
    bool m_isInitializing;               ///< Флаг процесса инициализации
};