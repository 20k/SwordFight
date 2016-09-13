#ifndef SOUND_HPP_INCLUDED
#define SOUND_HPP_INCLUDED

#include <vec/vec.hpp>
#include <vector>

struct sound
{
    static vec3f listener_pos;
    static vec3f listener_rot;

    static void add(int type, vec3f pos, bool relative = false, bool random = true);

    static void set_listener(vec3f pos, vec3f rot);

    static void update_listeners();

    static void add_extra_soundfile(const std::string& sfile);

    static int extra_start;
};


#endif // SOUND_HPP_INCLUDED
