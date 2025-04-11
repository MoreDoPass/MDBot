#include "BotTabWidget.hpp"
#include "gui/log/LogManager.hpp"
#include "gui/bot/ui/modules/character/CharacterWidget.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

BotTabWidget::BotTabWidget(DWORD processId, QWidget* parent)
    : QWidget(parent)
{
    try {
        // Создаем и инициализируем BotCore
        m_botCore = new BotCore(processId, this);
        LogManager::instance().debug("BotCore instance created", "UI");
        
        // Устанавливаем интерфейс до инициализации, чтобы пользователь видел UI даже если будет ошибка
        setupUi();
        createModuleTabs();
        
        // Инициализируем BotCore после создания UI
        if (!m_botCore->initialize()) {
            LogManager::instance().error("Failed to initialize BotCore", "UI");
            throw std::runtime_error("BotCore initialization failed");
        }
        
        // Подключаем сигналы только после успешной инициализации
        connect(m_botCore, &BotCore::contextUpdated, this, [this]() {
            if (m_characterTab) {
                m_characterTab->onContextUpdated();
            }
        });
        
        connect(m_botCore, &BotCore::stateChanged, this, [this](bool enabled) {
            emit botStateChanged(enabled);
            LogManager::instance().info(
                QString("Bot state changed to: %1").arg(enabled ? "enabled" : "disabled"), 
                "UI"
            );
        });
        
        LogManager::instance().info(
            QString("BotTabWidget initialized successfully for process %1")
                .arg(processId), 
            "UI"
        );
    }
    catch (const std::exception& e) {
        LogManager::instance().error(
            QString("Failed to create BotTabWidget: %1").arg(e.what()), 
            "UI"
        );
        // Очищаем ресурсы в случае ошибки
        if (m_botCore) {
            delete m_botCore;
            m_botCore = nullptr;
        }
        throw; // Пробрасываем исключение дальше для обработки в главном окне
    }
}

BotTabWidget::~BotTabWidget()
{
    if (m_botCore) {
        m_botCore->disable();  // Отключаем бота перед удалением
        delete m_botCore;
        m_botCore = nullptr;
    }
}

void BotTabWidget::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    // Создаем кнопку включения/выключения бота
    auto* botControlButton = new QPushButton("Enable Bot", this);
    connect(botControlButton, &QPushButton::clicked, this, [this, botControlButton]() {
        if (m_botCore) {
            if (m_botCore->isEnabled()) {
                m_botCore->disable();
                botControlButton->setText("Enable Bot");
            } else {
                m_botCore->enable();
                botControlButton->setText("Disable Bot");
            }
        }
    });
    mainLayout->addWidget(botControlButton);
    
    // Создаем вкладки для модулей
    m_moduleTabs = new QTabWidget(this);
    mainLayout->addWidget(m_moduleTabs);
    
    createModuleTabs();
}

void BotTabWidget::createModuleTabs()
{
    // Character Tab
    m_characterTab = new CharacterWidget(this);
    m_moduleTabs->addTab(m_characterTab, "Character");
    
    // Combat Tab
    m_combatTab = new QWidget(this);
    m_moduleTabs->addTab(m_combatTab, "Combat");
    
    // Grind Tab
    m_grindTab = new QWidget(this);
    m_moduleTabs->addTab(m_grindTab, "Grind");
    
    // Questing Tab
    m_questingTab = new QWidget(this);
    m_moduleTabs->addTab(m_questingTab, "Questing");
}