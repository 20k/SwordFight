#include "particle_effect.hpp"
#include "../openclrenderer/objects_container.hpp"
#include "object_cube.hpp"

std::vector<effect*> particle_effect::effects;

void cube_effect::make(float duration, vec3f _pos, float _scale, int _team, int _num)
{
    objects.clear();

    duration_ms = duration;
    pos = _pos;
    scale = _scale;

    elapsed_time.restart();

    num = _num;

    for(int i=0; i<num; i++)
    {
        vec3f p1 = {0,0,0};

        float len = 10.f * randf<1, float>(2, 0.1);

        vec3f p2 = p1 + (vec3f){0, 0, len};

        std::string tex;

        if(_team == 0)
            tex = "./res/red.png";
        else
            tex = "./res/blue.png";

        objects_container o;
        o.set_load_func(std::bind(load_object_cube, std::placeholders::_1, p1, p2, len/2, tex));
        o.cache = false;

        vec3f rpos = (randf<3, float>() - 0.5f) * scale;

        vec3f lpos = pos + rpos;

        o.set_pos({lpos.v[0], lpos.v[1], lpos.v[2]});

        objects.push_back(o);
    }
}

void cube_effect::push()
{
    cube_effect* e = new cube_effect(*this);

    particle_effect::effects.push_back(e);

    for(auto& i : e->objects)
    {
        i.set_active(true);
    }
}

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
            vec3f dir = xyz_to_vec(o.pos) - centre;

            dir = dir.norm();

            float time_frac = time / e.duration_ms;

            time_frac = - time_frac * (time_frac - 2);

            vec3f pos = dir * time_frac * e.scale + centre;

            o.set_pos({pos.v[0], pos.v[1], pos.v[2]});
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
                objects_container& o = e.objects[j];

                o.set_pos({0, 0, -3000000});
            }

            if(remaining < 0)
            {
                for(auto& o : e.objects)
                {
                    o.set_pos({0, 0, -30000000});
                    o.g_flush_objects();
                    o.set_active(false);

                    finished = true;
                }

                //effects.erase(effects.begin() + i);
                //i--;
                //continue;
            }
        }

        for(auto& o : e.objects)
        {
            o.g_flush_objects();
        }
    }
}

void particle_effect::tick()
{
    //for(auto& i : effects)
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
