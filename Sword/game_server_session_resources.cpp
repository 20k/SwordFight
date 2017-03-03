#include "game_server_session_resources.hpp"
#include "fighter.hpp"
#include "network_fighter_model.hpp"
#include "server_networking.hpp"
#include "network_update_wrappers.hpp"

std::vector<delay_information> delay_vectors;
float delay_ms = 200.f;

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
