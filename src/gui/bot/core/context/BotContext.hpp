#pragma once
#include <QObject>
#include "../character/CharacterData.hpp"

class BotContext : public QObject {
    Q_OBJECT

public:
    static BotContext& instance();
    
    // Character data
    const CharacterData& character() const { return m_characterData; }
    void updateCharacter(const CharacterData& data);

signals:
    void characterDataChanged();

private:
    BotContext() = default;
    ~BotContext() = default;
    BotContext(const BotContext&) = delete;
    BotContext& operator=(const BotContext&) = delete;

    CharacterData m_characterData;
};