#ifndef PHYSICS_HPP_INCLUDED
#define PHYSICS_HPP_INCLUDED

#include "vec.hpp"

struct part;

struct sword;

struct objects_container;

struct physobj
{
    objects_container* obj;
    part* p;

    vec3f min_pos, max_pos;

    int team;

    bool within(vec3f pos);
};

struct physics
{
    std::vector<physobj> bodies;

    void add_objects_container(objects_container* o, part* p, int team);

    void load();

    void tick();

    int sword_collides(sword& w);

    vec3f get_pos();
};

#endif // PHYSICS_HPP_INCLUDED
