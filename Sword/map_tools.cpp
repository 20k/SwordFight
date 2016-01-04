#include "../openclrenderer/objects_container.hpp"
#include "object_cube.hpp"

///doesnt' work because each addition to the container
///stomps over the position of the last
///we need to take the position and actually statically modify
///the triangle elements by that much
///rip
///maybe make a temp objects container and do that
///do we want a static merge function? Combine nearby points to fix holes
///works now, but the above comments are still relevant
void load_map(objects_container* obj, int* map_def, int width, int height)
{
    vec2f centre = {width/2.f, height/2.f};

    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            vec2f w2d = (vec2f){x, y} - centre;

            int world_h = map_def[y*width + x];

            ///-1 so that -1 -> 0 is floor
            ///and then 0 height is floor height
            ///which means that pos.v[1] can be set to the players feet
            vec3f world_pos_start = {w2d.v[0], -1, w2d.v[1]};
            vec3f world_pos_end = {w2d.v[0], world_h, w2d.v[1]};

            float scale = 1000.f;

            world_pos_start = world_pos_start + (vec3f){1/2.f, 0.f, 1/2.f};
            world_pos_end = world_pos_end + (vec3f){1/2.f, 0.f, 1/2.f};

            objects_container temp_obj;

            load_object_cube(&temp_obj, world_pos_start * scale, world_pos_end * scale, scale/2, "./Res/gray.png");

            ///subobject position set by obj->set_pos in load_object_cube
            obj->objs.push_back(temp_obj.objs[0]);
        }
    }

    obj->isloaded = true;
}
