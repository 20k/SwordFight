#ifndef WORLD_MANAGER_HPP_INCLUDED
#define WORLD_MANAGER_HPP_INCLUDED

#include <vec/vec.hpp>
#include <vector>

#include <SFML/Graphics.hpp>

#include <random>

struct state;

struct rect
{
    vec2f tl, dim;
};

struct game_entity;

struct world_manager
{
    std::vector<rect> rooms;
    std::vector<rect> rooms_without_corridors;

    std::minstd_rand level_rng;

    vec2f minvec, maxvec;

    bool* collision_map;
    int width, height;

    bool is_open(int x, int y);
    void set_wall_state(int x, int y, bool is_open);
    bool entity_in_wall(vec2f world_pos, vec2f dim);

    bool within_any_room(vec2f pos);

    void spawn_level(std::vector<game_entity*>& entities);

    ///if I am in a wall... what do?
    ///return the collision in world coordinates, not collision coordinates
    ///I will forget this so thats the bug
    vec2f raycast(vec2f world_pos, vec2f dir);

    vec2i world_to_collision(vec2f pos);
    vec2f collision_to_world(vec2f cpos);

    void generate_level(int seed);

    void draw_rooms(state& s);
};

#endif // WORLD_MANAGER_HPP_INCLUDED
