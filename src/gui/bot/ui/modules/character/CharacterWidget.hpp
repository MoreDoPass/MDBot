#pragma once
#include <QWidget>
#include <QLabel>
#include "gui/bot/core/BotCore.hpp"

/**
 * @brief Виджет отображения информации о персонаже
 * @details Отображает основную информацию о персонаже:
 * - Уровень персонажа
 * - Текущее здоровье и максимальное здоровье
 * - Текущая мана и максимальная мана
 * - Значение регистра EAX (для отладки)
 */
class CharacterWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Конструктор виджета
     * @param parent Родительский виджет
     */
    explicit CharacterWidget(QWidget* parent = nullptr);

    /**
     * @brief Слот обновления при изменении контекста бота
     * @details Вызывается когда данные персонажа в BotContext изменяются
     */
    void onContextUpdated();

private:
    /**
     * @brief Инициализация UI компонентов
     * @details Создает и размещает все метки на виджете
     */
    void setupUi();

    /**
     * @brief Обновляет текст меток актуальными данными
     * @details Получает данные из BotContext и обновляет отображение
     */
    void updateLabels();

    QLabel* m_levelLabel;    ///< Метка для отображения уровня
    QLabel* m_healthLabel;   ///< Метка для отображения здоровья
    QLabel* m_manaLabel;     ///< Метка для отображения маны
    QLabel* m_eaxLabel;      ///< Метка для отображения значения EAX
};