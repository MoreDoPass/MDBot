#include "ProcessListDialog.hpp"

#include <TlHelp32.h>

#include <QMessageBox>
#include <QVBoxLayout>

#include "core/memory/MemoryManager.hpp"
#include "gui/log/LogManager.hpp"

#include <psapi.h>

#pragma comment(lib, "Psapi.lib")

// Адрес смещения имени персонажа от базы run.exe
constexpr uintptr_t PLAYER_NAME_OFFSET = 0x879D18;

ProcessListDialog::ProcessListDialog(QWidget* parent) : QDialog(parent), selectedProcessId(0)
{
    setWindowTitle("Выбор процесса WoW");
    setupUi();
    refreshProcessList();
}

void ProcessListDialog::setupUi()
{
    // Создаем основной вертикальный layout
    auto* mainLayout = new QVBoxLayout(this);

    // Создаем и настраиваем список процессов
    processListWidget = new QListWidget(this);
    processListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(processListWidget, &QListWidget::itemSelectionChanged, this, &ProcessListDialog::onProcessSelected);

    // Создаем кнопки
    refreshButton = new QPushButton("Обновить список", this);
    acceptButton  = new QPushButton("Добавить процесс", this);
    cancelButton  = new QPushButton("Отмена", this);

    // Настраиваем кнопки
    acceptButton->setEnabled(false); // Изначально кнопка неактивна
    connect(refreshButton, &QPushButton::clicked, this, &ProcessListDialog::refreshProcessList);
    connect(acceptButton, &QPushButton::clicked, this, &ProcessListDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &ProcessListDialog::reject);

    // Создаем горизонтальный layout для кнопок
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(acceptButton);
    buttonLayout->addWidget(cancelButton);

    // Добавляем виджеты в основной layout
    mainLayout->addWidget(processListWidget);
    mainLayout->addLayout(buttonLayout);

    // Устанавливаем размеры окна
    resize(400, 300);
}

void ProcessListDialog::refreshProcessList()
{
    processListWidget->clear();
    findWoWProcesses();
}

void ProcessListDialog::findWoWProcesses()
{
    // Получаем снимок процессов системы
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось получить список процессов");
        return;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(pe32);

    // Перебираем все процессы
    if (Process32FirstW(snapshot, &pe32))
    {
        do
        {
            QString processName = QString::fromWCharArray(pe32.szExeFile);

            // Проверяем, является ли процесс run.exe
            if (processName.toLower() == "run.exe")
            {
                QString characterName;
                bool    readSuccess = false;
                QString errorMessage;

                try
                {
                    MemoryManager memory(pe32.th32ProcessID);
                    // Проверяем, что базовый адрес получен успешно
                    if (memory.GetModuleBaseAddress() != 0)
                    {
                        // Проверяем валидность адреса перед чтением
                        uintptr_t nameAddress = memory.ResolveAddress(PLAYER_NAME_OFFSET);
                        if (memory.IsValidAddress(nameAddress))
                        {
                            std::string name = memory.ReadString(
                                PLAYER_NAME_OFFSET, 12, true); // используем true для относительного адреса
                            if (MemoryManager::IsValidCharacterName(name))
                            {
                                characterName = QString::fromStdString(name);
                                readSuccess   = true;
                            }
                            else
                            {
                                errorMessage = "Invalid character name format";
                            }
                        }
                        else
                        {
                            errorMessage = "Invalid memory address";
                        }
                    }
                    else
                    {
                        errorMessage = "Failed to get module base address";
                    }
                }
                catch (const std::exception& e)
                {
                    errorMessage = QString("Error: %1").arg(e.what());
                    qDebug() << "Error reading process" << pe32.th32ProcessID << ":" << e.what();
                }

                // Формируем текст для отображения
                QString itemText;
                if (readSuccess)
                {
                    itemText = QString("run.exe - %1 (PID: %2)").arg(characterName).arg(pe32.th32ProcessID);
                }
                else
                {
                    itemText = QString("run.exe (PID: %1) - %2").arg(pe32.th32ProcessID).arg(errorMessage);
                }

                auto* item = new QListWidgetItem(itemText);
                item->setData(Qt::UserRole, QVariant(static_cast<quint32>(pe32.th32ProcessID)));
                item->setToolTip(errorMessage); // Добавляем подсказку с ошибкой
                processListWidget->addItem(item);
            }
        } while (Process32NextW(snapshot, &pe32));
    }

    CloseHandle(snapshot);

    // Обновляем статус кнопки
    acceptButton->setEnabled(processListWidget->count() > 0);
}

bool ProcessListDialog::isWoWProcess(DWORD processId, const QString& windowTitle)
{
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!processHandle)
    {
        return false;
    }

    bool  isWoW = false;
    WCHAR processPath[MAX_PATH];
    DWORD pathSize = MAX_PATH;

    if (GetModuleFileNameExW(processHandle, NULL, processPath, pathSize))
    {
        QString path = QString::fromWCharArray(processPath).toLower();
        // Проверяем и имя файла и заголовок окна
        isWoW = (path.contains("wow.exe") || path.contains("run.exe"))
                && (windowTitle.contains("World of Warcraft", Qt::CaseInsensitive)
                    || windowTitle.contains("WoW", Qt::CaseInsensitive));

        LogManager::instance().debug(QString("Process check - PID: %1, Path: %2, Title: %3, IsWoW: %4")
                                         .arg(processId)
                                         .arg(path)
                                         .arg(windowTitle)
                                         .arg(isWoW),
                                     "ProcessList");
    }

    CloseHandle(processHandle);
    return isWoW;
}

void ProcessListDialog::onProcessSelected()
{
    QList<QListWidgetItem*> selectedItems = processListWidget->selectedItems();
    acceptButton->setEnabled(!selectedItems.isEmpty());
}

void ProcessListDialog::accept()
{
    QList<QListWidgetItem*> selectedItems = processListWidget->selectedItems();
    if (!selectedItems.isEmpty())
    {
        bool    ok;
        quint32 pid = selectedItems.first()->data(Qt::UserRole).toUInt(&ok);
        if (ok)
        {
            selectedProcessId = pid;
            QDialog::accept();
        }
        else
        {
            QMessageBox::warning(this, "Ошибка", "Не удалось получить ID процесса");
        }
    }
}

DWORD ProcessListDialog::getSelectedProcessId() const
{
    return selectedProcessId;
}