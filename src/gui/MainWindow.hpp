/**
 * @file MainWindow.hpp
 * @brief Главное окно приложения MDBot
 */

#pragma once
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QList>
#include <QTabWidget>
#include <windows.h>
#include "log/LogWindow.hpp"
#include "debug/DebugWindow.hpp"

/**
 * @class MainWindow
 * @brief Основной класс главного окна приложения
 * @details Отвечает за основной интерфейс приложения, управление вкладками процессов и окнами
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Конструктор главного окна
     * @param parent Родительский виджет (nullptr по умолчанию)
     */
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    /**
     * @brief Показывает диалог выбора процесса WoW
     */
    void showProcessListDialog();

    /**
     * @brief Обработчик выбора процесса
     * @param processId ID выбранного процесса
     */
    void onProcessSelected(DWORD processId);

    /**
     * @brief Показывает окно логов
     */
    void showLogWindow();

    /**
     * @brief Показывает окно отладки
     */
    void showDebugWindow();

    /**
     * @brief Обработчик закрытия вкладки
     * @param index Индекс закрываемой вкладки
     */
    void onTabCloseRequested(int index);

private:
    /**
     * @brief Инициализация меню окна
     */
    void setupMenus();

    /**
     * @brief Инициализация центрального виджета с вкладками
     */
    void setupCentralWidget();
    
    QMenu* windowMenu;                  ///< Меню "Окно"
    QAction* addWindowAction;           ///< Действие для добавления нового процесса
    QAction* showLogWindowAction;       ///< Действие для показа окна логов
    QAction* showDebugWindowAction;     ///< Действие для показа окна отладки
    
    LogWindow* m_logWindow;             ///< Указатель на окно логов
    DebugWindow* m_debugWindow;         ///< Указатель на окно отладки
    QTabWidget* m_tabWidget;            ///< Виджет с вкладками для процессов
    QList<DWORD> attachedProcesses;     ///< Список подключенных процессов
};