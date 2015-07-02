#include "physics.hpp"

#include <iostream>

#include "../openclrenderer/objects_container.hpp"

#include <float.h>

#include "fighter.hpp"

bool physobj::within(vec3f pos)
{
    for(int i=0; i<3; i++)
        if(pos.v[i] < min_pos.v[i] + obj->pos.s[i]) ///lower than minimum point, not inside cube
            return false;

    for(int i=0; i<3; i++)
        if(pos.v[i] >= max_pos.v[i] + obj->pos.s[i]) ///greater than maximum point, not inside cube
            return false;

    ///must be within cube
    return true;
}

void physics::load()
{

}

void physics::add_objects_container(objects_container* _obj, part* _p, int _team, fighter* _parent)
{
    physobj p;
    p.obj = _obj;
    p.team = _team;
    p.p = _p;
    p.parent = _parent;

    vec3f tl = {FLT_MAX, FLT_MAX, FLT_MAX}, br = {FLT_MIN, FLT_MIN, FLT_MIN};

    for(object& o : _obj->objs)
    {
        for(triangle& t : o.tri_list)
        {
            for(vertex& v : t.vertices)
            {
                vec3f pos = {v.get_pos().x, v.get_pos().y, v.get_pos().z};

                tl = min(pos, tl);
                br = max(pos, br);
            }
        }
    }

    p.min_pos = tl;
    p.max_pos = br;

    bodies.push_back(p);
}

///this entire method seems like a hacky bunch of shite
int physics::sword_collides(sword& w, fighter* my_parent)
{
    //vec3f s_rot = w.rot;
    vec3f s_pos = xyz_to_vec(w.model.pos);

    vec3f dir = (vec3f){0, 1, 0}.rot({0,0,0}, xyz_to_vec(w.model.rot));
    dir = dir.norm();

    ///sword height FROM HANDLE FOCUS GRIP POINT
    float sword_height = FLT_MIN;

    for(triangle& t : w.model.objs[0].tri_list)
    {
        for(vertex& v : t.vertices)
        {
            vec3f pos = xyz_to_vec(v.get_pos());

            if(pos.v[1] > sword_height)
                sword_height = pos.v[1];
        }
    }

    float step = 1.f;

    for(float t = 0; t < sword_height; t += step)
    {
        vec3f pos = s_pos + t * dir;

        for(int i=0; i<bodies.size(); i++)
        {
            if(bodies[i].team != w.team && bodies[i].p->hp >= 0 && bodies[i].within(pos))
            {
                bodypart_t type = (bodypart_t)(i % bodypart::COUNT);

                fighter* them = bodies[i].parent;

                ///if there is no current lhand or rhand movement in the map
                ///movement default constructs does_block to false
                ///this seems a bit.... hacky

                ///this is THEIR current action
                movement m1 = them->action_map[bodypart::LHAND];
                movement m2 = them->action_map[bodypart::RHAND];

                movement y1 = my_parent->action_map[bodypart::LHAND];
                movement y2 = my_parent->action_map[bodypart::RHAND];

                ///recoil. Sword collides is only called for attacks that damage, so therefore this is fine
                if(m1.does(mov::BLOCKING) || m2.does(mov::BLOCKING) || them->net.is_blocking)
                {
                    my_parent->recoil();

                    return -1;
                }

                ///we still want to recoil even if we hit THEIR hand, but no damage
                if(type == bodypart::LHAND || type == bodypart::RHAND)
                    continue;

                return i;
            }
        }
    }

    //vec3f end = s_pos + sword_height*dir;

    //printf("%f %f %f\n", s_pos.v[0], s_pos.v[1], s_pos.v[2]);

    return -1;
}

/*void physics::tick()
{

}

vec3f physics::get_pos()
{

};*/
