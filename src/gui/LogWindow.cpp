/**
 * @file LogWindow.cpp
 * @brief Реализация графического интерфейса логирования MDBot
 */

#include "LogWindow.hpp"
#include <QVBoxLayout>
#include <QSplitter>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QAction>

LogWindow::LogWindow(QWidget* parent)
    : QMainWindow(parent),
      m_toolBar(nullptr),
      m_logView(nullptr),
      m_filterTree(nullptr),
      m_categoriesRoot(nullptr),
      m_toggleLoggingAction(nullptr),
      m_searchField(nullptr),
      m_isInitialized(false),
      m_isInitializing(false) {
    // Создаем базовый UI
    createBasicUI();
    initializeToolBar();
    initializeFilterTree();
    
    // Инициализируем систему логирования
    initializeManager();
    initializeConnections();
    
    qDebug() << "LogWindow constructor finished. Initialized:" << m_isInitialized;
}

void LogWindow::createBasicUI() {
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* layout = new QVBoxLayout(centralWidget);
    auto* splitter = new QSplitter(Qt::Horizontal, centralWidget);

    m_filterTree = new QTreeWidget(splitter);
    m_filterTree->setHeaderLabel("Категории");

    m_logView = new QTextEdit(splitter);
    m_logView->setReadOnly(true);

    splitter->addWidget(m_filterTree);
    splitter->addWidget(m_logView);
    layout->addWidget(splitter);
}

void LogWindow::initializeToolBar() {
    m_toolBar = addToolBar("Инструменты");
    m_toolBar->setMovable(false);

    m_toggleLoggingAction = new QCheckBox("Включить логирование", this);
    m_toolBar->addWidget(m_toggleLoggingAction);
    connect(m_toggleLoggingAction, &QCheckBox::toggled, this, &LogWindow::toggleLogging);

    m_searchField = new QLineEdit(this);
    m_searchField->setPlaceholderText("Поиск...");
    m_toolBar->addWidget(m_searchField);
    connect(m_searchField, &QLineEdit::returnPressed, this, &LogWindow::findInLogs);
}

void LogWindow::initializeFilterTree() {
    m_categoriesRoot = new QTreeWidgetItem(m_filterTree);
    m_categoriesRoot->setText(0, "Категории");

    for (const QString& category : LogManager::getDefaultCategories()) {
        auto* item = new QTreeWidgetItem(m_categoriesRoot);
        item->setText(0, category);
        item->setCheckState(0, Qt::Checked);
    }

    m_filterTree->expandAll();
    connect(m_filterTree, &QTreeWidget::itemChanged, this, &LogWindow::filterByTreeItem);
}

/**
 * @brief Включение/выключение логирования
 * @param enabled Новое состояние
 * 
 * Передает команду включения/выключения логирования в LogManager.
 * Блокирует возможность редактирования категорий при включенном логировании.
 */
void LogWindow::toggleLogging(bool enabled) {
    if (!m_isInitialized && enabled) {
        qDebug() << "Cannot enable logging - not initialized";
        return;
    }
    
    try {
        QSignalBlocker treeBlocker(m_filterTree); // Блокируем сигналы от дерева
        
        // Визуально показываем состояние возможности редактирования категорий
        for(int i = 0; i < m_categoriesRoot->childCount(); ++i) {
            QTreeWidgetItem* item = m_categoriesRoot->child(i);
            item->setDisabled(enabled);
        }
        
        LogManager::instance().setLoggingEnabled(enabled);
        qDebug() << "Logging" << (enabled ? "enabled" : "disabled");
    } catch (const std::exception& e) {
        qDebug() << "Error toggling logging:" << e.what();
        m_toggleLoggingAction->setChecked(!enabled);
    }
}

void LogWindow::findInLogs() {
    auto searchField = qobject_cast<QLineEdit*>(sender());
    if (!searchField) return;
    
    QString searchText = searchField->text();
    if (searchText.isEmpty()) return;
    
    QTextCursor cursor = m_logView->textCursor();
    cursor.movePosition(QTextCursor::Start);
    m_logView->setTextCursor(cursor);
    
    if (!m_logView->find(searchText)) {
        cursor.movePosition(QTextCursor::Start);
        m_logView->setTextCursor(cursor);
        if (!m_logView->find(searchText)) {
            QMessageBox::information(this, "Поиск", "Текст не найден.");
        }
    }
}

/**
 * @brief Инициализация менеджера логирования
 * 
 * Устанавливает начальное состояние для всех стандартных категорий.
 */
void LogWindow::initializeManager() {
    if (m_isInitialized || m_isInitializing) {
        qDebug() << "LogManager initialization already in progress or completed";
        return;
    }

    m_isInitializing = true;
    qDebug() << "Starting LogManager initialization...";

    try {
        auto& logManager = LogManager::instance();
        
        // Включаем все стандартные категории по умолчанию
        for (const QString& category : LogManager::getDefaultCategories()) {
            m_enabledCategories.insert(category);
            logManager.setCategoryEnabled(category, true);
        }
        
        m_isInitialized = true;
        qDebug() << "LogManager initialized successfully";
    }
    catch (const std::exception& e) {
        qDebug() << "Error initializing LogManager:" << e.what();
        m_isInitialized = false;
    }

    m_isInitializing = false;
}

/**
 * @brief Установка соединений с сигналами LogManager
 * 
 * Подключает обработчики к сигналам LogManager для обновления UI
 * при изменении состояния логирования или появлении новых сообщений.
 */
void LogWindow::initializeConnections() {
    qDebug() << "initializeConnections start";
    auto& logManager = LogManager::instance();
    
    // Используем прямые соединения для критичных операций
    connect(&logManager, &LogManager::messageLogged,
            this, &LogWindow::onMessageLogged,
            Qt::QueuedConnection);
    
    connect(&logManager, &LogManager::loggingStateChanged,
            this, &LogWindow::onLoggingStateChanged,
            Qt::DirectConnection);
    
    connect(&logManager, &LogManager::categoriesChanged,
            this, &LogWindow::onCategoriesChanged,
            Qt::DirectConnection);
            
    qDebug() << "initializeConnections finished";
}

/**
 * @brief Обновление элементов дерева категорий
 * 
 * Синхронизирует состояние чекбоксов в дереве категорий с текущими
 * настройками включенных/выключенных категорий.
 */
void LogWindow::updateCategoryTreeItems() {
    QSignalBlocker treeBlocker(m_filterTree); // Блокируем сигналы на время обновления

    // Очищаем текущие элементы
    m_categoriesRoot->takeChildren();

    // Добавляем все стандартные категории
    for (const QString& category : LogManager::getDefaultCategories()) {
        auto* item = new QTreeWidgetItem(m_categoriesRoot);
        item->setText(0, category);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, m_enabledCategories.contains(category) ? Qt::Checked : Qt::Unchecked);
    }

    m_filterTree->expandAll();
}

/**
 * @brief Обработчик изменения настроек категорий
 * 
 * Обновляет UI после изменения настроек категорий в LogManager.
 */
void LogWindow::onCategoriesChanged() {
    updateCategoryTreeItems();
}

/**
 * @brief Сброс категорий к состоянию по умолчанию
 * 
 * Сбрасывает все настройки фильтрации категорий к начальному состоянию.
 */
void LogWindow::resetCategories() {
    auto& logManager = LogManager::instance();

    // Сначала отключаем все категории
    m_enabledCategories.clear();
    
    // Затем включаем все стандартные категории
    for (const QString& category : LogManager::getDefaultCategories()) {
        m_enabledCategories.insert(category);
        logManager.setCategoryEnabled(category, true);
    }

    updateCategoryTreeItems();
}

/**
 * @brief Обработчик изменения фильтра в дереве
 * @param item Измененный элемент
 * @param column Номер колонки
 * 
 * Обрабатывает изменение состояния чекбокса в дереве фильтров
 * и обновляет соответствующие настройки фильтрации.
 */
void LogWindow::filterByTreeItem(QTreeWidgetItem* item, int column) {
    if (m_isInitialized && LogManager::instance().isLoggingEnabled()) {
        QMessageBox::information(this, "Информация",
            "Для изменения категорий необходимо выключить логирование");
        // Восстанавливаем предыдущее состояние чекбокса
        QSignalBlocker blocker(m_filterTree);
        item->setCheckState(0, item->checkState(0) == Qt::Checked ? Qt::Unchecked : Qt::Checked);
        return;
    }

    if (!item->parent()) {
        bool checked = item->checkState(0) == Qt::Checked;
        QSignalBlocker blocker(m_filterTree);
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem* child = item->child(i);
            if (child->checkState(0) != (checked ? Qt::Checked : Qt::Unchecked)) {
                child->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
                QString childText = child->text(0);
                if (item->parent() == m_categoriesRoot) {
                    if (checked) {
                        m_enabledCategories.insert(childText);
                    } else {
                        m_enabledCategories.remove(childText);
                    }
                    LogManager::instance().setCategoryEnabled(childText, checked);
                }
            }
        }
        return;
    }
    
    QString itemText = item->text(0);
    bool enabled = item->checkState(0) == Qt::Checked;
    
    if (item->parent() == m_categoriesRoot) {
        try {
            qDebug() << "Changing category" << itemText << "to" << (enabled ? "enabled" : "disabled");
            
            if (enabled) {
                m_enabledCategories.insert(itemText);
            } else {
                m_enabledCategories.remove(itemText);
            }
            
            LogManager::instance().setCategoryEnabled(itemText, enabled);
            qDebug() << "Category" << itemText << "state changed successfully";
            
        } catch (const std::exception& e) {
            qDebug() << "Error changing category state:" << e.what();
            QSignalBlocker blocker(m_filterTree);
            item->setCheckState(0, !enabled ? Qt::Checked : Qt::Unchecked);
        }
    } else if (item->parent()->text(0) == "Боты") {
        if (enabled) {
            m_enabledBots.insert(itemText);
        } else {
            m_enabledBots.remove(itemText);
        }
    }
}

/**
 * @brief Обработчик нового сообщения в логе
 * @param message Полученное сообщение
 * 
 * Форматирует и добавляет новое сообщение в область логов, если оно
 * соответствует текущим настройкам фильтрации.
 */
void LogWindow::onMessageLogged(const LogMessage& message) {
    if (shouldShowMessage(message)) {
        m_logView->append(formatLogMessage(message));
    }
}

/**
 * @brief Проверка необходимости отображения сообщения
 * @param message Проверяемое сообщение
 * @return true если сообщение должно быть показано
 * 
 * Проверяет, соответствует ли сообщение текущим настройкам фильтрации
 * по категориям и ботам.
 */
bool LogWindow::shouldShowMessage(const LogMessage& message) const {
    // Проверяем категорию
    if (!m_enabledCategories.contains(message.category)) {
        return false;
    }
    // Проверяем бота (если сообщение от бота)
    if (!message.botId.isEmpty() && !m_enabledBots.contains(message.botId)) {
        return false;
    }
    return true;
}

/**
 * @brief Форматирование сообщения лога
 * @param message Структура сообщения для форматирования
 * @return Отформатированная строка для отображения
 * 
 * Форматирует сообщение лога, добавляя временную метку, уровень важности,
 * категорию и идентификатор бота (если есть).
 */
QString LogWindow::formatLogMessage(const LogMessage& message) const {
    QString botInfo = message.botId.isEmpty() ? "" : QString(" [Bot: %1]").arg(message.botId);
    return QString("[%1] [%2]%3 %4")
        .arg(message.timestamp.toString("hh:mm:ss.zzz"))
        .arg(message.category)
        .arg(botInfo)
        .arg(message.message);
}

/**
 * @brief Обработчик изменения состояния логирования
 * @param enabled Новое состояние (включено/выключено)
 * 
 * Обновляет состояние UI в соответствии с изменением глобального
 * состояния логирования.
 */
void LogWindow::onLoggingStateChanged(bool enabled) {
    // В этой версии просто обновляем отображение
}

/**
 * @brief Экспорт логов в файл
 * 
 * Открывает диалог выбора файла и сохраняет текущее содержимое
 * области логов в выбранный файл.
 */
void LogWindow::exportLogs() {
    QString fileName = QFileDialog::getSaveFileName(this,
        "Экспорт логов", "", "Log Files (*.log);;All Files (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл для записи");
        return;
    }
    
    QTextStream stream(&file);
    stream << m_logView->toPlainText();
    file.close();
}

/**
 * @brief Очистка всех логов
 * 
 * Очищает область отображения логов после подтверждения пользователя.
 */
void LogWindow::clearLogs() {
    m_logView->clear();
}