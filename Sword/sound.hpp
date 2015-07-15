#ifndef SOUND_HPP_INCLUDED
#define SOUND_HPP_INCLUDED

#include "vec.hpp"

struct sound
{
    static vec3f listener_pos;
    static vec3f listener_rot;

    static void add(int type, vec3f pos);

    static void set_listener(vec3f pos, vec3f rot);
};


#endif // SOUND_HPP_INCLUDED
