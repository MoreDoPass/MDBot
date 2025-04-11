#include "MainWindow.hpp"
#include "bot/process/ProcessListDialog.hpp"
#include "bot/ui/BotTabWidget.hpp"  // Добавляем include
#include "LogWindow.hpp"
#include "LogManager.hpp"
#include <QMessageBox>
#include <QDateTime>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_logWindow(nullptr)
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
    // Создаем меню "Окно"
    windowMenu = menuBar()->addMenu("Окно");
    
    // Добавляем действие для работы с процессами
    addWindowAction = new QAction("Добавить окно", this);
    connect(addWindowAction, &QAction::triggered, this, &MainWindow::showProcessListDialog);
    windowMenu->addAction(addWindowAction);
    
    // Добавляем действие для показа окна логов
    showLogWindowAction = new QAction("Показать логи", this);
    connect(showLogWindowAction, &QAction::triggered, this, &MainWindow::showLogWindow);
    windowMenu->addAction(showLogWindowAction);
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
    std::cout << "Attempting to create/show log window..." << std::endl;
    
    if (!m_logWindow) {
        std::cout << "Creating new log window..." << std::endl;
        m_logWindow = new LogWindow(this); // Создаём как дочернее окно
        std::cout << "Log window created." << std::endl;
    }
    
    std::cout << "Showing log window..." << std::endl;
    m_logWindow->show();
    m_logWindow->raise();
    m_logWindow->activateWindow();
    std::cout << "Log window should be visible now." << std::endl;
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