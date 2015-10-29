#ifndef CAPE_HPP_INCLUDED
#define CAPE_HPP_INCLUDED

#include <Boost/compute.hpp>
#include "vec.hpp"

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

    cape(object_context& cpu, object_context_data& gpu);

    compute::buffer fighter_to_fixed(objects_container* l, objects_container* m, objects_container* r);

    void tick(objects_container* l, objects_container* m, objects_container* r, fighter* parent);

    void make_stable(fighter* parent);

    void load(int team);

private:
    bool loaded;
    object_context* cpu_context;
    object_context_data* gpu_context;
};

#endif // CAPE_HPP_INCLUDED
