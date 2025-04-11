#include "CharacterWidget.hpp"
#include "gui/bot/core/BotCore.hpp"
#include "gui/log/LogManager.hpp"
#include <QVBoxLayout>

CharacterWidget::CharacterWidget(QWidget* parent)
    : QWidget(parent)
    , m_levelLabel(new QLabel("Level: 0", this))
    , m_healthLabel(new QLabel("Health: 0/0", this))
    , m_manaLabel(new QLabel("Mana: 0/0", this))
    , m_eaxLabel(new QLabel("EAX: 0x00000000", this))
{
    LogManager::instance().debug("Creating CharacterWidget", "UI");
    setupUi();
}

void CharacterWidget::setupUi() {
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_levelLabel);
    layout->addWidget(m_healthLabel);
    layout->addWidget(m_manaLabel);
    layout->addWidget(m_eaxLabel);
    setLayout(layout);
    LogManager::instance().debug("CharacterWidget UI setup complete", "UI");
}

void CharacterWidget::onContextUpdated() {
    LogManager::instance().debug("Context update received", "Character");
    try {
        const BotContext& context = qobject_cast<BotCore*>(parent()->parent())->context();
        updateLabels();
    } catch (const std::exception& e) {
        LogManager::instance().error(QString("Error updating context: %1").arg(e.what()), "Character", "");
    }
}

void CharacterWidget::updateLabels() {
    try {
        const BotContext& context = qobject_cast<BotCore*>(parent()->parent())->context();
        const auto& character = context.character;
        
        m_levelLabel->setText(QString("Level: %1").arg(character.level));
        m_healthLabel->setText(QString("Health: %1/%2")
            .arg(character.currentHealth)
            .arg(character.maxHealth));
        m_manaLabel->setText(QString("Mana: %1/%2")
            .arg(character.currentMana)
            .arg(character.maxMana));
        m_eaxLabel->setText(QString("EAX: 0x%1")
            .arg(character.eaxRegister, 8, 16, QChar('0')));
            
        LogManager::instance().debug("Labels updated successfully", "Character");
    } catch (const std::exception& e) {
        LogManager::instance().error(QString("Error updating labels: %1").arg(e.what()), "Character", "");
    }
}