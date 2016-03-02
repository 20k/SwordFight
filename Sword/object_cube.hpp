#ifndef OBJECT_CUBE_HPP_INCLUDED
#define OBJECT_CUBE_HPP_INCLUDED

#include <vec/vec.hpp>

struct texture;

struct objects_container;

void load_object_cube(objects_container* obj, vec3f start, vec3f fin, float size, std::string tex_name);
void load_object_cube_tex(objects_container* obj, vec3f start, vec3f fin, float size, texture& tex);

#endif // OBJECT_CUBE_HPP_INCLUDED
