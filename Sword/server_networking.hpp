#ifndef SERVER_NETWORKING_HPP_INCLUDED
#define SERVER_NETWORKING_HPP_INCLUDED

#include "../sword_server/master_server/network_messages.hpp"
#include "../sword_server/master_server/server.hpp"
#include <net/shared.hpp>
#include <map>

struct fighter;
struct object_context;
struct gameplay_state;
struct physics;

///should really register myself as network player
struct network_player
{
    fighter* fight;
    int32_t id = -1;
};

/*
///NETWORKING COMPONENT DEFINITION
///0 -> parts.size() - individual parts pos/rot
///parts.size() -> parts.size() * 2 -> hp
///parts.size()*2 -> parts.size() * 2 + 1 = weapon.model pos/rot
///parts.size() * 2 + 2 = fight.net.is_blocking
///parts.size() * 2 + 3 = fight.net.recoil*/

///parts pos/rot/hp
///weapon.model/pos/rot
///net.is_blocking
///net.recoil

struct server_networking
{
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

    std::vector<game_server> server_list;

    ///found by just accidentally receiving info
    ///maps player id to actual player
    ///i mean honestly, we want to tie a unique character to this
    //std::map<int32_t, player> discovered_players; ///???
    //std::map<int32_t, bool> discovered_active;
    //std::map<int32_t, fighter*> discovered_fighters; ///???

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

    std::vector<game_server> get_serverlist(byte_fetch& fetch);

    void print_serverlist(); ///debugging really
    void tick(object_context* ctx, gameplay_state* st, physics* phys);
    void ping_master();

    void set_my_fighter(fighter* fight);
};

#endif // SERVER_NETWORKING_HPP_INCLUDED
