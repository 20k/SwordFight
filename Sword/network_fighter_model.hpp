#ifndef NETWORK_FIGHTER_MODEL_HPP_INCLUDED
#define NETWORK_FIGHTER_MODEL_HPP_INCLUDED

#include "fighter.hpp"

///so, this is the networked representation we receive for a client

struct network_part_info
{
    vec3f local_pos;
    vec3f local_rot;

    float hp;
};

struct network_sword_info
{
    vec3f global_pos;
    vec3f global_rot;

    int32_t is_blocking = 0;
    int32_t is_recoiling = 0;
    int32_t is_damaging = 0;
};

struct network_fighter_info
{
    vec3f pos;
    vec3f rot;

    int32_t is_dead = 0;

    uint8_t name[MAX_NAME_LENGTH];
};

namespace network_message
{
    enum message_t
    {
        CLANG_AUDIO,
        HIT_AUDIO,
        HIT
    };

    struct network_union
    {
        message_t type;

        union {
            vec3f pos;
            float hp_lost;
        };
    };
}

struct network_fighter
{
    network_part_info network_parts[bodypart::COUNT];

    network_fighter_info network_fighter_inf;


};

#endif // NETWORK_FIGHTER_MODEL_HPP_INCLUDED
