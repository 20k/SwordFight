#ifndef SERVER_NETWORKING_HPP_INCLUDED
#define SERVER_NETWORKING_HPP_INCLUDED

#include "../sword_server/master_server/network_messages.hpp"
#include "../sword_server/master_server/server.hpp"
#include "../sword_server/game_server/game_modes.hpp"
#include "../sword_server/reliability_shared.hpp"
#include "../sword_server/packet_clumping_shared.hpp"
#include <net/shared.hpp>
#include <map>
#include "../openclrenderer/logging.hpp"
#include "game_server_session_resources.hpp"

struct fighter;
struct object_context;
struct physics;
struct world_collision_handler;
struct server_networking;
struct game_server;


///merge into the map_tool definition???
namespace team_defs
{
    static std::vector<std::string> team_names
    {
        "Red",
        "Blue",
        "Gold",
        "Error"
    };
};

///currently excluding reliability manager
struct network_statistics
{
    int bytes_out = 0;
    int bytes_in = 0;
};

///parts pos/rot/hp
///weapon.model/pos/rot
///net.is_blocking
///net.recoil

struct in_progress_ping
{
    sf::Clock clk;

    std::string address;
    std::string port;

    udp_sock sock;
};

///ok. Maybe I should do this properly
///Ideally we'd actually pull out all the networking thats relevant to a server into a
///game server class. That way, when we join we can just destroy it and make a new one
///but... pulling it out into a game server class would be as much effort as creating a simple disconnect method
///and I'm not sure if chopping it up into persistent and non persistent resources is useful, as inevitably
///a game server class will end up containing more information and the distinction will be lost
///BUT... it means we could add more resources to it that would be easy freed automagically rather than having to maintain
///a disconnect function

///Things that need to be destroyed in a disconnect:
///to_game should be closed. to_game_store is invalid. Init = false
///Should tear down fighter and rebuild it completely, which needs it to be removed from the physics system
///clear discovered fighters, clear registered network variables, clear packet callback perplayer, joined_game = false
///my_id = -1, have_id = false, trying to join game = false
///Then need to readd fighter afterwards
///lets go down the separate class route which we can destroy. game_server_session_resouces
struct server_networking
{
    packet_clumper packet_clump;

    reliability_manager reliable_manager;

    sf::Clock time_since_last_send;
    float time_between_sends_ms = 15; ///unused

    game_server_session_resources connected_server;

    //sock_info master_info;
    //sock_info game_info;

    //tcp_sock to_master;
    udp_sock to_master_udp;

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

    void join_master();
    void join_game_tick(const std::string& address, const std::string& port); ///intiates connection attempt and polls
    void join_game_by_local_id_tick(int id);
    void set_game_to_join(const std::string& address, const std::string& port); ///only necessary to call once. You can call more if you like
    void set_game_to_join(int id);
    void disconnect_game();

    ///does not modify internal state
    network_player make_networked_player(int32_t id, object_context* ctx, object_context* tctx, world_collision_handler* hn, physics* phys, int quality);

    static std::vector<game_server> get_serverlist(byte_fetch& fetch);

    void print_serverlist(); ///debugging really
    void tick(object_context* ctx, object_context* tctx, world_collision_handler* collision_handler, physics* phys);
    void ping_master();

    std::vector<fighter*> get_fighters();

    ///for making new fighters
    void set_graphics(int graphics);
    int graphics_settings = 0;

    void handle_ping_response(byte_fetch& fetch);
    void handle_ping(byte_fetch& fetch);
    void handle_ping_data(byte_fetch& fetch);
    void handle_ping_gameserver_response(byte_fetch& fetch);

    void ping();
    void ping_gameserver(const std::string& address, const std::string& port);
    void tick_gameserver_ping_responses(); ///remember that if we're connected to a server and we do this, we'll have a duplicated sock!

    ///for server browser
    std::vector<in_progress_ping> current_server_pings;

    network_statistics get_frame_stats();

    ///if called after tick we get this frames stats, otherwise itll be last frames stats
    ///really this should be tied to game logic, not rendering ;_;
    network_statistics this_frame_stats;

    ///ptr_info is just a descriptor of ptr data size and a pointer
    //std::vector<ptr_info> registered_network_variables;
};

#endif // SERVER_NETWORKING_HPP_INCLUDED
