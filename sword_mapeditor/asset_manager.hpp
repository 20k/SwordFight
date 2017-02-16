#ifndef ASSET_MANAGER_HPP_INCLUDED
#define ASSET_MANAGER_HPP_INCLUDED

#include "../openclrenderer/proj.hpp"
#include "tinydir/tinydir.h"
#include "../imgui/imgui.h"
#include "../openclrenderer/util.hpp"
#include "../openclrenderer/vec.hpp"
#include "../sword/util.hpp"

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

    void set_asset(const std::string& path, const std::string& name)
    {
        asset_path = path;
        asset_name = name;

        size_t first_split = path.find_first_of('/');
        size_t second_split = path.find_first_of('/', first_split + 1);

        asset_toplevel_directory = path.substr(first_split + 1, second_split - first_split - 1);

        //std::cout << asset_toplevel_directory << std::endl;
    }

    void load_object(object_context& ctx)
    {
        loaded_asset = ctx.make_new();

        loaded_asset->set_file(asset_path);
        loaded_asset->set_active(true);
    }
};

//col = {0.2705 * 255.f, 0.407843 * 255.f, 0.4 * 255.f};
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

struct asset_manager
{
    bool any_loaded = false;

    std::vector<asset> assets;

    objects_container* last_hovered_object = nullptr;

    int grid_size = 40;
    bool use_grid = false;

    cl_float4 grid_lock(cl_float4 in)
    {
        if(!use_grid)
            return in;

        vec3f v = {in.x, in.y, in.z};

        v = v / grid_size;

        v = round(v);

        v = v * grid_size;

        return {v.x(), v.y(), v.z()};
    }

    vec3f quant(vec3f in, float gsize)
    {
        in = in / gsize;

        in = round(in);

        in = in * gsize;

        return in;
    }

    void populate(const std::string& asset_dir)
    {
        tinydir_dir dir;

        tinydir_open_sorted(&dir, asset_dir.c_str());

        for(int i=0; i<dir.n_files; i++)
        {
            tinydir_file file;

            tinydir_readfile_n(&dir, &file, i);

            std::string fname = file.name;

            if(file.is_dir && fname != "." && fname != "..")
            {
                populate(file.path);
            }
            else
            {
                size_t loc = fname.find(".obj");

                if(loc == std::string::npos || loc != fname.size() - strlen(".obj"))
                {
                    continue;
                }

                asset a;
                a.set_asset(file.path, file.name);

                assets.push_back(a);
            }
        }

        tinydir_close(&dir);
    }

    void load_object_all(object_context& ctx)
    {
        for(auto& i : assets)
        {
            i.load_object(ctx);
        }
    }

    void position_object(objects_container* obj)
    {
        float miny = obj->get_min_y() * obj->dynamic_scale;

        cl_float4 cpos = obj->pos;

        cpos.y -= miny;

        obj->set_pos(cpos);
    }

    void position()
    {
        int id = 0;

        for(asset& a : assets)
        {
            int x = id % 25;
            int y = id / 25;

            cl_float4 pos = {x * 500, 0, y * 500};

            a.loaded_asset->set_pos(pos);

            id++;
        }

        for(asset& a : assets)
        {
            float miny = a.loaded_asset->get_min_y() * a.loaded_asset->dynamic_scale;

            cl_float4 cpos = a.loaded_asset->pos;

            cpos.y -= miny;

            a.loaded_asset->set_pos(cpos);
        }
    }

    void scale()
    {
        for(asset& a : assets)
        {
            a.loaded_asset->set_dynamic_scale(100.f);
        }
    }

    void patch_textures()
    {
        for(asset& a : assets)
        {
            a.loaded_asset->patch_non_2pow_texture_maps();
        }
    }

    void set_last_hovered(objects_container* c)
    {
        if(c == nullptr)
        {
            last_hovered_object = nullptr;
            return;
        }

        last_hovered_object = c;
    }

    void do_ui()
    {
        ImGui::Begin("Currently Hovering");

        std::string hovered_name;

        //if(last_hovered != -1)
        //    hovered_name = assets[last_hovered].loaded_asset->file;

        if(last_hovered_object != nullptr)
            hovered_name = last_hovered_object->file;

        ImGui::Button(hovered_name.c_str());

        ImGui::End();

        std::map<std::string, std::vector<int>> directory_to_asset_name;

        for(int i=0; i<assets.size(); i++)
        {
            directory_to_asset_name[assets[i].asset_toplevel_directory].push_back(i);
        }


        ImGui::Begin("Asset list");

        for(auto& i : directory_to_asset_name)
        {
            std::string dirname = i.first;
            const std::vector<int>& asset_list = i.second;

            if(ImGui::CollapsingHeader(dirname.c_str()))
            {
                for(auto& j : asset_list)
                {
                    asset& a = assets[j];

                    ImGui::Button(a.asset_name.c_str());
                }
            }
        }

        ImGui::End();
    }

    void do_grid_ui()
    {
        ImGui::Begin("Grid UI");

        ImGui::Checkbox("Toggle Grid", &use_grid);

        std::string str;

        str = std::string("GS ") + std::to_string(grid_size);

        //ImGui::Button(str.c_str());

        ImGui::InputInt("Grid Size", &grid_size);

        ImGui::End();
    }

    float cdx = 0;
    float cdy = 0;

    void do_interactivity(engine& window)
    {
        sf::Keyboard key;
        sf::Mouse mouse;

        float dx = window.get_mouse_delta_x();
        float dy = window.get_mouse_delta_y();

        if(last_hovered_object == nullptr)
        {
            cdx = 0;
            cdy = 0;

            return;
        }

        if(!mouse.isButtonPressed(sf::Mouse::Left) && !mouse.isButtonPressed(sf::Mouse::Right))
        {
            cdx = 0;
            cdy = 0;

            return;
        }

        if(last_hovered_object->position_quantise && !use_grid)
        {
            last_hovered_object->pos = conv_implicit<cl_float4>(quant({last_hovered_object->pos.x, last_hovered_object->pos.y, last_hovered_object->pos.z}, last_hovered_object->position_quantise_grid_size));
        }

        cdx += dx;
        cdy += dy;

        cdx = dx;
        cdy = dy;

        float fov_const = calculate_fov_constant_from_hfov(window.horizontal_fov_degrees, window.width);

        cl_float4 pos = last_hovered_object->pos;

        vec3f object_pos = {pos.x, pos.y, pos.z};

        vec3f c_pos = {window.c_pos.x, window.c_pos.y, window.c_pos.z};

        vec3f camera_to_object = object_pos - c_pos;

        float object_depth = camera_to_object.length();

        vec3f object_rot = object_pos.rot({window.c_pos.x, window.c_pos.y, window.c_pos.z}, {window.c_rot.x, window.c_rot.y, window.c_rot.z});
        vec3f object_proj = {object_rot.x() * fov_const / object_rot.z(), object_rot.y() * fov_const / object_rot.z(), object_rot.z()};

        object_proj.x() = object_proj.x() + window.width/2.f;
        object_proj.y() = object_proj.y() + window.height/2.f;

        vec3f new_pos = {object_proj.x() + cdx, object_proj.y() + -cdy, object_proj.z()};

        float yplane = object_pos.y();

        vec3f back_projected_new_pos = {new_pos.x() - window.width/2.f, new_pos.y() - window.height/2, new_pos.z()};

        back_projected_new_pos = {back_projected_new_pos.x() * back_projected_new_pos.z() / fov_const, back_projected_new_pos.y() * back_projected_new_pos.z() / fov_const, back_projected_new_pos.z()};

        vec3f camera_ray = back_projected_new_pos.back_rot({0,0,0}, {window.c_rot.x, window.c_rot.y, window.c_rot.z});

        vec3f intersect_point = ray_plane_intersect(camera_ray, c_pos, (vec3f){0, 1, 0}, (vec3f){0, yplane, 0});

        ///really we want distance to projection plane (?) but... close enough, this is only for controls

        vec3f current_relative_mouse_projected_pos = {cdx * object_depth / fov_const, cdy * object_depth / fov_const, 0.f};

        float move_len = current_relative_mouse_projected_pos.xy().length();

        vec3f move_axis = {0, 0, 0};

        float len = sqrt(cdx*cdx + cdy*cdy);

        if(mouse.isButtonPressed(sf::Mouse::Right))
        {
            move_axis.y() = 1;
        }

        if(move_axis.x() == 1)
        {
            move_axis.x() = move_axis.x() * current_relative_mouse_projected_pos.x();
        }

        if(move_axis.y() == 1)
        {
            move_axis.y() = move_axis.y() * -current_relative_mouse_projected_pos.y();
        }

        if(move_axis.z() == 1)
        {
            move_axis.z() = move_axis.z() * -current_relative_mouse_projected_pos.y();
        }

        vec3f next_pos = object_pos + move_axis;

        cl_float4 next_clpos = {next_pos.x(), next_pos.y(), next_pos.z()};
        cl_float4 current_clpos = last_hovered_object->pos;

        if(move_axis.y() != 0)
            last_hovered_object->set_pos(next_clpos);

        if(mouse.isButtonPressed(sf::Mouse::Left))
        {
            vec3f spos = (vec3f){intersect_point.x(), intersect_point.y(), intersect_point.z()};

            next_clpos = {spos.x(), spos.y(), spos.z()};

            vec2f bound = {15000, 15000};

            if(spos.x() >= -bound.x() && spos.x() < bound.x() && spos.z() >= -bound.y() && spos.z() < bound.y())
            {
                //a.loaded_asset->set_pos({spos.x(), spos.y(), spos.z()});

                last_hovered_object->set_pos(next_clpos);
            }
        }

        last_hovered_object->set_quantise_position(use_grid, grid_size);
    }

    void do_save_ui(object_context& ctx, std::string file, objects_container* floor)
    {
        ImGui::Begin("Save Window");

        if(ImGui::Button("Save Objects"))
        {
            save_objects(ctx, file);
        }

        if(ImGui::Button("Save Terrain"))
        {
            save_terrain(file, floor);
        }

        if(ImGui::Button("Load"))
        {
            load(ctx, file, floor);
        }

        ImGui::End();
    }

    void save_objects(object_context& ctx, std::string file)
    {
        std::ofstream stream(file, std::ios::out | std::ios::trunc);

        for(objects_container* c : ctx.containers)
        {
            if(c->file == "")
                continue;

            if(!c->isactive)
                continue;

            stream << c->file << "\n";

            vec3f pos = {c->pos.x, c->pos.y, c->pos.z};

            //if(c->position_quantise)
            //    pos = quant(pos, c->position_quantise_grid_size);

            stream << pos << "\n";

            stream << c->dynamic_scale << "\n";

            stream << c->rot_quat << "\n";

            stream << (int)c->position_quantise << "\n";

            stream << c->position_quantise_grid_size << "\n";
        }

        stream.close();
    }

    void save_terrain(std::string file, objects_container* floor)
    {
        FILE* pFile = fopen((file + "d").c_str(), "wb");

        for(object& o : floor->objs)
        {
            fwrite(&o.tri_list[0], sizeof(triangle), o.tri_list.size(), pFile);
        }

        fclose(pFile);
    }

    void load(object_context& ctx, std::string file, objects_container* floor)
    {
        if(any_loaded)
            return;

        std::ifstream stream(file);

        if(stream.good())
        {
            std::string file;

            while(std::getline(stream, file))
            {
                std::string posstr;
                std::getline(stream, posstr);

                std::vector<std::string> splitted = split(posstr, ' ');

                cl_float4 pos;

                pos.x = atof(splitted[0].c_str());
                pos.y = atof(splitted[1].c_str());
                pos.z = atof(splitted[2].c_str());

                std::string scalestr;
                std::getline(stream, scalestr);

                float scale = atof(scalestr.c_str());

                std::string quatstr;
                std::getline(stream, quatstr);

                std::vector<std::string> quat_split = split(quatstr, ' ');

                quat rquat;
                rquat.q.v[0] = atof(quat_split[0].c_str());
                rquat.q.v[1] = atof(quat_split[1].c_str());
                rquat.q.v[2] = atof(quat_split[2].c_str());
                rquat.q.v[3] = atof(quat_split[3].c_str());

                int should_quantise;

                std::string shouldquant;
                std::getline(stream, shouldquant);

                should_quantise = atoi(shouldquant.c_str());

                float quant_grid;

                std::string quantgrid;
                std::getline(stream, quantgrid);

                quant_grid = atof(quantgrid.c_str());

                objects_container* c = ctx.make_new();

                c->set_file(file);
                c->set_active(true);

                c->set_pos(pos);

                ctx.load_active();

                c->set_rot_quat(rquat);
                c->set_quantise_position(should_quantise, quant_grid);

                ///found something invalid in the save, ah well
                if(!c->isloaded)
                    c->set_active(false);

                c->set_dynamic_scale(scale);
            }
        }

        ctx.build_request();

        stream.close();

        FILE* pFile = fopen((file + "d").c_str(), "rb");

        fseek(pFile, 0L, SEEK_END);
        int file_len = ftell(pFile);
        rewind(pFile);

        int num_tris = file_len / sizeof(triangle);

        if(num_tris == floor->objs[0].tri_list.size())
        {
            floor->objs[0].tri_list.resize(num_tris);

            fread(&floor->objs[0].tri_list[0], sizeof(triangle), num_tris, pFile);

            fclose(pFile);

            floor->parent->build_request();
        }
        else
        {
            printf("Err invalid num of tris \n\n\n\n\n\n\n\n");
        }

        for(triangle& t : floor->objs[0].tri_list)
        {
            t.vertices[0].set_pad(floor->objs[0].object_g_id);
        }

        ctx.build(true);

        modify_texture_colour_dynamic(ctx);

        any_loaded = true;
    }

    void do_dyn_scale(engine& window)
    {
        sf::Mouse mouse;

        if(!mouse.isButtonPressed(sf::Mouse::XButton1))
            return;

        if(last_hovered_object == nullptr)
            return;

        float dyn_scale = last_hovered_object->dynamic_scale;

        float dx = window.get_mouse_delta_x();
        float dy = window.get_mouse_delta_y();

        dyn_scale += dx;

        dyn_scale = std::max(dyn_scale, 5.f);

        last_hovered_object->set_dynamic_scale(dyn_scale);
    }

    void do_object_rotation(engine& window)
    {
        if(last_hovered_object == nullptr)
            return;

        if(!window.focus)
            return;

        float scroll = window.get_scrollwheel_delta();

        quaternion q;
        q.load_from_euler({0, scroll/10.f, 0});

        quaternion crot;
        crot = last_hovered_object->rot_quat;

        quaternion res;
        res.load_from_matrix(q.get_rotation_matrix() * crot.get_rotation_matrix());

        last_hovered_object->set_rot_quat(res);
    }

    void do_reset()
    {
        if(!once<sf::Mouse::Middle>())
            return;

        if(last_hovered_object == nullptr)
            return;

        last_hovered_object->set_dynamic_scale(100.f);
        last_hovered_object->set_rot_quat(quaternion().identity());
    }

    std::vector<objects_container*> paste_asset_stack;

    void add_to_paste_stack(objects_container* st)
    {
        if(st == nullptr)
            return;

        paste_asset_stack.push_back(st);
    }

    void clear_asset_stack()
    {
        paste_asset_stack.clear();
    }

    void remove_from_asset_stack(objects_container* st)
    {
        for(int i=0; i<paste_asset_stack.size(); i++)
        {
            if(paste_asset_stack[i] == st)
            {
                paste_asset_stack.erase(paste_asset_stack.begin() + i);
                i--;
            }
        }
    }

    void do_paste_stack_ui()
    {
        ImGui::Begin("Paste Stack");

        for(objects_container* o : paste_asset_stack)
        {
            ImGui::Button(o->file.c_str());
        }

        ImGui::End();

        if(once<sf::Keyboard::X>())
        {
            clear_asset_stack();
        }
    }

    bool hide_c = false;
    bool textured_c = false;

    void do_level_ui(objects_container* c, texture* white, texture* regular)
    {
        ImGui::Begin("Hide level ui");

        if(ImGui::Checkbox("Hide", &hide_c))
        {

        }

        if(hide_c)
            c->hide();
        else
            c->set_pos({0,0,0});

        if(ImGui::Checkbox("Textured", &textured_c))
        {

        }

        if(textured_c)
        {
            for(auto& o : c->objs)
            {
                o.tid = regular->id;
            }
        }
        else
        {
            for(auto& o : c->objs)
            {
                o.tid = white->id;
            }
        }

        ImGui::End();
    }

    void check_copy_stack()
    {
        if(!key_combo<sf::Keyboard::LControl, sf::Keyboard::C>())
            return;

        if(last_hovered_object == nullptr)
            return;

        paste_asset_stack.push_back(last_hovered_object);
    }

    void check_paste_stack(object_context& ctx, engine& window)
    {
        if(!key_combo<sf::Keyboard::LControl, sf::Keyboard::V>())
            return;

        if(paste_asset_stack.size() == 0)
            return;

        float fov_const = calculate_fov_constant_from_hfov(window.horizontal_fov_degrees, window.width);

        ///depth of 1 FROM THE PROJECTION PLANE
        vec3f local_pos = {window.get_mouse_x(), window.height - window.get_mouse_y(), 1};

        vec3f unproj_pos = {local_pos.x() - window.width / 2.f, local_pos.y() - window.height / 2.f, local_pos.z()};

        unproj_pos.x() = unproj_pos.x() * local_pos.z() / fov_const;
        unproj_pos.y() = unproj_pos.y() * local_pos.z() / fov_const;

        vec3f camera_ray = unproj_pos.back_rot({0,0,0}, {window.c_rot.x, window.c_rot.y, window.c_rot.z});

        objects_container* cobject = paste_asset_stack[0];

        vec3f intersect = ray_plane_intersect(camera_ray, {window.c_pos.x, window.c_pos.y, window.c_pos.z}, {0, 1, 0}, {0, cobject->pos.y, 0});

        std::string file = cobject->file;

        cl_float4 next_pos = grid_lock({intersect.x(), intersect.y(), intersect.z()});
        cl_float4 current_pos = cobject->pos;

        cl_float4 offset_pos = sub(next_pos, current_pos);

        for(int i=0; i<paste_asset_stack.size(); i++)
        {
            cl_float4 cpos = paste_asset_stack[i]->pos;

            objects_container* n = ctx.make_new();

            n->set_file(paste_asset_stack[i]->file);
            n->set_active(true);
            n->set_unique_textures(true);
            n->cache = false;

            ctx.load_active();
            ctx.build_request();

            n->set_dynamic_scale(paste_asset_stack[i]->dynamic_scale);

            n->set_pos(grid_lock(add(cpos, offset_pos)));
            n->set_rot_quat(paste_asset_stack[i]->rot_quat);
            n->set_quantise_position(paste_asset_stack[i]->position_quantise, paste_asset_stack[i]->position_quantise_grid_size);
        }
    }

    void del(objects_container* st)
    {
        if(last_hovered_object == st)
            last_hovered_object = nullptr;

        if(st == nullptr)
            return;

        for(asset& a : assets)
        {
            if(a.loaded_asset == st)
                return;
        }

        st->hide();
        st->set_active(false);
        remove_from_asset_stack(st);
    }
};

#endif // ASSET_MANAGER_HPP_INCLUDED
