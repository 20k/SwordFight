#ifndef FIGHTER_HPP_INCLUDED
#define FIGHTER_HPP_INCLUDED

#include "../openclrenderer/objects_container.hpp"

#include "vec.hpp"

namespace bodypart
{
    enum bodypart : unsigned int
    {
        HEAD,
        LUPPERARM,
        LLOWERARM,
        RUPPERARM,
        RLOWERARM,
        LHAND,
        RHAND,
        BODY,
        LUPPERLEG,
        LLOWERLEG,
        RUPPERLEG,
        RLOWERLEG,
        COUNT
    };

    static float scale = 100.f;

    ///internal storage
    extern const vec3f* init_default();

    ///for all the bodyparts
    static const vec3f* default_position = init_default();
}

typedef bodypart::bodypart bodypart_t;

struct part
{
    bodypart_t type;
    vec3f pos;
    objects_container model;

    void set_type(bodypart_t); ///sets me up in the default position
    void set_pos(vec3f pos);

    part();
    part(bodypart_t);
    ~part();
};

struct fighter
{
    part parts[bodypart::COUNT];

    fighter();

    ///sigh, cant be on init because needs to be after object load
    void scale();

    void IK_hand(int hand, vec3f pos);
};


#endif // FIGHTER_HPP_INCLUDED
