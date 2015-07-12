#ifndef PHYSICS_HPP_INCLUDED
#define PHYSICS_HPP_INCLUDED

#include "vec.hpp"
#include "bbox.hpp"

struct part;
struct sword;
struct fighter;

struct objects_container;

struct physobj
{
    objects_container* obj;
    part* p;
    fighter* parent;

    vec3f min_pos, max_pos;

    ///min_pos - fudge, max_pos + fudge
    ///pos is worldspace
    bool within(vec3f pos, vec3f fudge = {0,0,0});
};


bbox get_bbox(objects_container* obj);

struct physics
{
    std::vector<physobj> bodies;

    void add_objects_container(objects_container* o, part* p, fighter* parent);

    void load();

    //void tick();

    int sword_collides(sword& w, fighter* parent, vec3f sword_move_dir, bool is_player = false);

    //vec3f get_pos();
};

#endif // PHYSICS_HPP_INCLUDED
