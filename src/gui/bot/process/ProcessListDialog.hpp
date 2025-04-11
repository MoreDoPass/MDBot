#pragma once

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>

/**
 * @struct ProcessInfo
 * @brief Структура для хранения информации о процессе
 */
struct ProcessInfo {
    DWORD processId;      ///< Идентификатор процесса
    QString windowTitle;   ///< Заголовок окна процесса
};

/**
 * @class ProcessListDialog
 * @brief Диалог для выбора процесса WoW из списка запущенных процессов
 * @details Отображает список процессов WoW и позволяет пользователю выбрать один из них для подключения
 */
class ProcessListDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Конструктор диалога выбора процесса
     * @param parent Родительский виджет (nullptr по умолчанию)
     */
    explicit ProcessListDialog(QWidget *parent = nullptr);
    ~ProcessListDialog() = default;

    /**
     * @brief Получить ID выбранного процесса
     * @return DWORD - идентификатор выбранного процесса
     */
    DWORD getSelectedProcessId() const;

private slots:
    /**
     * @brief Обновить список процессов
     * @details Очищает текущий список и заново ищет процессы WoW
     */
    void refreshProcessList();
    
    /**
     * @brief Обработчик выбора процесса из списка
     * @details Вызывается при выборе процесса в списке, обновляет selectedProcessId
     */
    void onProcessSelected();
    
    /**
     * @brief Обработчик нажатия кнопки "Принять"
     * @details Закрывает диалог с результатом QDialog::Accepted если процесс выбран
     */
    void accept() override;

private:
    /**
     * @brief Настройка пользовательского интерфейса
     * @details Создает и размещает все виджеты диалога, настраивает сигналы/слоты
     */
    void setupUi();
    
    /**
     * @brief Поиск запущенных процессов WoW
     * @details Сканирует список процессов системы, находит процессы WoW и добавляет их в список
     */
    void findWoWProcesses();
    
    /**
     * @brief Проверяет, является ли процесс процессом WoW
     * @param processId ID проверяемого процесса
     * @param windowTitle Заголовок окна процесса
     * @return true если это процесс WoW, false в противном случае
     */
    static bool isWoWProcess(DWORD processId, const QString& windowTitle);

    /** 
     * @brief Список процессов
     * @details Виджет для отображения списка найденных процессов WoW
     */
    QListWidget* processListWidget;

    /**
     * @brief Кнопка обновления списка процессов
     * @details Позволяет пользователю обновить список доступных процессов WoW
     */
    QPushButton* refreshButton;

    /**
     * @brief Кнопка подтверждения выбора процесса
     * @details Становится активной только когда процесс выбран в списке
     */
    QPushButton* acceptButton;

    /**
     * @brief Кнопка отмены выбора процесса
     * @details Закрывает диалог без выбора процесса
     */
    QPushButton* cancelButton;

    /**
     * @brief ID выбранного процесса
     * @details Хранит идентификатор процесса, выбранного пользователем из списка
     */
    DWORD selectedProcessId;
};