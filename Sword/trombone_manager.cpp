#include "trombone_manager.hpp"
#include <SFML/Graphics.hpp>
#include "util.hpp"
#include "../openclrenderer/engine.hpp"
#include <vec/vec.hpp>
#include "sound.hpp"
#include "fighter.hpp"

void trombone_manager::init(object_context* _ctx)
{
    ctx = _ctx;

    std::string volume = "mezzo-forte";
    std::string duration = "05";
    std::string octave = "3";
    std::string dir = "./Res/trombone/";

    std::string tones[12] =
    {
        "C",
        "Cs",
        "D",
        "Ds",
        "E",
        "F",
        "Fs",
        "G",
        "Gs",
        "A",
        "As",
        "B",
    };

    for(int i=0; i<12; i++)
    {
        std::string str;

        str = dir + "trombone_" + tones[i] + octave + "_" + duration + "_" + volume + "_normal.ogg";

        sound::add_extra_soundfile(str);
    }

    trombone = ctx->make_new();
    trombone->set_active(true);
    trombone->set_file("./Res/trombone_cutdown_nomod.obj");
    trombone->cache = false;

    ctx->load_active();
    ctx->build_request();

    trombone->set_dynamic_scale(50.f);
}

void trombone_manager::tick(engine& window, fighter* my_fight)
{
    //if(is_active)
    {
        my_fight->weapon.set_active(!is_active);
    }

    if(!is_active)
        return;

    tone += window.get_scrollwheel_delta() > 0.01 ? 1 : 0;
    tone += window.get_scrollwheel_delta() < -0.01 ? -1 : 0;

    tone = clamp(tone, 0, 11);

    /*sf::Keyboard key;

    if(once<sf::Keyboard::Num1>())
    {
        sound::add(11 + tone, my_fight->parts[bodypart::BODY].global_pos, true, false);
    }*/

    trombone->set_pos(conv_implicit<cl_float4>(my_fight->parts[bodypart::LHAND].global_pos));

    vec3f up = {0, 1, 0};
    vec3f forw = my_fight->parts[bodypart::LHAND].global_pos - (my_fight->parts[bodypart::HEAD].global_pos - (vec3f){0, 50, 0});

    vec3f rcross = cross(forw, up).norm() * 50;

    forw = my_fight->parts[bodypart::LHAND].global_pos - (my_fight->parts[bodypart::HEAD].global_pos - (vec3f){0, 50, 0} - rcross);

    quaternion nq = look_at_quat(forw, up);

    quaternion izquat;
    izquat.load_from_axis_angle({0, 1, 0, -my_fight->rot.v[1]});

    trombone->set_rot_quat(nq * izquat);

    float tone_dist = 3.5f;

    vec3f front_slider = my_fight->parts[bodypart::LHAND].global_pos;

    mat3f r = trombone->rot_quat.get_rotation_matrix();

    front_slider = front_slider + r * ((vec3f){0, 1, 0} * tone_dist * tone);

    trombone->objs[9].set_pos(conv_implicit<cl_float4>(front_slider));
    trombone->objs[2].set_pos(conv_implicit<cl_float4>(front_slider));
}

void trombone_manager::play(fighter* my_fight)
{
    sound::add(11 + tone, my_fight->parts[bodypart::BODY].global_pos, true, false);
}

void trombone_manager::set_active(bool active)
{
    is_active = active;

    if(!is_active)
    {
        trombone->hide();
    }
}
