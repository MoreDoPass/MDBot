#include "MainWindow.hpp"
#include "bot/process/ProcessListDialog.hpp"
#include "bot/ui/BotTabWidget.hpp"
#include "log/LogWindow.hpp"
#include "debug/DebugWindow.hpp"
#include "LogManager.hpp"
#include <QMessageBox>
#include <QDateTime>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_logWindow(nullptr)
    , m_debugWindow(nullptr)
{
    setWindowTitle("MDBot");
    resize(800, 600);
    
    setupCentralWidget();
    setupMenus();
}

void MainWindow::setupCentralWidget()
{
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabPosition(QTabWidget::North);
    m_tabWidget->setMovable(true);
    m_tabWidget->setTabsClosable(true);
    
    // Подключаем сигнал закрытия вкладки
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
    
    setCentralWidget(m_tabWidget);
}

void MainWindow::setupMenus()
{
    // Меню "Окно"
    windowMenu = menuBar()->addMenu("Окно");
    
    // Действие для работы с процессами
    addWindowAction = new QAction("Добавить окно", this);
    connect(addWindowAction, &QAction::triggered, this, &MainWindow::showProcessListDialog);
    windowMenu->addAction(addWindowAction);
    
    // Добавляем действие для показа окна логов
    showLogWindowAction = new QAction("Показать логи", this);
    connect(showLogWindowAction, &QAction::triggered, this, &MainWindow::showLogWindow);
    windowMenu->addAction(showLogWindowAction);

    // Добавляем действие для показа окна отладки
    showDebugWindowAction = new QAction("Показать отладку", this);
    connect(showDebugWindowAction, &QAction::triggered, this, &MainWindow::showDebugWindow);
    windowMenu->addAction(showDebugWindowAction);
}

void MainWindow::showProcessListDialog()
{
    ProcessListDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        DWORD processId = dialog.getSelectedProcessId();
        onProcessSelected(processId);
    }
}

void MainWindow::onProcessSelected(DWORD processId)
{
    try {
        // Создаем вкладку с ботом для процесса
        QString tabTitle = QString("Process %1").arg(processId);
        BotTabWidget* botTab = new BotTabWidget(processId, m_tabWidget);
        
        int index = m_tabWidget->addTab(botTab, tabTitle);
        m_tabWidget->setCurrentIndex(index);
        
        attachedProcesses.append(processId);
        LogManager::instance().info(
            QString("Процесс (PID: %1) успешно добавлен").arg(processId),
            "MainWindow"
        );
        QMessageBox::information(this, "Информация", 
            QString("Процесс (PID: %1) успешно добавлен").arg(processId));
    }
    catch (const std::exception& e) {
        LogManager::instance().error(
            QString("Ошибка при подключении к процессу %1: %2")
                .arg(processId)
                .arg(e.what()),
            "MainWindow"
        );
        QMessageBox::critical(this, "Ошибка",
            QString("Не удалось подключиться к процессу %1:\n%2")
                .arg(processId)
                .arg(e.what()));
    }
}

void MainWindow::showLogWindow()
{
    if (!m_logWindow) {
        m_logWindow = new LogWindow(nullptr); // Создаем независимое окно
    }
    
    m_logWindow->show();
    m_logWindow->raise();
    m_logWindow->activateWindow();
}

void MainWindow::showDebugWindow()
{
    DWORD selectedProcessId = 0;
    // Берем ID первого подключенного процесса
    if (!attachedProcesses.isEmpty()) {
        selectedProcessId = attachedProcesses.first();
    }

    if (selectedProcessId == 0) {
        QMessageBox::warning(this, "Ошибка", "Сначала подключитесь к процессу WoW");
        return;
    }

    if (!m_debugWindow) {
        m_debugWindow = new DebugWindow(selectedProcessId);
    }
    
    m_debugWindow->show();
    m_debugWindow->raise();
    m_debugWindow->activateWindow();
}

void MainWindow::onTabCloseRequested(int index)
{
    if (index < 0 || index >= m_tabWidget->count())
        return;
        
    // Получаем виджет вкладки и PID процесса
    QWidget* tab = m_tabWidget->widget(index);
    DWORD processId = attachedProcesses[index];
    
    // Удаляем вкладку и процесс из списка
    m_tabWidget->removeTab(index);
    attachedProcesses.removeAt(index);
    delete tab;
    
    QMessageBox::information(this, "Информация",
        QString("Процесс (PID: %1) отключен").arg(processId));
}