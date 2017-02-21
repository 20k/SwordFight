#ifndef ASSET_MANAGER_HPP_INCLUDED
#define ASSET_MANAGER_HPP_INCLUDED

#include "../openclrenderer/proj.hpp"
#include "tinydir/tinydir.h"
#include "../imgui/imgui.h"
#include "../openclrenderer/util.hpp"
#include "../openclrenderer/vec.hpp"
#include "../sword/util.hpp"
#include "shared_mapeditor.hpp"

inline
void load_floor(objects_container* obj)
{
    texture* tex = obj->parent->tex_ctx.make_new();
    tex->set_create_colour(sf::Color(220, 220, 220), 128, 128);

    int dsize = 1;

    objects_container temp_obj;
    temp_obj.parent = obj->parent;

    obj_rect(&temp_obj, *tex, (cl_float2){dsize, dsize});

    ///subobject position set by obj->set_pos in load_object_cube
    obj->objs.push_back(temp_obj.objs[0]);

    obj->isloaded = true;
}

inline
void obj_load_blank_tex(objects_container* obj)
{
    obj_load(obj);

    object_context& ctx = *obj->parent;

    for(object& o : obj->objs)
    {
        int tid = o.tid;

        texture* tex = ctx.tex_ctx.id_to_tex(tid);

        if(tex != nullptr)
        {
            tex->set_create_colour(sf::Color(200, 200, 200), 128, 128);
        }
    }
}

struct asset
{
    std::string asset_name;
    std::string asset_toplevel_directory; ///used for categorisation
    std::string asset_path; ///including name
    objects_container* loaded_asset = nullptr;

    void set_asset(const std::string& path, const std::string& name);

    void load_object(object_context& ctx);
};

//col = {0.2705 * 255.f, 0.407843 * 255.f, 0.4 * 255.f};
#if 0
inline
void modify_texture_colour_dynamic(object_context& ctx)
{
    objects_container* found_object = nullptr;

    for(objects_container* c : ctx.containers)
    {
        if(c->file.find("naturePack_007") != std::string::npos)
        {
            found_object = c;
            break;
        }
    }

    if(found_object == nullptr)
    {
        lg::log("NONE FOUND ERIROP");
        return;
    }

    int green_tid = found_object->objs[0].tid;
    int brown_tid = found_object->objs[1].tid;

    texture* green_tex = ctx.tex_ctx.id_to_tex(green_tid);
    texture* brown_tex = ctx.tex_ctx.id_to_tex(brown_tid);

    green_tex->unload();
    green_tex->set_load_from_other_texture(brown_tex);

    ctx.build(true);

    /*texture* tex = ctx.tex_ctx.id_to_tex(tid);

    ///should probably have a texture is dirty flag
    tex->unload();

    vec3f col = {0.2705 * 255.f, 0.407843 * 255.f, 0.4 * 255.f};

    ///Hmm. This wont be loaded yet. I don't want it to be that you have to manually load it
    ///as the deferred loading etc is nice. But then... I guess its impossible to do nicely
    tex->set_create_colour(sf::Color(col.x(), col.y(), col.z()), 32, 32);

    ctx.build(true);*/
}
#endif

std::vector<objects_container*> load_level_from_file(object_context& ctx, const std::string& file);
void load_floor_from_file(objects_container* obj, const std::string& file);

struct asset_manager
{
    bool any_loaded = false;

    std::vector<asset> assets;

    objects_container* last_hovered_object = nullptr;

    int grid_size = 40;
    bool use_grid = false;

    cl_float4 grid_lock(cl_float4 in);

    vec3f quant(vec3f in, float gsize);

    void populate(const std::string& asset_dir);

    void load_object_all(object_context& ctx);

    void position_object(objects_container* obj);

    void position();

    void scale();

    void patch_textures();
    void set_last_hovered(objects_container* c);

    void do_ui();

    void do_grid_ui();

    float cdx = 0;
    float cdy = 0;

    void do_interactivity(engine& window);

    void do_save_ui(object_context& ctx, std::string file, objects_container* floor);
    void save_objects(object_context& ctx, std::string file);

    void save_terrain(std::string file, objects_container* floor);

    ///this is what we need to pull out
    void load(object_context& ctx, std::string file, objects_container* floor);

    void do_dyn_scale(engine& window);

    void do_object_rotation(engine& window);

    void do_reset();

    std::vector<objects_container*> paste_asset_stack;

    void add_to_paste_stack(objects_container* st);

    void clear_asset_stack();

    void remove_from_asset_stack(objects_container* st);

    void do_paste_stack_ui();

    bool hide_c = false;
    bool textured_c = false;

    void do_level_ui(objects_container* c, texture* white, texture* regular);

    void check_copy_stack();

    void check_paste_stack(object_context& ctx, engine& window);

    void del(objects_container* st);
};

#endif // ASSET_MANAGER_HPP_INCLUDED
