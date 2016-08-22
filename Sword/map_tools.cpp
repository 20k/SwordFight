#include "../openclrenderer/objects_container.hpp"
#include "object_cube.hpp"
#include "map_tools.hpp"
#include "../openclrenderer/vec.hpp"
#include "../openclrenderer/obj_load.hpp"

/*void world_map::init(const std::vector<int>& _map, int w, int h)
{
    map_def = _map;
    width = w;
    height = h;
}*/

void world_map::init(int map_id)
{
    map_def = game_map::cube_map_list[map_id];
    width = game_map::map_dims[map_id].v[0];
    height = game_map::map_dims[map_id].v[1];
}

std::function<void(objects_container*)> world_map::get_load_func()
{
    if(width == 0 || height == 0)
        throw std::runtime_error("wildly invalid map, what do");

    return std::bind(load_map_cube, std::placeholders::_1, map_def, width, height);
}

int world_map::get_real_dim()
{
    int largest = std::max(width, height);

    return largest * game_map::scale;
}

void gameplay_state::set_map(world_map& _map)
{
    current_map = _map;
}

int get_map_loc(const std::vector<int>& map_def, vec2i pos, vec2i dim)
{
    int world_h = map_def[pos.v[1] * dim.v[0] + pos.v[0]];

    //if(world_h == map_namespace::R || world_h == map_namespace::B)
    //    world_h = 0;

    if(world_h < 0)
        world_h = 0;

    return world_h;
}

vec3f get_world_loc(const std::vector<int>& map_def, vec2i pos, vec2i dim)
{
    int height = get_map_loc(map_def, pos, dim);

    vec2f centre = {dim.v[0]/2.f, dim.v[1]/2.f};

    vec2f w2d = (vec2f){pos.v[0], pos.v[1]} - centre;

    vec3f world_pos_top = {w2d.v[0], height, w2d.v[1]};

    world_pos_top = world_pos_top + (vec3f){0.5f, 0.f, 0.5f};

    return world_pos_top * game_map::scale;
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
    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            vec3f world_pos_end = get_world_loc(map_def, {x, y}, {width, height});
            vec3f world_pos_start = world_pos_end;

            world_pos_start.v[1] = -1;

            objects_container temp_obj;
            temp_obj.parent = obj->parent;

            load_object_cube(&temp_obj, world_pos_start, world_pos_end, game_map::scale/2, "./Res/gray.png");

            ///subobject position set by obj->set_pos in load_object_cube
            obj->objs.push_back(temp_obj.objs[0]);
        }
    }

    obj->independent_subobjects = true;
    obj->isloaded = true;
}

///width must equal height :{
/*vec3f get_map_pivot(int dim)
{
    vec2f pivot_3d = {dim/2, dim/2, dim/2};
}*/

///useful brain conversion function
float get_cube_half_length(int width)
{
    return width/2.f;
}

void load_map_cube(objects_container* obj, const std::vector<std::vector<int>>& map_def, int width, int height)
{
    for(int i=0; i<map_def.size(); i++)
    {
        objects_container temp_obj;
        temp_obj.parent = obj->parent;

        load_map(&temp_obj, map_def[i], width, height);

        float len = get_cube_half_length(width) * game_map::scale;

        vec3f rot = map_namespace::map_cube_rotations[i];
        //vec3f offset = (vec3f){0, -len, 0}.rot({0,0,0}, rot);

        ///rotating them isn't rotating the whole thing
        ///bloody non composing objects ;_;
        ///SHOULD HAVE USED MATRICES
        ///I REGRET EVERYTHING

        for(auto& o : temp_obj.objs)
        {
            vec3f local_offset = (vec3f){o.pos.x, o.pos.y, o.pos.z} - (vec3f){width/2, 0, width/2};

            vec3f local_rotated_offset = local_offset.rot({0,len,0}, rot) + (vec3f){0,len,0};

            //local_rotated_offset = round(local_rotated_offset);

            cl_float4 pos = {local_rotated_offset.v[0], local_rotated_offset.v[1], local_rotated_offset.v[2]};

            o.set_pos(pos);
            o.set_rot({rot.v[0], rot.v[1], rot.v[2]});

            obj->objs.push_back(o);
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
        return false;

    ///if we're bigger than 0, we're a wall. Otherwise, not
    return map_def[imap.v[1]*width + imap.v[0]] > 0;
}

///so, it looks like we basically just need to replace this function, and the one above, to fix 3d collision detection
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

    int face = st->current_map.cube_info->face;

    return rectangle_in_wall(centre, dim, st->current_map.map_def[face], st->current_map.width, st->current_map.height);
}
