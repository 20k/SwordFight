#include "../openclrenderer/objects_container.hpp"
#include "object_cube.hpp"
#include "map_tools.hpp"

void world_map::init(const std::vector<int>& _map, int w, int h)
{
    map_def = _map;
    width = w;
    height = h;
}

void world_map::init(int map_id)
{
    map_def = game_map::map_list[map_id];
    width = game_map::map_dims[map_id].v[0];
    height = game_map::map_dims[map_id].v[1];
}

std::function<void(objects_container*)> world_map::get_load_func()
{
    if(width == 0 || height == 0)
        throw std::runtime_error("wildly invalid map, what do");

    return std::bind(load_map, std::placeholders::_1, map_def, width, height);
}

void gameplay_state::set_map(world_map& _map)
{
    current_map = _map;
}

///doesnt' work because each addition to the container
///stomps over the position of the last
///we need to take the position and actually statically modify
///the triangle elements by that much
///rip
///maybe make a temp objects container and do that
///do we want a static merge function? Combine nearby points to fix holes
///works now, but the above comments are still relevant
void load_map(objects_container* obj, const std::vector<int>& map_def, int width, int height)
{
    vec2f centre = {width/2.f, height/2.f};

    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            vec2f w2d = (vec2f){x, y} - centre;

            int world_h = map_def[y*width + x];

            if(world_h == map_namespace::R || world_h == map_namespace::B)
            {
                world_h = 0;
            }

            ///-1 so that -1 -> 0 is floor
            ///and then 0 height is floor height
            ///which means that pos.v[1] can be set to the players feet
            vec3f world_pos_start = {w2d.v[0], -1, w2d.v[1]};
            vec3f world_pos_end = {w2d.v[0], world_h, w2d.v[1]};

            float scale = game_map::scale;

            world_pos_start = world_pos_start + (vec3f){0.5f, 0.f, 0.5f};
            world_pos_end = world_pos_end + (vec3f){0.5f, 0.f, 0.5f};

            objects_container temp_obj;
            temp_obj.parent = obj->parent;

            load_object_cube(&temp_obj, world_pos_start * scale, world_pos_end * scale, scale/2, "./Res/gray.png");

            ///subobject position set by obj->set_pos in load_object_cube
            obj->objs.push_back(temp_obj.objs[0]);
        }
    }

    obj->independent_subobjects = true;
    obj->isloaded = true;
}

bool is_wall(vec2f world_pos, const std::vector<int>& map_def, int width, int height)
{
    ///so world pos has been scaled
    ///and the level geometry has been moved by 0.5 to the right
    ///and then this is also set to the centre helpfully

    vec2f centre = {width/2.f, height/2.f};

    vec2f map_scale = world_pos / game_map::scale;

    map_scale = map_scale - 0.5f;

    map_scale = round(map_scale + centre);

    vec2i imap = {map_scale.v[0], map_scale.v[1]};

    //printf("Map: %i %i\n", imap.v[0], imap.v[1]);

    if(imap.v[0] < 0 || imap.v[0] >= width || imap.v[1] < 0 || imap.v[1] >= height)
        return true;

    ///if we're bigger than 0, we're a wall. Otherwise, not
    return map_def[imap.v[1]*width + imap.v[0]] > 0;
}

bool rectangle_in_wall(vec2f centre, vec2f dim, const std::vector<int>& map_def, int width, int height)
{
    vec2f hd = dim/2.f;

    vec2f posl[4];

    posl[0] = centre - hd;
    posl[1] = centre + (vec2f){hd.v[0], -hd.v[1]};
    posl[2] = centre + (vec2f){-hd.v[0], hd.v[1]};
    posl[3] = centre + hd;

    for(int i=0; i<4; i++)
    {
        if(is_wall(posl[i], map_def, width, height))
            return true;
    }

    return false;
}

bool rectangle_in_wall(vec2f centre, vec2f dim, gameplay_state* st)
{
    if(st == nullptr)
        return false;

    return rectangle_in_wall(centre, dim, st->current_map.map_def, st->current_map.width, st->current_map.height);
}
