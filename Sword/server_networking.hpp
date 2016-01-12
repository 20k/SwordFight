#ifndef SERVER_NETWORKING_HPP_INCLUDED
#define SERVER_NETWORKING_HPP_INCLUDED

#include "../sword_server/master_server/network_messages.hpp"
#include "../sword_server/master_server/server.hpp"
#include "../sword_server/game_server/game_modes.hpp"
#include "../sword_server/reliability_shared.hpp"
#include <net/shared.hpp>
#include <map>

struct fighter;
struct object_context;
struct gameplay_state;
struct physics;

///should really register myself as network player
struct network_player
{
    fighter* fight = nullptr;
    int32_t id = -1;
};

///merge into the map_tool definition???
namespace team_defs
{
    static std::vector<std::string> team_names
    {
        "Red",
        "Blue",
        "Error"
    };
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

///parts pos/rot/hp
///weapon.model/pos/rot
///net.is_blocking
///net.recoil

struct server_networking
{
    reliability_manager reliable_manager;

    respawn_info respawn_inf;

    gamemode_info game_info;

    sf::Clock time_since_last_send;
    float time_between_sends_ms = 16;

    sock_info master_info;
    //sock_info game_info;

    tcp_sock to_master;
    udp_sock to_game;

    sockaddr_storage to_game_store;
    bool is_init = false;

    server_networking();

    bool has_serverlist = false;
    bool pinged = false;

    bool trying_to_join_game = false;
    std::string trying_to_join_address = "";
    std::string trying_to_join_port = SERVERPORT;

    bool joined_game = false;

    bool just_new_round = false;

    std::vector<game_server> server_list;

    ///found by just accidentally receiving info
    ///maps player id to actual player
    std::map<int32_t, network_player> discovered_fighters;
    int32_t my_id = -1;

    bool have_id = false;

    void join_master();
    void join_game_tick(const std::string& address, const std::string& port); ///intiates connection attempt and polls
    void join_game_by_local_id_tick(int id);
    void set_game_to_join(const std::string& address, const std::string& port); ///only necessary to call once. You can call more if you like
    void set_game_to_join(int id);
    void disconnect_game();

    network_player make_networked_player(int32_t id, object_context* ctx, gameplay_state* st, physics* phys);

    int32_t get_id_from_fighter(fighter* f);

    std::vector<game_server> get_serverlist(byte_fetch& fetch);

    void print_serverlist(); ///debugging really
    void tick(object_context* ctx, gameplay_state* st, physics* phys);
    void ping_master();

    void set_my_fighter(fighter* fight);
};

struct ptr_info
{
    void* ptr = nullptr;
    int size = 0;
};

std::map<int, ptr_info> build_fighter_network_stack(fighter* fight);

template<typename T>
inline
int get_position_of(std::map<int, ptr_info>& stk, T* elem)
{
    for(int i=0; i<stk.size(); i++)
    {
        if(stk[i].ptr == elem)
            return i;
    }

    return -1;
}

///any network discovered fighter

template<typename T>
inline
void
network_update_element(server_networking* net, T* element, fighter* fight)
{
    auto memory_map = build_fighter_network_stack(fight);

    int32_t pos = get_position_of(memory_map, element);

    if(pos == -1)
    {
        printf("Error in network update element -1\n");
        return;
    }

    int32_t network_id = net->get_id_from_fighter(fight);

    if(network_id == -1)
    {
        printf("Error in network update netid -1\n");
        return;
    }

    if(!net->is_init || !net->to_game.valid())
    {
        printf("Some kind of... network error, spewing errors back into the observable universe!\n");
        return;
    }

    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::FORWARDING);
    vec.push_back<int32_t>(network_id);
    vec.push_back<int32_t>(pos);

    int32_t S = sizeof(T);

    vec.push_back<int32_t>(S);
    vec.push_back((uint8_t*)element, S);
    vec.push_back(canary_end);

    udp_send_to(net->to_game, vec.ptr, (const sockaddr*)&net->to_game_store);
}

#endif // SERVER_NETWORKING_HPP_INCLUDED
