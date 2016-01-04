#ifndef MAP_TOOLS_HPP_INCLUDED
#define MAP_TOOLS_HPP_INCLUDED

#include <vector>

struct objects_container;

//#include "../openclrenderer/objects_conta

static std::vector<int>
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
};

static std::vector<int>
map_one
{
    1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,1,
    1,0,0,1,1,1,1,1,0,0,1,
    1,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,1,0,1,0,0,0,1,
    1,0,0,0,1,0,1,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,1,
    1,0,0,1,1,1,1,1,0,0,1,
    1,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,
};

namespace game_map
{
    static float scale = 1000.f;
    //static float floor_const =
    #define FLOOR_CONST (bodypart::default_position[bodypart::LFOOT].v[1] - bodypart::scale/3)
}

struct world_map
{
    int width = 0, height = 0;
    std::vector<int> map_def;

    void init(const std::vector<int>& _map, int w, int h);

    std::function<void(objects_container*)> get_load_func();
};

struct gameplay_state
{
    world_map current_map;

    void set_map(world_map& m);
};

///what we really want is a map class
///that returns a load function
///or a loaded object

void load_map(objects_container* obj, const std::vector<int>& map_def, int width, int height);

///xz, where z is y in 2d space
bool is_wall(vec2f world_pos, const std::vector<int>& map_def, int width, int height);
bool rectangle_in_wall(vec2f centre, vec2f dim, const std::vector<int>& map_def, int width, int height);
bool rectangle_in_wall(vec2f centre, vec2f dim, gameplay_state* st);


#endif // MAP_TOOLS_HPP_INCLUDED
