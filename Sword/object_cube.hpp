#ifndef OBJECT_CUBE_HPP_INCLUDED
#define OBJECT_CUBE_HPP_INCLUDED

#include "vec.hpp"

struct objects_container;

void load_object_cube(objects_container* obj, vec3f start, vec3f fin, float size, std::string tex_name);

#endif // OBJECT_CUBE_HPP_INCLUDED
