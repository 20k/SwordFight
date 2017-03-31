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


void game_mode_handler_shared::register_kill(int32_t id_who_killed, int32_t id_who_died, int32_t team_who_killed, int32_t team_who_died)
{
    if(id_who_killed < 0)
        return;

    if(id_who_died < 0)
        return;

    if(team_who_died < 0 || team_who_died >= TEAM_NUMS)
        return;

    if(team_who_killed < 0 || team_who_killed >= TEAM_NUMS)
        return;

    player_info[id_who_killed].kills++;
    player_info[id_who_died].deaths++;

    current_session_state.team_kills[team_who_killed]++;
    current_session_state.team_killed[team_who_died]++;
}

void game_mode_handler_shared::make_player_entry(int32_t id)
{
    player_info[id] = player_info_shared();
}

void game_mode_handler_shared::remove_player_entry(int32_t id)
{
    player_info.erase(id);
}
