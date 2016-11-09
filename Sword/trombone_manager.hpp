#ifndef TROMBONE_MANAGER_HPP_INCLUDED
#define TROMBONE_MANAGER_HPP_INCLUDED

#include <stdint.h>
#include <vec/vec.hpp>

struct engine;
struct fighter;
struct object_context;
struct objects_container;

///we need a dirty thing to tell if we've received a network packet
struct net_trombone
{
    int32_t tone = 0;
    int8_t dirty = 0;
    vec3f pos = {0,0,0};
};

struct trombone_manager
{
    void init(object_context* _ctx);

    void tick(engine& window, fighter* my_fight);
    void play(fighter* my_fight);

    void set_active(bool active);

    bool is_active = false;

    int tone = 0;
    object_context* ctx = nullptr;
    objects_container* trombone = nullptr;

    static constexpr int max_tones = 13;

    net_trombone construct_net_trombone();
    void load_from_net_trombone(const net_trombone& net);
};

#endif // TROMBONE_MANAGER_HPP_INCLUDED
