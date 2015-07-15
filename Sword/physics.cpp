#include "physics.hpp"

#include <iostream>

#include "../openclrenderer/objects_container.hpp"

#include <float.h>

#include "fighter.hpp"

#include <CL/cl.h> ///sigh
#include "../openclrenderer/engine.hpp"

#include "text.hpp"
#include "sound.hpp"
#include "../openclrenderer/network.hpp"

bool physobj::within(vec3f pos, vec3f fudge)
{
    for(int i=0; i<3; i++)
        if(pos.v[i] < min_pos.v[i] + obj->pos.s[i] - fudge.v[i]) ///lower than minimum point, not inside cube
            return false;

    for(int i=0; i<3; i++)
        if(pos.v[i] >= max_pos.v[i] + obj->pos.s[i] + fudge.v[i]) ///greater than maximum point, not inside cube
            return false;

    ///must be within cube
    return true;
}

void physics::load()
{

}

bbox get_bbox(objects_container* obj)
{
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

    return {tl, br};
}


void physics::add_objects_container(objects_container* _obj, part* _p, fighter* _parent)
{
    physobj p;
    p.obj = _obj;
    p.p = _p;
    p.parent = _parent;

    bbox b = get_bbox(_obj);

    p.min_pos = b.min;
    p.max_pos = b.max;

    bodies.push_back(p);
}

///this entire method seems like a hacky bunch of shite
///REMEMBER, DEAD OBJECTS ARE STILL CHECKED AGAINST. This is bad for performance (although who cares), but moreover its producing BUGS
///FIXME
int physics::sword_collides(sword& w, fighter* my_parent, vec3f sword_move_dir, bool is_player)
{
    if(my_parent->net.dead)
        return -1;

    if(w.model.isactive == false)
        return -1;

    //vec3f s_rot = w.rot;
    vec3f s_pos = xyz_to_vec(w.model.pos); ///this is its worldspace position

    vec3f dir = (vec3f){0, 1, 0}.rot({0,0,0}, xyz_to_vec(w.model.rot));
    dir = dir.norm();

    vec3f world_move_dir = sword_move_dir.rot({0,0,0}, xyz_to_vec(w.model.rot));

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

    bbox bound = w.bound;


    ///im a lazy programmer, and this is the thickness of the sword
    float min_dist = FLT_MAX;

    for(int i=0; i<3; i++)
    {
        float dist = (bound.max.v[i] - bound.min.v[i]);

        if(dist < min_dist)
            min_dist = dist;
    }

    /// we have one factor of /2 because we only want the half width,
    ///and another factor of two because a bbox check adds and subtracts this, we do not want to double add
    min_dist = min_dist / 4.f;

    ///we want to do the below check for all 4 corners and centre
    ///i am bad at programming
    ///if we fudge the cylinders aabb box hitbox by the width of the sword, that is the same as making the sword larger

    const float time = 500.f;


    bool caused_hand_recoil = false;
    cl_float4 hand_scr = {0,0,0,0};
    vec3f rel = {0,0,0};


    const float block_half_angle = M_PI/3;

    float step = 1.f;

    for(float t = 0; t < sword_height; t += step)
    {
        ///world pos
        vec3f pos = s_pos + t * dir;

        for(int i=0; i<bodies.size(); i++)
        {
            if(bodies[i].p->team != w.team && bodies[i].p->alive() && bodies[i].within(pos, {min_dist, min_dist, min_dist}) && bodies[i].p->model.isactive)
            {
                bodypart_t type = (bodypart_t)(i % bodypart::COUNT);

                fighter* them = bodies[i].parent;

                float arm_length = (them->rest_positions[bodypart::LUPPERARM] - them->rest_positions[bodypart::LHAND]).length();

                ///these are also very likely working as extrinsic rotations
                float their_x = sin(them->look_displacement.v[1] / arm_length);
                float their_y = them->parts[bodypart::BODY].model.rot.s[1];

                ///this is very likely correct!
                vec3f rotated_sword_dir = sword_move_dir.rot({0,0,0}, my_parent->rot);

                ///its - because the up angle for look is positive, and the down is negative
                ///but adding a negative would make it go up, and adding a positive would make it look down
                ///bad! so we have to subtract to cancel that shit right out
                ///this is probably correct too!

                ///rotations must be separate because they  dont work otherwise
                ///this one is local
                vec3f t_look = (vec3f){0, 0, -1}.rot({0,0,0}, -(vec3f){their_x, 0.f, 0.f});

                ///this one transforms the rotation into global rotation space
                t_look = t_look.rot({0,0,0}, xyz_to_vec(them->parts[bodypart::BODY].model.rot));

                ///angle between look and sword direction
                ///we want the opposite direction for one of these components
                ///as the peon is look AT the attack direction, not along it
                float angle = dot(t_look.norm(), -rotated_sword_dir.norm());
                angle = acos(angle);

                bool can_block = angle < block_half_angle && angle >= -block_half_angle;

                ///if there is no current lhand or rhand movement in the map
                ///movement default constructs does_block to false
                ///this seems a bit.... hacky

                //rel = pos - xyz_to_vec(my_parent->parts[bodypart::BODY].model.pos);
                //rel = rel.back_rot({0,0,0}, xyz_to_vec(my_parent->parts[bodypart::BODY].model.rot));

                rel = pos;

                cl_float4 scr;

                if(is_player)
                    scr = engine::project({pos.v[0], pos.v[1], pos.v[2]});

                //printf("%i\n", them->net.is_blocking);

                ///this is THEIR current action
                movement m1 = them->action_map[bodypart::LHAND];
                movement m2 = them->action_map[bodypart::RHAND];

                movement y1 = my_parent->action_map[bodypart::LHAND];
                movement y2 = my_parent->action_map[bodypart::RHAND];

                ///recoil. Sword collides is only called for attacks that damage, so therefore this is fine
                ///blocking recoil IS handled over the network currently
                if((m1.does(mov::BLOCKING) || m2.does(mov::BLOCKING) || them->net.is_blocking) && can_block)
                {
                    if(is_player)
                        text::add("Clang!", time, {scr.x, scr.y});
                    else
                        text::add_random("Clang!", time);

                    sound::add(1, rel);

                    my_parent->recoil();

                    return -1;
                }

                ///we still want to recoil even if we hit THEIR hand, but no damage
                ///this doesn't get networked...?
                if((type == bodypart::LHAND || type == bodypart::RHAND))
                {
                    if(m1.does(mov::WINDUP) || m2.does(mov::WINDUP))
                    {
                        hand_scr = scr;

                        caused_hand_recoil = true;

                        ///want to network them recoiling
                        them->net.recoil = 1;
                        network::host_update(&them->net.recoil);

                        them->cancel_hands(); ///will recoil
                    }

                    continue;
                }

                if(is_player)
                    text::add("Hrrk!", time, {scr.x, scr.y});
                else
                    text::add_random(std::string("Crikey!") + " My " + bodypart::ui_names[i % bodypart::COUNT] + "!", time);

                sound::add(0, rel);

                return i;
            }
        }
    }

    ///did not do a real hit
    if(caused_hand_recoil)
    {
        if(is_player)
            text::add("Smack!", time, {hand_scr.x, hand_scr.y});
        else
            text::add_random("MY HAND!", time);

        sound::add(0, rel);
    }

    //vec3f end = s_pos + sword_height*dir;

    //printf("%f %f %f\n", s_pos.v[0], s_pos.v[1], s_pos.v[2]);

    return -1;
}
