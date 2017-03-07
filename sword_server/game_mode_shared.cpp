#include "game_mode_shared.hpp"

bool game_mode_handler_shared::game_over()
{
    return current_session_state.game_over(current_session_boundaries);
}

void game_mode_handler_shared::tick(bool is_client)
{
    current_session_state.time_elapsed += clk.getElapsedTime().asMicroseconds() / 1000.f;
    clk.restart();
}

///needs to be gamemode specific really
std::string game_mode_handler_shared::get_game_over_string()
{
    return current_session_state.get_game_over_string(current_session_boundaries);
}

///gamemode?
///centre align?
std::string game_mode_handler_shared::get_display_string()
{
    return current_session_state.get_current_game_state_string(current_session_boundaries);
}
