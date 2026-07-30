#pragma once

class Input;
class BattleState;

class HumanTurnController
{
public:
    explicit HumanTurnController(BattleState &state) : m_state(state) {}
    void handleActiveTurn(const Input &input);

private:
    BattleState &m_state;
};
