#ifndef FLAT_POLY_COLLISION_HANDLER_HPP_INCLUDED
#define FLAT_POLY_COLLISION_HANDLER_HPP_INCLUDED

#include <vector>
#include <set>
#include <vec/vec.hpp>

struct objects_container;

struct flat_poly_collision_handler
{
    objects_container* obj = nullptr;
    std::set<int>* coords_to_indices; ///2d xz coordinate to list of triangle indices
    vec2f min_pos;
    vec2f max_pos;
    int number_of_cells_in_one_dimension;

    void set_obj(objects_container* o);

    void construct_collision_map(int pnumber_of_cells_in_one_dimension = 100);

    float get_heightmap_of_world_pos(vec3f pos);
};

#endif // FLAT_POLY_COLLISION_HANDLER_HPP_INCLUDED
