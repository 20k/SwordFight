#ifndef GAME_MODE_SHARED_HPP_INCLUDED
#define GAME_MODE_SHARED_HPP_INCLUDED

#include "game_server/game_modes.hpp"
#include <SFML/System.hpp>
#include <map>

struct server_game_state;

///between client and server
///we're doing it. Its getting unified folks
///Then I can implement stuff
///Ok. First decision. This class IS going to know if its client or server
///we're going to keep all gamemode logic in one place EVEN THOUGH aspects of this won't be run for each client
///This will make it a lot more straightforward to manage gamemodes as EVERYTHING (pretty much) will be in ONE place-ish
///ok. Keep player stats in here relayed from server to client

struct player_info_shared
{
    int32_t kills = 0;
    int32_t assists = 0;
    int32_t deaths = 0;
};

struct game_mode_handler_shared
{
    game_mode_t current_game_mode = game_mode::FIRST_TO_X;

    session_state current_session_state;
    session_boundaries current_session_boundaries;

    std::map<int32_t, player_info_shared> player_info;

    std::string get_display_string();
    std::string get_game_over_string();

    ///true = client, false = server
    void tick(bool is_client);

    bool game_over();

    void register_kill(int32_t id_who_killed, int32_t id_who_died, int32_t team_who_killed, int32_t team_who_died);

    void make_player_entry(int32_t id);
    void remove_player_entry(int32_t id);

    ///useless on client
    bool in_game_over_state = false;
    sf::Clock game_over_timer;

    sf::Clock clk;
};

#endif // GAME_MODE_SHARED_HPP_INCLUDED
