#include "game_server_session_resources.hpp"
#include "fighter.hpp"
#include "network_fighter_model.hpp"
#include "server_networking.hpp"
#include "network_update_wrappers.hpp"

std::vector<delay_information> delay_vectors;
float delay_ms = 200.f;

void gamemode_info::process_gamemode_update(byte_fetch& arg)
{
    byte_fetch fetch = arg;

    shared_game_state.current_game_mode = (game_mode_t)fetch.get<int32_t>();

    shared_game_state.current_session_state = fetch.get<decltype(shared_game_state.current_session_state)>();
    shared_game_state.current_session_boundaries = fetch.get<decltype(shared_game_state.current_session_boundaries)>();

    /*max_time_elapsed = fetch.get<float>();
    max_kills = fetch.get<int32_t>();*/

    int32_t found_end = fetch.get<int32_t>();

    if(found_end != canary_end)
    {
        lg::log("err in proces gamemode updates bad canary");
        return;
    }

    arg = fetch;

    ///so that our timer isn't just the servers time
    shared_game_state.clk.restart();
}

void gamemode_info::tick()
{
    shared_game_state.tick(true);
}

bool gamemode_info::game_over()
{
    return shared_game_state.game_over();
}

void game_server_session_resources::update_fighter_name_infos()
{
    for(auto& i : discovered_fighters)
    {
        if(i.first == my_id)
            continue;

        if(i.second.id < 0)
        {
            lg::log("super bad error, invalid fighter 2");
            continue;
        }

        if(!i.second.fight->dead())
            i.second.fight->update_name_info(true);
    }
}

void game_server_session_resources::update_fighter_gpu_name()
{
    for(auto& i : discovered_fighters)
    {
        if(i.first == my_id)
            continue;

        if(i.second.id < 0)
        {
            lg::log("super bad error, invalid fighter 2");
            continue;
        }

        if(!i.second.fight->dead())
            i.second.fight->update_gpu_name();
    }
}

int32_t game_server_session_resources::get_id_from_fighter(fighter* f)
{
    for(auto& i : discovered_fighters)
    {
        if(i.second.fight == f)
            return i.first;
    }

    return -1;
}

void game_server_session_resources::set_my_fighter(fighter* fight)
{
    if(!have_id)
        return;

    network_player net_player;

    net_player.net_fighter = new network_fighter;
    net_player.fight = fight;
    net_player.id = my_id;

    //discovered_fighters[my_id] = {new network_fighter, fight, my_id};

    discovered_fighters[my_id] = net_player;

    fight->set_network_id(my_id);
}

void game_server_session_resources::update_network_variable(server_networking* server, int player_id, int num)
{
    if(!have_id || discovered_fighters[player_id].fight == nullptr || player_id == -1)
    {
        lg::log("Warning, no fighter id or null fighter set in update_network_variable");
        return;
    }

    auto net_map = build_fighter_network_stack(&discovered_fighters[player_id], this);

    if(net_map.find(num) == net_map.end())
    {
        lg::log("No element with offset num ", num, " found");
        return;
    }

    network_update_element(server, net_map[num].ptr, &discovered_fighters[player_id], net_map[num].size);
}

void game_server_session_resources::disconnect()
{
    to_game.close();

    for(auto net_fights : discovered_fighters)
    {
        fighter* fight = net_fights.second.fight;

        bool should_delete = false;

        if(fight->network_id != my_id)
        {
            should_delete = true;
        }

        fight->fully_unload();

        if(should_delete)
            delete fight;

        delete net_fights.second.net_fighter;
    }

    *this = game_server_session_resources();
}
