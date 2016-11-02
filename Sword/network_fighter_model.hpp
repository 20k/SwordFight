#ifndef NETWORK_FIGHTER_MODEL_HPP_INCLUDED
#define NETWORK_FIGHTER_MODEL_HPP_INCLUDED

///so, this is the networked representation we receive for a client

#include "fighter.hpp"

template<typename T>
struct network_wrap
{
    T networked_val = T(); ///received from the networking, and sent across the networking
    T unmodified_val = T();
    T local_val = T();

    bool should_network = false;

    T get_received_networked_val()
    {
        return unmodified_val;
    }

    T get_local_val()
    {
        return local_val;
    }

    void set_local_val(const T& val)
    {
        local_val = val;
    }

    ///dunnae touch
    T* get_networking_ptr()
    {
        return &networked_val;
    }

    void set_transmit_val(T pval)
    {
        networked_val = pval;
        should_network = true;
    }

    bool should_send()
    {
        return should_network;
    }

    ///because we can only send/receive through networked val
    ///we backup the internal structure
    void update_internal()
    {
        unmodified_val = networked_val;
        local_val = networked_val;

        should_network = false;
    }

    void network_local()
    {
        set_transmit_val(local_val);
    }

    /*void reset()
    {
        networked_val = T();
    }*/
};

struct network_part_info
{
    vec3f global_pos;
    vec3f global_rot;

    float hp = 0;
    network_wrap<damage_info> requested_damage_info;
    int32_t play_hit_audio = 0;
};

struct network_sword_info
{
    vec3f global_pos;
    vec3f global_rot;

    uint8_t is_blocking = 0;
    //int32_t is_recoiling = 0;
    uint8_t is_damaging = 0;
};

struct network_fighter_info
{
    vec3f pos;
    vec3f rot;

    int32_t is_dead = 0;

    vec<MAX_NAME_LENGTH + 1, int8_t> name = {0};

    network_wrap<int32_t> recoil_requested;
    network_wrap<int32_t> recoil_forced;
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

    network_sword_info network_sword;

    network_fighter_info network_fighter_inf;
};

#endif // NETWORK_FIGHTER_MODEL_HPP_INCLUDED
