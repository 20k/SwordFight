#ifndef MAP_TOOLS_HPP_INCLUDED
#define MAP_TOOLS_HPP_INCLUDED

#include <vector>

#include "../openclrenderer/logging.hpp"
#include "fighter.hpp"

struct objects_container;

//#include "../openclrenderer/objects_conta

#include "../sword_server/game_server/game_modes.hpp"
#include "flat_poly_collision_handler.hpp"

/*static std::vector<int>
map_test =
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 1, 1, 1, 1, 1, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};*/

struct map_cube_info;
struct world_collision_handler;

struct world_map
{
    int width = 0, height = 0;
    std::vector<std::vector<int>> map_def;

    //void init(const std::vector<int>& _map, int w, int h);
    void init(int map_id);

    std::function<void(objects_container*)> get_load_func();

    ///move these into world map, make equivalent for polygonal world map
    ///xz, where z is y in 2d space
    bool is_wall(vec2f world_pos, const std::vector<int>& map_def, int width, int height);
    ///ok, update me to work with new cubemap system
    bool rectangle_in_wall(vec2f centre, vec2f dim, const std::vector<int>& map_def, int width, int height);
    bool rectangle_in_wall(vec2f centre, vec2f dim, world_collision_handler* collision_handler);
};

struct polygonal_world_map
{
    flat_poly_collision_handler floor_collision_handler;
    objects_container* level_floor;
    std::vector<objects_container*> other_objects;

    std::string name;

    void init(object_context& ctx, const std::string& file);

    bool is_wall(vec3f world_pos);

    bool rectangle_in_wall(vec2f centre, vec2f dim, world_collision_handler* collision_handler);
    float get_ground_height(vec3f pos, vec3f* optional_normal = nullptr);
};

struct world_collision_handler
{
    ///basically only used for collision
    polygonal_world_map current_map;

    void set_map(polygonal_world_map& m);
};

///what we really want is a map class
///that returns a load function
///or a loaded object

void load_map(objects_container* obj, const std::vector<int>& map_def, int width, int height);

void load_map_cube(objects_container* obj, const std::vector<std::vector<int>>& map_def, int width, int height);


#endif // MAP_TOOLS_HPP_INCLUDED
