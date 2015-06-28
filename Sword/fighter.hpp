#ifndef FIGHTER_HPP_INCLUDED
#define FIGHTER_HPP_INCLUDED

#include "../openclrenderer/objects_container.hpp"

#include "vec.hpp"

#include <SFML/Graphics.hpp>

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

struct movement
{
    sf::Clock clk;

    float end_time;
    vec3f fin;
    vec3f start;
    int type; ///0 for linear, 1 for slerp

    int hand; ///need leg support too
    bodypart_t limb;

    bool going;

    float get_frac();

    void fire();

    bool finished();

    movement();
};

struct fighter
{
    part parts[bodypart::COUNT];

    fighter();

    std::vector<movement> moves;


    ///sigh, cant be on init because needs to be after object load
    void scale();

    void IK_hand(int hand, vec3f pos);

    void linear_move(int hand, vec3f pos, float time);
    void spherical_move(int hand, vec3f pos, float time);

    void tick();
};


#endif // FIGHTER_HPP_INCLUDED
