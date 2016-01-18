#ifndef GAME_STATE_MANAGER_HPP_INCLUDED
#define GAME_STATE_MANAGER_HPP_INCLUDED

namespace game_state
{
    enum game_state
    {
        MENU,
        OPTIONS,
        SERVERBROWSER,
        GAME
    };
}

struct state_manager
{
    game_state::game_state current_state;

    void tick();

    void tick_menu();
    void tick_options();
    void tick_serverbrowser();
    void tick_game();
};

#endif // GAME_STATE_MANAGER_HPP_INCLUDED
