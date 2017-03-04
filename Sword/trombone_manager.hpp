#ifndef TROMBONE_MANAGER_HPP_INCLUDED
#define TROMBONE_MANAGER_HPP_INCLUDED

#include <stdint.h>
#include <vec/vec.hpp>

struct engine;
struct fighter;
struct object_context;
struct objects_container;
struct server_networking;

///we need a dirty thing to tell if we've received a network packet
///Uhh. Structure packing here and networking this is going to be an issue
///TONE. THis is a tone
struct net_trombone
{
    uint8_t tone = 0;
    uint8_t dirty = 0;
    vec3f pos = {0,0,0};
};

struct net_trombone_descriptor
{
    uint8_t is_active = 0;
};

struct trombone_manager;

void trombone_packet_callback(void* ptr, int N, trombone_manager& manage);
void trombone_debug(void* ptr, int N);

///need to network trombone position
///ok, this has to be moved into fighter
///every fighter must have a trombone
///its no longer sensible to pretend
///fighter in this context can be a network fighter, tick is called in shared_tick
struct trombone_manager
{
    void init(object_context* _ctx);

    ///called in tick
    void network_tick(int player_id);
    void tick(engine& window, fighter* my_fight);
    void position_model(fighter* my_fight);
    void play(fighter* my_fight, int offset = 0);

    void set_active(bool active);

    bool is_active = false;

    int tone = 0;
    object_context* ctx = nullptr;
    objects_container* trombone = nullptr;

    static constexpr int max_tones = 13;

    void register_server_networking(fighter* my_fight, server_networking* networking);

    int network_offset = -1;
    int network_active_offset = -1;
    net_trombone network_representation;
    net_trombone local_representation;

    net_trombone_descriptor network_trombone_descriptor;

    server_networking* network = nullptr;

    void fully_unload(fighter* my_fight);
};

#endif // TROMBONE_MANAGER_HPP_INCLUDED
