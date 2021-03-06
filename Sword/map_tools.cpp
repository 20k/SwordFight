#include "../openclrenderer/objects_container.hpp"
#include "object_cube.hpp"
#include "map_tools.hpp"
#include "../openclrenderer/vec.hpp"
#include "../openclrenderer/obj_load.hpp"
#include "shared_mapeditor.hpp"

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

void polygonal_world_map::init(object_context& ctx, const std::string& file)
{
    name = file;

    level_floor = ctx.make_new();

    float scale_mult = 1.2f;

    other_objects = load_level_from_file(ctx, file, scale_mult);

    load_floor_from_file(level_floor, file, scale_mult);

    level_floor->set_is_static(true);

    for(auto& i : other_objects)
    {
        i->set_is_static(true);

        i->offset_pos({0, FLOOR_CONST, 0});
    }

    level_floor->offset_pos({0, FLOOR_CONST, 0});

    floor_collision_handler.set_obj(level_floor);
    floor_collision_handler.construct_collision_map(100);
}

float polygonal_world_map::get_ground_height(vec3f pos, vec3f* optional_normal)
{
    return floor_collision_handler.get_heightmap_of_world_pos(pos, optional_normal);
}

std::function<void(objects_container*)> world_map::get_load_func()
{
    if(width == 0 || height == 0)
        throw std::runtime_error("wildly invalid map, what do");

    return std::bind(load_map_cube, std::placeholders::_1, map_def, width, height);
}

void world_collision_handler::set_map(polygonal_world_map& _map)
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

            ///so the first 6 are always the lower section of the cube away from end, and the last 6 are towards the end
            load_object_cube(&temp_obj, world_pos_start, world_pos_end, game_map::scale/2, "./Res/gray.png");

            ///Ok. So the first two triangles are the base of the cube. We never need this ever
            temp_obj.objs[0].tri_list.erase(temp_obj.objs[0].tri_list.begin() + 0);
            temp_obj.objs[0].tri_list.erase(temp_obj.objs[0].tri_list.begin() + 0);

            if(get_map_loc(map_def, {x, y}, {width, height}) == 0)
            {
                for(int i=0; i<8; i++)
                {
                    temp_obj.objs[0].tri_list.pop_back();
                }
            }

            temp_obj.objs[0].tri_num = temp_obj.objs[0].tri_list.size();

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
    /*for(int i=0; i<map_def.size(); i++)
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
    obj->isloaded = true;*/

    ///0.2ms optimisation here ish
    object base;
    base.isloaded = true;

    for(int i=0; i<map_def.size(); i++)
    {
        objects_container temp_obj;
        temp_obj.parent = obj->parent;
        load_map(&temp_obj, map_def[i], width, height);

        float len = get_cube_half_length(width) * game_map::scale;

        vec3f rot = map_namespace::map_cube_rotations[i];
        ///rotating them isn't rotating the whole thing
        ///bloody non composing objects ;_;
        ///SHOULD HAVE USED MATRICES
        ///I REGRET EVERYTHING

        for(auto& o : temp_obj.objs)
        {
            vec3f local_offset = (vec3f){o.pos.x, o.pos.y, o.pos.z} - (vec3f){width/2, 0, width/2};

            vec3f local_rotated_offset = local_offset.rot({0,len,0}, rot) + (vec3f){0,len,0};

            cl_float4 pos = {local_rotated_offset.v[0], local_rotated_offset.v[1], local_rotated_offset.v[2]};

            //o.set_pos(pos);
            //o.set_rot({rot.v[0], rot.v[1], rot.v[2]});

            //obj->objs.push_back(o);

            for(triangle& t : o.tri_list)
            {
                for(vertex& v : t.vertices)
                {
                    vec3f lpos = {v.get_pos().x, v.get_pos().y, v.get_pos().z};
                    vec3f lnormal = {v.get_normal().x, v.get_normal().y, v.get_normal().z};

                    lpos = lpos.rot({0,0,0}, rot);
                    lpos = lpos + local_rotated_offset;

                    lnormal = lnormal.rot(0.f, rot);

                    lpos = round(lpos);

                    v.set_pos({lpos.x(), lpos.y(), lpos.z()});
                    v.set_normal({lnormal.x(), lnormal.y(), lnormal.z()});
                }

                base.tid = o.tid;
                base.tri_list.push_back(t);
            }
        }
    }

    base.tri_num = base.tri_list.size();

    obj->objs.push_back(base);

    obj->isloaded = true;
}

bool world_map::is_wall(vec2f world_pos, const std::vector<int>& map_def, int width, int height)
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
bool world_map::rectangle_in_wall(vec2f centre, vec2f dim, const std::vector<int>& map_def, int width, int height)
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

bool world_map::rectangle_in_wall(vec2f centre, vec2f dim, world_collision_handler* st)
{
    if(st == nullptr)
        return false;

    int face = 0;

    return rectangle_in_wall(centre, dim, map_def[face], width, height);
}

bool polygonal_world_map::is_wall(vec3f world_pos)
{
    return false;
}

bool polygonal_world_map::rectangle_in_wall(vec2f centre, vec2f dim, world_collision_handler* st)
{
    return false;
}
