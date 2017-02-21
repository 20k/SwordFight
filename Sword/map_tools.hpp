#ifndef MAP_TOOLS_HPP_INCLUDED
#define MAP_TOOLS_HPP_INCLUDED

#include <vector>

#include "../openclrenderer/logging.hpp"
#include "fighter.hpp"

struct objects_container;

//#include "../openclrenderer/objects_conta

#include "../sword_server/game_server/game_modes.hpp"

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

struct world_map
{
    int width = 0, height = 0;
    std::vector<std::vector<int>> map_def;

    //void init(const std::vector<int>& _map, int w, int h);
    void init(int map_id);

    std::function<void(objects_container*)> get_load_func();
};

struct polygonal_world_map
{
    objects_container* level_floor;
    std::vector<objects_container*> other_objects;

    std::string name;

    void init(object_context& ctx, const std::string& file);
};

struct game_mode_exec;

namespace menu_state
{
    enum menu_state
    {
        MENU,
        GAME,
        COUNT
    };
}

typedef menu_state::menu_state menu_t;


///make gamemode implementation a polymorphic struct
struct gameplay_state
{
    world_map current_map;
    game_mode_t current_mode = game_mode::FIRST_TO_X;
    game_mode_exec* mode_exec = nullptr;

    menu_t current_menu;

    void set_map(world_map& m);
    void start_game_mode();

    bool should_end_game_mode();
    void end_game_mode();
};

///what we really want is a map class
///that returns a load function
///or a loaded object

void load_map(objects_container* obj, const std::vector<int>& map_def, int width, int height);

void load_map_cube(objects_container* obj, const std::vector<std::vector<int>>& map_def, int width, int height);

///xz, where z is y in 2d space
bool is_wall(vec2f world_pos, const std::vector<int>& map_def, int width, int height);
///ok, update me to work with new cubemap system
bool rectangle_in_wall(vec2f centre, vec2f dim, const std::vector<int>& map_def, int width, int height);
bool rectangle_in_wall(vec2f centre, vec2f dim, gameplay_state* st);


#endif // MAP_TOOLS_HPP_INCLUDED
