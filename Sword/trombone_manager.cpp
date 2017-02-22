#include "trombone_manager.hpp"
#include <SFML/Graphics.hpp>
#include "util.hpp"
#include "../openclrenderer/engine.hpp"
#include <vec/vec.hpp>
#include "sound.hpp"
#include "fighter.hpp"
#include "server_networking.hpp"
#include "../openclrenderer/texture.hpp"

void trombone_manager::init(object_context* _ctx)
{
    ctx = _ctx;

    std::string volume = "mezzo-forte";
    std::string duration = "05";
    //std::string octave = "3";
    int octave = 3;
    std::string dir = "./Res/trombone/";

    ///define a max_tones, auto octave scale
    std::string tones[max_tones] =
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

    for(int i=0; i<max_tones; i++)
    {
        std::string str;

        int coctave = i / 12;

        std::string ostr = std::to_string(octave + coctave);

        str = dir + "trombone_" + tones[i % 12] + ostr + "_" + duration + "_" + volume + "_normal.ogg";

        sound::add_extra_soundfile(str);
    }

    trombone = ctx->make_new();
    trombone->set_active(true);
    trombone->set_file("./Res/trombone_cutdown_nomod.obj");
    trombone->cache = false;

    ctx->load_active();
    ctx->build_request();

    texture* yellow = ctx->tex_ctx.make_new();
    yellow->set_create_colour(sf::Color(216, 188, 61), 32, 32);

    for(auto& i : trombone->objs)
    {
        i.tid = yellow->id;
    }

    trombone->set_dynamic_scale(50.f);

    set_active(false);
}

void trombone_packet_callback(void* ptr, int N, trombone_manager& manage)
{
    assert(N == sizeof(net_trombone));
    assert(ptr != nullptr);

    net_trombone* ntr = (net_trombone*)ptr;

    int ntone = ntr->tone;

    if(ntone >= 0 && ntone < manage.max_tones)
    {
        sound::add(11 + ntone, ntr->pos, true, false);

        manage.tone = ntone;
    }
}

void trombone_debug(void* ptr, int N)
{
    uint8_t test = *(uint8_t*)ptr;

    lg::log("test ", test);
}

void trombone_manager::network_tick(int player_id)
{
    if(local_representation.dirty)
    {
        network_representation = local_representation;

        if(network_offset >= 0)
            network->update_network_variable(player_id, network_offset);
    }

    local_representation.dirty = 0;

    network->update_network_variable(player_id, network_active_offset);
}

void trombone_manager::tick(engine& window, fighter* my_fight)
{
    my_fight->weapon.set_active(!network_trombone_descriptor.is_active);

    if(my_fight->network_id != -1)
        network_tick(my_fight->network_id);

    if(!network_trombone_descriptor.is_active)
    {
        trombone->hide();
        return;
    }

    //set_active(false);

    tone += window.get_scrollwheel_delta() > 0.01 ? 1 : 0;
    tone += window.get_scrollwheel_delta() < -0.01 ? -1 : 0;

    tone = clamp(tone, 0, max_tones-1);

    position_model(my_fight);
}

void trombone_manager::position_model(fighter* my_fight)
{
    if(!network_trombone_descriptor.is_active)
    {
        trombone->hide();
        return;
    }

    trombone->set_pos(conv_implicit<cl_float4>(my_fight->parts[bodypart::LHAND].global_pos));

    vec3f up = {0, 1, 0};
    vec3f forw = my_fight->parts[bodypart::LHAND].global_pos - (my_fight->parts[bodypart::HEAD].global_pos - (vec3f){0, 50, 0});

    ///horizontal shift term
    vec3f rcross = cross(forw, up).norm() * 50;

    forw = my_fight->parts[bodypart::LHAND].global_pos - (my_fight->parts[bodypart::HEAD].global_pos - (vec3f){0, 50, 0} - rcross);

    quaternion nq = look_at_quat(forw, up);

    quaternion izquat;
    izquat.load_from_axis_angle({0, 1, 0, -my_fight->rot.v[1]});

    trombone->set_rot_quat(nq * izquat);

    float tone_dist = 3.5f;

    vec3f front_slider = my_fight->parts[bodypart::LHAND].global_pos;

    mat3f r = trombone->rot_quat.get_rotation_matrix();

    front_slider = front_slider + r * ((vec3f){0, 1, 0} * tone_dist * (max_tones - tone));

    trombone->objs[9].set_pos(conv_implicit<cl_float4>(front_slider));
    trombone->objs[2].set_pos(conv_implicit<cl_float4>(front_slider));

    my_fight->override_rhand_pos((front_slider - my_fight->pos).back_rot(0, my_fight->rot));
}

void trombone_manager::play(fighter* my_fight, int offset)
{
    int old_tone = tone;

    tone = clamp(tone + offset, 0, max_tones-1);

    sound::add(11 + tone, my_fight->parts[bodypart::BODY].global_pos, true, false);

    local_representation.tone = tone;
    local_representation.dirty = 1;
    local_representation.pos = my_fight->parts[bodypart::BODY].global_pos;

    tone = old_tone;
}

void trombone_manager::set_active(bool active)
{
    network_trombone_descriptor.is_active = active;

    if(!network_trombone_descriptor.is_active)
    {
        trombone->hide();
    }
}

void trombone_manager::register_server_networking(fighter* my_fight, server_networking* networking)
{
    network = networking;

    ///only register if we're invalid
    if(network_offset != -1)
        return;

    if(my_fight->network_id == -1)
        return;

    network_offset = network->register_network_variable(my_fight->network_id, &network_representation);
    network_active_offset = network->register_network_variable(my_fight->network_id, &network_trombone_descriptor.is_active);

    ///only register callback if we're now valid
    if(network_offset == -1)
        return;

    network->register_packet_callback(my_fight->network_id, network_offset, std::bind(trombone_packet_callback, std::placeholders::_1, std::placeholders::_2, *this));
    //network->register_packet_callback(my_fight->network_id, network_active_offset, trombone_debug);
}
