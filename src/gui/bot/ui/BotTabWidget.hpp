#pragma once
#include <QWidget>
#include <QTabWidget>
#include "gui/bot/core/BotCore.hpp"
#include "gui/bot/ui/modules/character/CharacterWidget.hpp"

/**
 * @brief Виджет главного окна бота, содержащий вкладки с различными модулями
 * @details Этот класс представляет собой контейнер для всех модулей бота:
 * - Character - информация о персонаже
 * - Combat - настройки боя
 * - Grind - настройки фарма
 * - Questing - настройки квестинга
 * 
 * Каждый модуль размещается на отдельной вкладке и управляется через BotCore
 */
class BotTabWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Конструктор создает новый экземпляр виджета для управления ботом
     * @param processId ID процесса WoW, к которому подключен бот
     * @param parent Родительский виджет (по умолчанию nullptr)
     */
    explicit BotTabWidget(DWORD processId, QWidget* parent = nullptr);
    
    /**
     * @brief Деструктор освобождает ресурсы, включая BotCore
     */
    ~BotTabWidget();

    /**
     * @brief Проверяет, включен ли бот
     * @return true если бот активен
     */
    bool isEnabled() const { return m_botCore ? m_botCore->isEnabled() : false; }

signals:
    /**
     * @brief Сигнал об изменении состояния бота
     * @param enabled Новое состояние (true - включен, false - выключен)
     */
    void botStateChanged(bool enabled);

private:
    /**
     * @brief Инициализирует пользовательский интерфейс
     * @details Создает основной layout и контейнер для вкладок
     */
    void setupUi();

    /**
     * @brief Создает и инициализирует вкладки для каждого модуля
     * @details Создает следующие вкладки:
     * - Character - для отображения информации о персонаже
     * - Combat - для настройки боевой системы
     * - Grind - для настройки фарма
     * - Questing - для настройки квестинга
     */
    void createModuleTabs();

private:
    BotCore* m_botCore{nullptr};         ///< Ядро бота, управляющее всей логикой
    QTabWidget* m_moduleTabs{nullptr};   ///< Виджет вкладок для размещения модулей
    
    // Модули
    CharacterWidget* m_characterTab{nullptr};  ///< Вкладка информации о персонаже
    QWidget* m_combatTab{nullptr};            ///< Вкладка настроек боя
    QWidget* m_grindTab{nullptr};             ///< Вкладка настроек фарма
    QWidget* m_questingTab{nullptr};          ///< Вкладка настроек квестинга
};