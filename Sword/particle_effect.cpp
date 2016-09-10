#include "particle_effect.hpp"
#include "../openclrenderer/objects_container.hpp"
#include "object_cube.hpp"
#include "../openclrenderer/light.hpp"
#include "../sword_server/teaminfo_shared.hpp"
#include "../openclrenderer/obj_load.hpp"
#include "../openclrenderer/texture.hpp"

std::vector<effect*> particle_effect::effects;

///this is probably a big reason for the slowdown on dying?
void cube_effect::make(float duration, vec3f _pos, float _scale, int _team, int _num, object_context& _cpu_context)
{
    cpu_context = &_cpu_context;

    for(auto& i : objects)
    {
        cpu_context->destroy(i);
    }

    objects.clear();

    duration_ms = duration;
    pos = _pos;
    scale = _scale;

    elapsed_time.restart();

    num = _num;
    team = _team;

    ///we need to update the cache to be able to deal with texture ids
    ///have objects_container->cache_textures
    for(int i=0; i<num; i++)
    {
        vec3f p1 = {0,0,0};

        //float len = 10.f * randf<1, float>(2, 0.1);;
        float len = 10;

        vec3f p2 = p1 + (vec3f){0, 0, 10.f};

        texture_context* tex_ctx = &cpu_context->tex_ctx;
        texture* ntex = tex_ctx->make_new_cached(team_info::get_texture_cache_name(_team));

        vec3f col = team_info::get_team_col(_team);
        ntex->set_create_colour({col.v[0], col.v[1], col.v[2]}, 128, 128);

        objects_container* o = cpu_context->make_new();
        o->set_load_func(std::bind(load_object_cube_tex, std::placeholders::_1, p1, p2, len/2, *ntex));

        vec3f rpos = (randf<3, float>() - 0.5f) * scale;

        vec3f lpos = pos + rpos;

        o->set_pos({lpos.v[0], lpos.v[1], lpos.v[2]});

        objects.push_back(o);
    }

    /*for(int i=0; i<num; i++)
    {
        vec3f p1 = {0,0,0};

        float len = 10.f * randf<1, float>(2, 0.1);

        vec3f p2 = p1 + (vec3f){0, 0, len};


        texture_context* tex_ctx = &cpu_context->tex_ctx;
        texture* ntex = tex_ctx->make_new_cached(team_info::get_texture_cache_name(_team));

        vec3f col = team_info::get_team_col(_team);
        ntex->set_create_colour({col.v[0], col.v[1], col.v[2]}, 128, 128);


        objects_container* o = cpu_context->make_new();
        o->set_load_func(std::bind(load_object_cube_tex, std::placeholders::_1, p1, p2, len/2, *ntex));
        o->cache = false;

        vec3f rpos = (randf<3, float>() - 0.5f) * scale;

        vec3f lpos = pos + rpos;

        o->set_pos({lpos.v[0], lpos.v[1], lpos.v[2]});

        objects.push_back(o);
    }*/
}

void cube_effect::activate()
{
    for(objects_container* i : objects)
    {
        i->set_active(true);

        i->parent->load_active();

        for(object& o : i->objs)
        {
            o.tid = i->parent->tex_ctx.make_new_cached(team_info::get_texture_cache_name(team))->id;
        }

        float dyn_scale = randf<1, float>(2.f, 0.1f);
        i->set_dynamic_scale(dyn_scale);
    }
}

///when we do async, trying to flush to an un alloced memory piece will asplod everything
void cube_effect::tick()
{
    //for(int i=0; i<effects.size(); i++)
    {
        ///move all particles away from centre slowly, based on duration_ms

        cube_effect& e = *this;

        float time = e.elapsed_time.getElapsedTime().asMicroseconds() / 1000.f;

        float remaining = e.duration_ms - time;

        vec3f centre = e.pos;

        for(auto& o : e.objects)
        {
            vec3f dir = xyz_to_vec(o->pos) - centre;

            dir = dir.norm();

            float time_frac = time / e.duration_ms;

            time_frac = - time_frac * (time_frac - 2);

            vec3f pos = dir * time_frac * e.scale + centre;

            o->set_pos({pos.v[0], pos.v[1], pos.v[2]});
        }

        ///if we make them disappear one by one, itll be much more bettererer
        float rem_frac = remaining / e.duration_ms;

        ///we're coming to a close
        if(rem_frac < 0.2)
        {
            rem_frac /= 0.2;

            float total_num = e.num;

            float num_left = rem_frac * total_num;

            float to_remove = total_num - num_left;

            for(int j=0; j<to_remove && j < e.objects.size(); j++)
            {
                objects_container* o = e.objects[j];

                o->set_pos({0, 0, -3000000});
            }

            if(remaining < 0)
            {
                for(auto& o : e.objects)
                {
                    o->set_pos({0, 0, -30000000});
                    o->set_active(false);

                    cpu_context->destroy(o);

                    finished = true;
                }
            }
        }
    }
}

void light_effect::make(float duration, light* _l)
{
    l = _l;

    pos = xyz_to_vec(l->pos);

    start = pos;

    duration_ms = duration;
}

///need to make this frametime independent
void light_effect::tick()
{
    float time = elapsed_time.getElapsedTime().asMicroseconds() / 1000.f;

    pos = start + (vec3f){0, 0.2f, 0} * time;

    l->set_pos({pos.v[0], pos.v[1], pos.v[2]});

    if(time > duration_ms)
        finished = true;
}

void clang_light_effect::make(float duration, vec3f pos, vec3f col, float rad)
{
    light nl;

    nl.set_pos({pos.v[0], pos.v[1], pos.v[2]});
    nl.set_col({col.v[0], col.v[1], col.v[2]});
    nl.set_radius(rad);

    l = light::add_light(&nl);

    duration_ms = duration;
}

void clang_light_effect::tick()
{
    float time = elapsed_time.getElapsedTime().asMicroseconds() / 1000.f;

    float done_frac = time / duration_ms;

    l->set_brightness(clamp(1.f - done_frac, 0.f, 1.f));

    if(time > duration_ms)
    {
        light::remove_light(l);

        finished = true;
    }
}

void particle_effect::tick()
{
    for(int i=0; i<effects.size(); i++)
    {
        effects[i]->tick();

        if(effects[i]->finished)
        {
            delete effects[i];

            effects.erase(effects.begin() + i);
            i--;
        }
    }
}
