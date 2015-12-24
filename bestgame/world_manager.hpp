#ifndef WORLD_MANAGER_HPP_INCLUDED
#define WORLD_MANAGER_HPP_INCLUDED

#include <vec/vec.hpp>
#include <vector>

#include <SFML/Graphics.hpp>

struct state;

struct rect
{
    vec2f tl, dim;
};

struct world_manager
{
    std::vector<rect> rooms;

    vec2f minvec, maxvec;

    bool* collision_map;
    int width, height;

    bool is_wall(int x, int y);
    void set_wall_state(int x, int y, bool is_open);

    bool within_any_room(vec2f pos);

    vec2i world_to_collision(vec2f pos);
    vec2f collision_to_world(vec2f cpos);

    void generate_level(int seed);

    void draw_rooms(state& s);
};

#endif // WORLD_MANAGER_HPP_INCLUDED
