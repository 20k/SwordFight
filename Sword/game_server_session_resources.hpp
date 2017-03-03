#ifndef GAME_SERVER_SESSION_RESOURCES_HPP_INCLUDED
#define GAME_SERVER_SESSION_RESOURCES_HPP_INCLUDED

#include <map>
#include <vector>
#include <SFML/System.hpp>
#include "../sword_server/game_server/game_modes.hpp"
#include <net/shared.hpp>
#include "../openclrenderer/logging.hpp"

struct fighter;
struct network_fighter; ///new fighter networking model
struct server_networking;
struct game_server_session_resources;

struct ptr_info
{
    void* ptr = nullptr;
    int size = 0;
};

template<typename T>
ptr_info get_inf(T* ptr)
{
    return {(void*)ptr, sizeof(T)};
}

template<int N>
ptr_info get_inf(void* ptr)
{
    return {ptr, N};
}

///should really register myself as network player
///ok, we're gunna have to have 3 representations
///1 is raw, 1 is for the client to tinker with with no repercussions
///1 is for the client to tinker with and authoritatively have the changes
///networked
struct network_player
{
    network_fighter* net_fighter = nullptr;
    fighter* fight = nullptr;
    int32_t id = -1;
    sf::Clock disconnect_timer;
    float disconnect_time_ms = 10000;
    bool cleanup = false;
};

///should probably unify this
///put me into game_modes?
struct gamemode_info
{
    game_mode_t current_mode;

    session_state current_session_state;
    session_boundaries current_session_boundaries;

    void process_gamemode_update(byte_fetch& fetch);

    std::string get_display_string();
    std::string get_game_over_string();

    void tick();

    bool game_over();

    sf::Clock clk;
};

struct respawn_info
{
    float time_elapsed = 0;
    float spawn_time = 1;

    std::string get_display_string();
};

std::map<int, ptr_info> build_fighter_network_stack(network_player* net_fight, game_server_session_resources* networking);

///destructing this class disconnects us from the server (or similar)
struct game_server_session_resources
{
    respawn_info respawn_inf;
    gamemode_info game_info;

    udp_sock to_game;
    sockaddr_storage to_game_store;

    ///found by just accidentally receiving info
    ///maps player id to actual player
    std::map<int32_t, network_player> discovered_fighters;
    int32_t my_id = -1;
    bool have_id = false;
    bool joined_server = false;

    ///networking id
    int32_t get_id_from_fighter(fighter* f);

    void update_fighter_name_infos();
    void update_fighter_gpu_name();

    void set_my_fighter(fighter* fight);

    std::map<int, std::vector<ptr_info>> registered_network_variable_perplayer;

    ///player id -> map[component_id -> packet callback]
    ///ie for every player id, we can define a callback per-component
    std::map<int, std::map<int, std::function<void(void*, int)>>> packet_callback_perplayer;

    template<typename T>
    int register_network_variable(int player_id, T* ptr)
    {
        if(!have_id || discovered_fighters[player_id].fight == nullptr)// || discovered_fighters[player_id].fight->network_id != player_id)
        {
            //lg::log("Warning, no fighter id or null fighter set in register_network_variable");
            return -1;
        }

        std::map<int, ptr_info> net_map = build_fighter_network_stack(&discovered_fighters[player_id], this);

        int num = net_map.size();

        ptr_info inf = get_inf(ptr);

        registered_network_variable_perplayer[player_id].push_back(inf);

        return num;
    }

    void update_network_variable(server_networking* server, int player_id, int num);

    void register_packet_callback(int player_id, int component_id, std::function<void(void*, int)> func)
    {
        packet_callback_perplayer[player_id][component_id] = func;
    }
};

#endif // GAME_SERVER_SESSION_RESOURCES_HPP_INCLUDED
