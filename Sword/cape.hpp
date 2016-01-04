#ifndef CAPE_HPP_INCLUDED
#define CAPE_HPP_INCLUDED

#include <Boost/compute.hpp>
#include <vec/vec.hpp>
#include <sfml/system.hpp>

struct object_context_data;
struct object_context;

struct objects_container;

namespace compute = boost::compute;

struct fighter;

struct cape
{
    static void load_cape(objects_container* obj, int team);

    objects_container* model;

    compute::buffer in, out;
    int which;

    int width, height, depth;

    ///0 when alive, decreases with gravity on death
    float death_height_offset;
    bool hit_floor;
    sf::Clock timeout_time;

    cape(object_context& cpu, object_context_data& gpu);

    compute::buffer fighter_to_fixed(objects_container* l, objects_container* m, objects_container* r);

    compute::buffer fighter_to_fixed_vec(vec3f p1, vec3f p2, vec3f p3, vec3f rot);

    void tick(fighter* parent);

    void make_stable(fighter* parent);

    void load(int team);

private:
    bool loaded;
    object_context* cpu_context;
    object_context_data* gpu_context;
};

#endif // CAPE_HPP_INCLUDED
