#ifndef CAPE_HPP_INCLUDED
#define CAPE_HPP_INCLUDED

#include <Boost/compute.hpp>

struct objects_container;

namespace compute = boost::compute;

struct cape
{
    static void load_cape(objects_container* obj);

    objects_container* model;

    compute::buffer in, out;
    int which;

    int width, height, depth;

    cape();

    compute::buffer fighter_to_fixed(objects_container* l, objects_container* m, objects_container* r);

    void tick(objects_container* l, objects_container* m, objects_container* r);
};

#endif // CAPE_HPP_INCLUDED
