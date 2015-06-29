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

void physics::add_objects_container(objects_container* obj, int _team)
{
    physobj p;
    p.obj = obj;
    p.team = _team;

    vec3f tl = {FLT_MAX, FLT_MAX, FLT_MAX}, br = {FLT_MIN, FLT_MIN, FLT_MIN};

    for(object& o : obj->objs)
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

int physics::sword_collides(sword& w)
{
    vec3f s_rot = w.rot;
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

    /*for(auto& i : bodies)
    {
        printf("%f %f %f\n", i.obj->pos.s[0], i.obj->pos.s[1], i.obj->pos.s[2]);
    }*/

    float step = 1.f;

    for(float t = 0; t < sword_height; t += step)
    {
        vec3f pos = s_pos + t * dir;

        for(int i=0; i<bodies.size(); i++)
        {
            if(bodies[i].team != w.team && bodies[i].within(pos))
            {
                return i;
            }

            if(bodies[i].team == w.team)
                exit(1);
        }
    }

    //vec3f end = s_pos + sword_height*dir;

    //printf("%f %f %f\n", s_pos.v[0], s_pos.v[1], s_pos.v[2]);

    return -1;

    //vec3d dir = {0, }
}

void physics::tick()
{

}

vec3f physics::get_pos()
{

}
