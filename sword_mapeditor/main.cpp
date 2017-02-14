#include "../openclrenderer/proj.hpp"
#include "tinydir/tinydir.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui-SFML.h"
#include "../openclrenderer/util.hpp"
#include "../openclrenderer/vec.hpp"

///has the button been pressed once, and only once
template<sf::Keyboard::Key k>
bool once()
{
    static bool last;

    sf::Keyboard key;

    if(key.isKeyPressed(k) && !last)
    {
        last = true;

        return true;
    }

    if(!key.isKeyPressed(k))
    {
        last = false;
    }

    return false;
}

template<sf::Mouse::Button b>
bool once()
{
    static bool last;

    sf::Mouse m;

    if(m.isButtonPressed(b) && !last)
    {
        last = true;

        return true;
    }

    if(!m.isButtonPressed(b))
    {
        last = false;
    }

    return false;
}

template<sf::Keyboard::Key k1, sf::Keyboard::Key k2>
bool key_combo()
{
    sf::Keyboard key;

    static bool last = false;

    bool b1 = key.isKeyPressed(k1);
    bool b2 = key.isKeyPressed(k2);

    if(b1 && b2 && !last)
    {
        last = true;

        return true;
    }

    if(!b1 || !b2)
    {
        last = false;
    }

    return false;
}

struct displace_info
{
    vec3f pos;
    int yval;
    //int id_offset = 0;
};

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

void into_squares(objects_container* pobj, texture& tex, cl_float2 dim, float tessellate_dim)
{
    cl_float2 hdim = {dim.x/2, dim.y/2};

    ///1 + this many of points
    ///how many of length tessellate dim fit into dim
    float xnum = dim.x / tessellate_dim;
    float ynum = dim.y / tessellate_dim;

    int xp = xnum;
    int yp = ynum;

    float start_x = -dim.x/2;
    float start_y = -dim.y/2;

    object obj;
    obj.isloaded = true;

    for(int y = 0; y < yp; y++)
    {
        for(int x = 0; x < xp; x++)
        {

            std::array<cl_float4, 6> tris;

            tris[0] = {start_x + tessellate_dim, start_y, 0};
            tris[1] = {start_x, start_y, 0};
            tris[2] = {start_x, start_y + tessellate_dim, 0};

            tris[3] = {start_x + tessellate_dim, start_y, 0};
            tris[4] = {start_x, start_y + tessellate_dim, 0};
            tris[5] = {start_x + tessellate_dim, start_y + tessellate_dim, 0};

            triangle t1, t2;

            t1.vertices[0].set_pos(tris[0]);
            t1.vertices[1].set_pos(tris[1]);
            t1.vertices[2].set_pos(tris[2]);

            t2.vertices[0].set_pos(tris[3]);
            t2.vertices[1].set_pos(tris[4]);
            t2.vertices[2].set_pos(tris[5]);

            cl_float4 normal = {0, 0, -1};

            t1.vertices[0].set_normal(normal);
            t1.vertices[1].set_normal(normal);
            t1.vertices[2].set_normal(normal);

            t2.vertices[0].set_normal(normal);
            t2.vertices[1].set_normal(normal);
            t2.vertices[2].set_normal(normal);

            for(int i=0; i<3; i++)
            {
                float vx = (t1.vertices[i].get_pos().x + dim.x/2) / dim.x;
                float vy = (t1.vertices[i].get_pos().y + dim.y/2) / dim.y;

                t1.vertices[i].set_vt({vx, vy});
            }

            for(int i=0; i<3; i++)
            {
                float vx = (t2.vertices[i].get_pos().x + dim.x/2) / dim.x;
                float vy = (t2.vertices[i].get_pos().y + dim.y/2) / dim.y;

                t2.vertices[i].set_vt({vx, vy});
            }

            obj.tri_list.push_back(t1);
            obj.tri_list.push_back(t2);

            start_x += tessellate_dim;
        }

        start_y += tessellate_dim;
        start_x = -dim.x/2;
    }

    obj.tri_num = obj.tri_list.size();

    obj.tid = tex.id;

    pobj->objs.push_back(obj);

    pobj->isloaded = true;

}

objects_container* load_map_reference(object_context& ctx)
{
    objects_container* c = ctx.make_new();

    texture* tex = ctx.tex_ctx.make_new();
    tex->set_location("mapshots/map_texture.png");

    c->set_active(true);
    c->cache = false;
    c->set_load_func(std::bind(into_squares, std::placeholders::_1, *tex, (cl_float2){1000, 1000}, 10));

    ctx.load_active();

    quat q;
    q.load_from_euler({-M_PI/2, 0, 0});

    c->set_rot_quat(q);
    c->set_dynamic_scale(30.f);

    ctx.build_request();

    return c;
}

void colour_object(objects_container* obj)
{
    for(object& o : obj->objs)
    {
        for(triangle& t : o.tri_list)
        {
            bool all_low = true;

            for(vertex& v : t.vertices)
            {
                /*if(v.get_pos().z > 1)
                    //v.set_vertex_col(0, 0, 1, 255);
                    v.set_vertex_col(193, 154, 107, 255);
                else
                    //v.set_vertex_col(0,0,0,0);
                    //v.set_vertex_col(34, 139, 34, 255);
                    v.set_vertex_col(0.2705 * 255.f, 0.407843 * 255.f, 0.4 * 255.f, 255);*/

                if(v.get_pos().z < 1)
                    all_low = false;

            }

            for(vertex& v : t.vertices)
            {
                vec3f col;

                if(all_low)
                {
                    col = {193, 154, 107};
                }
                else
                {
                    col = {0.2705 * 255.f, 0.407843 * 255.f, 0.4 * 255.f};
                }

                col = col + col * randf_s(-0.01f, 0.01f);

                col = clamp(col, 0.f, 255.f);

                v.set_vertex_col(col.x(), col.y(), col.z(), 255);
            }

            t.vertices[0].set_pad(o.object_g_id);
        }

        int byte_base = o.gpu_tri_start * sizeof(triangle);

        clEnqueueWriteBuffer(cl::cqueue, obj->parent->fetch()->g_tri_mem.get(), CL_FALSE, byte_base, sizeof(triangle) * o.tri_list.size(), &o.tri_list[0], 0, nullptr, nullptr);
    }
}

void colour_ui(objects_container* obj)
{
    ImGui::Begin("Colour Floor");

    if(ImGui::Button("Colour Floor") && obj != nullptr)
    {
        colour_object(obj);
    }

    ImGui::End();
}

uint32_t wang_hash(uint32_t seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

uint32_t rand_xorshift(uint32_t rng_state)
{
    // Xorshift algorithm from George Marsaglia's paper
    rng_state ^= (rng_state << 13);
    rng_state ^= (rng_state >> 17);
    rng_state ^= (rng_state << 5);
    return rng_state;
}

void scatter_vert(vertex& v, float amount)
{
    vec3f pos = xyz_to_vec(v.get_pos());

    uint32_t base = wang_hash(pos.x() * 1000000 + pos.y() * 1000 + pos.z());

    uint32_t seed2 = rand_xorshift(base);
    uint32_t seed3 = rand_xorshift(seed2);
    uint32_t seed4 = rand_xorshift(seed3);

    float f1 = (float)seed2 / (float)UINT_MAX;
    float f2 = (float)seed3 / (float)UINT_MAX;
    float f3 = (float)seed4 / (float)UINT_MAX;

    vec3f rseed = (vec3f){f1, f2, f3};
    rseed = (rseed - 0.5f) * 2;

    rseed.x() *= 5 * amount;
    rseed.y() *= 5 * amount;

    rseed = rseed * 1.f + pos;

    v.set_pos({rseed.x(), rseed.y(), rseed.z()});
}

void scatter(objects_container* c, float displace_amount = 3)
{
    for(object& o : c->objs)
    {
        for(triangle& t : o.tri_list)
        {
            for(vertex& v : t.vertices)
            {
                vec3f pos = xyz_to_vec(v.get_pos());

                uint32_t base = wang_hash(pos.x() * 1000000 + pos.y() * 1000 + pos.z());

                uint32_t seed2 = rand_xorshift(base);
                uint32_t seed3 = rand_xorshift(seed2);
                uint32_t seed4 = rand_xorshift(seed3);

                float f1 = (float)seed2 / (float)UINT_MAX;
                float f2 = (float)seed3 / (float)UINT_MAX;
                float f3 = (float)seed4 / (float)UINT_MAX;

                vec3f rseed = (vec3f){f1, f2, f3};
                rseed = (rseed - 0.5f) * 2;

                rseed.x() *= displace_amount;
                rseed.y() *= displace_amount;

                rseed = rseed * 1.f + pos;

                v.set_pos({rseed.x(), rseed.y(), rseed.z()});
            }

            t.generate_flat_normals();
        }
    }
}


///I think I'm going to have to do normal mapping to get the detail I actually want
void displace_near_tris(engine& window, objects_container* floor, object_context& ctx)
{
    vec3f screen_mouse = {window.get_mouse_x(), window.height - window.get_mouse_y(), 1.f};

    vec3f world_ray = screen_mouse.depth_unproject({window.width, window.height}, calculate_fov_constant_from_hfov(window.horizontal_fov_degrees, window.width)).back_rot(0, window.get_camera_rot());

    ///might need to negate intersection
    vec3f intersect = ray_plane_intersect(world_ray.norm(), window.get_camera_pos(), {0, 1, 0}, {0,0,0});

    vec3f nearest_vertex = {0, 9999999, 0};

    for(object& o : floor->objs)
    {
        for(triangle& t : o.tri_list)
        {
            for(vertex& v : t.vertices)
            {
                vec3f lpos = xyz_to_vec(v.get_pos()) * o.dynamic_scale;

                lpos = o.rot_quat.get_rotation_matrix() * lpos;

                lpos = lpos + xyz_to_vec(o.pos);

                if((lpos.xz() - intersect.xz()).length() < (nearest_vertex.xz() - intersect.xz()).length())
                {
                    nearest_vertex = lpos;
                }
            }
        }
    }

    sf::Mouse mouse;

    if(!mouse.isButtonPressed(sf::Mouse::Left) && !mouse.isButtonPressed(sf::Mouse::Right))
        return;

    float displace_dir = 0.f;

    if(mouse.isButtonPressed(sf::Mouse::Left))
        displace_dir = 1;

    if(mouse.isButtonPressed(sf::Mouse::Right))
        displace_dir = -1;

    displace_dir *= 2.f;

    #define FIXED
    #ifdef FIXED
    if(displace_dir > 0)
        displace_dir = 2;

    if(displace_dir < 0)
        displace_dir = 0;
    #endif

    bool any_dirty = false;

    for(object& o : floor->objs)
    {
        o.set_outlined(false);

        int which_vertex = 0;

        //for(triangle& t : o.tri_list)
        for(int ti = 0; ti < o.tri_list.size(); ti++)
        {
            bool dirty = false;

            triangle& t = o.tri_list[ti];
            triangle backup = t;

            //for(vertex& v : t.vertices)
            for(int vi = 0; vi < 3; vi++)
            {
                vertex& v = t.vertices[vi];

                vec3f original_pos = xyz_to_vec(v.get_pos());

                ///we need to project the position
                vec3f lpos = o.rot_quat.get_rotation_matrix() * original_pos * o.dynamic_scale;

                lpos = lpos + xyz_to_vec(o.pos);

                float dist = (lpos.xz() - nearest_vertex.xz()).length();

                if(dist < 0.01f)
                {
                    /*vec3f pos = xyz_to_vec(v.get_pos());

                    pos.z() += displace_dir;

                    v.set_pos(conv_implicit<cl_float4>(pos));*/

                    dirty = true;


                    cl_float4 orig = backup.vertices[vi].get_pos();
                    orig.z = displace_dir;
                    backup.vertices[vi].set_pos(orig);

                    any_dirty = true;
                }

                which_vertex++;
            }

            t.generate_flat_normals();
            backup.generate_flat_normals();

            if(dirty)
            {
                for(int vi = 0; vi < 3; vi++)
                {
                    t.vertices[vi].set_pos(backup.vertices[vi].get_pos());
                }

                ///ok. This is a bit of a weird one, this is used for a triangle to know what object its related to
                ///but its only set cpu side depending on performance optimisation circumstances
                ///so we need to set it here as well in case an object has too many triangles to be done cpu side
                t.vertices[0].set_pad(o.object_g_id);
            }

            ///we're doing something wrong here, works after a rebuild
            if(dirty)
            {
                int byte_base = o.gpu_tri_start * sizeof(triangle) + ti * sizeof(triangle);

                clEnqueueWriteBuffer(cl::cqueue, floor->parent->fetch()->g_tri_mem.get(), CL_FALSE, byte_base, sizeof(triangle), &t, 0, nullptr, nullptr);
            }
        }
    }
}

///need to delete objects next
///make the floor cuboid like the old experiment
///or at least just made up of small squares
///or polygonally model the colours
///Ok. Squares didn't work. Maybe cubes will? But itll basically be as a substitue for polygonal modelling
///Ok. Terrain deformation. Just nudge the vertices around. Maybe procedurally add some terrain randomness too
///radiosity point view sampling thing?
int main(int argc, char *argv[])
{
    lg::set_logfile("./logging.txt");

    std::streambuf* b1 = std::cout.rdbuf();

    std::streambuf* b2 = lg::output->rdbuf();

    std::ios* r1 = &std::cout;
    std::ios* r2 = lg::output;

    r2->rdbuf(b1);

    sf::Clock load_time;

    object_context context;
    object_context secondary_context;

    auto sponza = context.make_new();
    //sponza->set_file("../openclrenderer/sp2/sp2.obj");
    sponza->set_file("../sword/res/low/bodypart_blue.obj");
    sponza->set_active(false);
    sponza->cache = false;

    engine window;

    window.append_opencl_extra_command_line("-DCAN_OUTLINE");
    //window.append_opencl_extra_command_line("-DSTYLISED");
    window.append_opencl_extra_command_line("-DSHADOWBIAS=200");
    window.append_opencl_extra_command_line("-DSHADOWEXP=200");
    window.append_opencl_extra_command_line("-DSSAO_DIV=2");
    window.load(1680,1050,1000, "turtles", "../openclrenderer/cl2.cl", true);

    window.set_camera_pos({0, 485.298, -900});
    window.set_camera_rot({0.4, 0, 0});
    window.c_rot_keyboard_only = window.c_rot;

    ImGui::SFML::Init(window.window);

    window.raw_input_set_active(true);

    asset_manager asset_manage;
    asset_manage.populate("Assets");
    asset_manage.load_object_all(context);

    ///we need a context.unload_inactive
    context.load_active();
    secondary_context.load_active();


    asset_manage.scale();
    asset_manage.position();
    asset_manage.patch_textures();

    sponza->scale(100.f);

    sponza->set_specular(0.f);

    context.build();

    auto object_dat = context.fetch();
    window.set_object_data(*object_dat);

    sf::Event Event;

    light l;
    l.set_col({1.0f, 1.0f, 1.0f, 0.0f});
    l.set_shadow_casting(1);
    l.set_brightness(4);
    l.radius = 100000;
    l.set_pos((cl_float4){0, 15000, 300, 0});
    //l.set_pos((cl_float4){-200, 2000, -100, 0});

    light* current_light = light::add_light(&l);

    auto light_data = light::build();
    ///

    window.set_light_data(light_data);

    /*objects_container* level = secondary_context.make_new();

    level->set_load_func(std::bind(load_floor, std::placeholders::_1));
    level->set_active(false);
    level->cache = false;*/

    objects_container* level = load_map_reference(secondary_context);


    texture* green_texture = secondary_context.tex_ctx.make_new();
    //green_texture->set_create_colour(sf::Color(128, 255, 128), 32, 32);
    green_texture->set_create_colour(sf::Color(10, 110, 10), 32, 32);
    green_texture->force_load = true;

    texture* brown_texture = secondary_context.tex_ctx.make_new();
    brown_texture->set_create_colour(sf::Color(140, 79, 10), 32, 32);
    //brown_texture->set_create_colour(sf::Color(244, 164, 96), 32, 32);
    brown_texture->force_load = true;

    texture* white_texture = secondary_context.tex_ctx.make_new();
    white_texture->set_create_colour(sf::Color(180, 180, 180), 32, 32);
    white_texture->force_load = true;

    texture* floor_texture = secondary_context.tex_ctx.make_new();
    floor_texture->set_location("mapshots/map_texture.png");
    floor_texture->force_load = true;

    ///reallocating the textures of one context
    ///requires invaliding the textures of the second context
    ///this is because textures are global
    ///lets fix this... next time, with JAMES!!!!
    context.load_active();
    context.build(true);
    secondary_context.load_active();
    scatter(level);
    secondary_context.build(true);

    for(auto& i : level->objs)
    {
        //i.tid = white_texture->id;
    }

    sf::Image crab;
    crab.loadFromFile("ico.png");

    window.window.setIcon(crab.getSize().x, crab.getSize().y, crab.getPixelsPtr());

    sf::Keyboard key;
    sf::Mouse mouse;

    sf::Clock deltaClock;

    objects_container* last_hovered = nullptr;
    int last_o_id = -1;

    int which_context = 0;

    cl_float4 saved_c_pos[2];
    cl_float4 saved_c_rot[2];
    cl_float4 saved_keyboard_default[2];

    for(auto& i : saved_c_pos)
        i = window.c_pos;

    for(auto& i : saved_c_rot)
        i = window.c_rot;

    for(auto& i : saved_keyboard_default)
        i = window.c_rot_keyboard_only;


    int last_mx = window.get_mouse_x();
    int last_my = window.get_mouse_y();

    vec2f last_vt_upscaled = {-1,-1};

    ///use event callbacks for rendering to make blitting to the screen and refresh
    ///asynchronous to actual bits n bobs
    ///clSetEventCallback
    while(window.window.isOpen() && !key.isKeyPressed(sf::Keyboard::F10))
    {
        sf::Clock c;

        window.reset_scrollwheel_delta();
        window.raw_input_process_events();

        while(window.window.pollEvent(Event))
        {
            if(Event.type == sf::Event::Closed)
                window.window.close();

            ImGui::SFML::ProcessEvent(Event);

            window.update_scrollwheel_delta(Event);

            if(Event.type == sf::Event::GainedFocus)
                window.set_focus(true);

            if(Event.type == sf::Event::LostFocus)
                window.set_focus(false);
        }

        ImGui::SFML::Update(deltaClock.restart());

        object_context& cctx = which_context == 0 ? context : secondary_context;

        if(&cctx == &context)
        {
            current_light->set_brightness(2);
        }
        else
        {
            current_light->set_brightness(2);
        }

        //asset_manage.check_copy();
        //asset_manage.check_paste_object(cctx, window);

        #if 1
        if(window.focus)
        {
            asset_manage.check_copy_stack();
            asset_manage.check_paste_stack(cctx, window);
        }

        //asset_manage.copy_ui();

        if(window.focus)
        {
            asset_manage.do_dyn_scale(window);
            asset_manage.do_object_rotation(window);
            asset_manage.do_reset();
        }

        asset_manage.do_grid_ui();
        asset_manage.do_paste_stack_ui();
        asset_manage.do_level_ui(level, white_texture, floor_texture);

        colour_ui(level);

        if(last_hovered == nullptr && window.focus)
            displace_near_tris(window, level, secondary_context);

        /*if(key_combo<sf::Keyboard::LControl, sf::Keyboard::S>() && window.focus)
        {
            asset_manage.save(secondary_context, "save.txt", level);
        }*/

        if(once<sf::Keyboard::Delete>() && last_hovered != level && window.focus && last_hovered != nullptr)
        {
            asset_manage.del(last_hovered);
        }

        int object_id = -1;
        cl_uint depth = -1;

        {
            cl_int frag_id = -1;

            int x = window.get_mouse_x();
            int y = window.height - window.get_mouse_y();

            size_t origin[3] = {window.get_mouse_x(), window.height - window.get_mouse_y(), 0};
            size_t region[3] = {1, 1, 1};

            ///if we wanted to do this better, we could set up a clseteventcallback on the completion of the read of frag id, and use that to enqueue the next read
            ///that way we'd only have to stall explicitly once at the end. There might be an implicit stall in the middle though, but its possible that the other read will be overlapped which might take out some of the pain
            ///but really thats not going to be enough time to remove a stall. This needs testing. We could overlap it with shadow generation
            if(origin[0] >= 0 && origin[0] < window.width && origin[1] >= 0 && origin[1] < window.height)
            {
                clEnqueueReadImage(cl::cqueue.get(), cctx.fetch()->g_id_screen_tex.get(), CL_FALSE, origin, region, 0, 0, &frag_id, 0, nullptr, nullptr);
                clEnqueueReadBuffer(cl::cqueue.get(), cctx.fetch()->depth_buffer[1].get(), CL_FALSE, sizeof(cl_uint) * (y * window.width + x), sizeof(cl_uint), &depth, 0, nullptr, nullptr);
            }

            cl::cqueue.finish();

            cl_uint o_id = -1;

            if(frag_id >= 0)
                clEnqueueReadBuffer(cl::cqueue.get(), window.g_tid_buf.get(), CL_FALSE, sizeof(cl_uint) * (frag_id * 6 + 5), sizeof(cl_uint), &o_id, 0, nullptr, nullptr);

            cl::cqueue.finish();

            last_o_id = o_id;
            object_id = cctx.translate_gpu_o_id_to_container_offset(o_id);
        }

        if(!mouse.isButtonPressed(sf::Mouse::Left) && !mouse.isButtonPressed(sf::Mouse::Right) && !mouse.isButtonPressed(sf::Mouse::XButton1) && window.focus)
        {
            for(auto& i : cctx.containers)
            {
                if(i == level)
                    continue;

                i->set_outlined(false);
            }

            /*if(last_hovered != nullptr)
            {
                last_hovered->set_quantise_position(false);
            }*/

            if(object_id != -1 && depth != -1)
            {
                last_hovered = cctx.containers[object_id];
            }
            else
            {
                last_hovered = nullptr;
            }

            if(last_hovered && last_hovered != level)
            {
                last_hovered->set_outlined(true);
            }

            if(last_hovered != level)
                asset_manage.set_last_hovered(last_hovered);

            if(last_hovered == level)
                asset_manage.set_last_hovered(nullptr);

            if(last_hovered == level)
                last_hovered = nullptr;


            /*float actual_depth = ((float)depth / UINT_MAX) * 350000.f;

            float fov = 500.f; ///will need to use real value

            vec3f local_position = {((x - window.width/2.0f)*actual_depth/fov), ((y - window.height/2.0f)*actual_depth/fov), actual_depth};
            vec3f global_position = local_position.back_rot(0.f, {window.c_rot.x, window.c_rot.y, window.c_rot.z});
            global_position += (vec3f){window.c_pos.x, window.c_pos.y, window.c_pos.z};*/
        }

        #ifdef TEXTURE_COLOURING
        if(!mouse.isButtonPressed(sf::Mouse::Left) && !mouse.isButtonPressed(sf::Mouse::Right))
        {
            last_vt_upscaled.v[0] = -1;
        }

        ///SPECIAL CASE TESTING
        if(last_hovered == level && window.focus)
        {
            object* o = nullptr;

            for(object& c : last_hovered->objs)
            {
                if(last_o_id == c.object_g_id)
                    o = &c;
            }

            /*if(mouse.isButtonPressed(sf::Mouse::Left))
            {
                if(o != nullptr)
                    o->tid = green_texture->id;
            }

            if(mouse.isButtonPressed(sf::Mouse::Right))
            {
                if(o != nullptr)
                    o->tid = brown_texture->id;
            }*/

            vec3f col = 0;

            bool lm = mouse.isButtonPressed(sf::Mouse::Left);
            bool rm = mouse.isButtonPressed(sf::Mouse::Right);

            if(lm)
                col = {10, 110, 10};

            if(rm)
                col = {140, 79, 10};

            if((lm || rm) && o != nullptr)
            {
                vec3f screen_mouse = {window.get_mouse_x(), window.height - window.get_mouse_y(), 1.f};

                vec3f world_ray = screen_mouse.depth_unproject({window.width, window.height}, calculate_fov_constant_from_hfov(window.horizontal_fov_degrees, window.width)).back_rot(0, window.get_camera_rot());

                vec3f intersect = ray_plane_intersect(world_ray.norm(), window.get_camera_pos(), {0, 1, 0}, {0,0,0});

                vec2f vt = (((intersect.xz() / 500.f) / level->dynamic_scale) + 1.f) / 2.f;

                vt.y() = 1.f - vt.y();

                //vt = clamp(vt, 0.f, 1.f);

                printf("vt %f %f\n", EXPAND_2(vt));

                std::cout << "isect " << intersect << "\n";

                vec2f upscaled_pos = vt * 2048.f;

                upscaled_pos = clamp(upscaled_pos, 0.f, 2047);

                if(last_vt_upscaled.v[0] < 0)
                    last_vt_upscaled = upscaled_pos;

                texture* tex = secondary_context.tex_ctx.id_to_tex(level->objs[0].tid);

                tex->c_image.setPixel(upscaled_pos.x(), upscaled_pos.y(), sf::Color(255, 255, 255));

                int thickness = 6;

                vec2f line_dir;
                int num;
                line_draw_helper(upscaled_pos, last_vt_upscaled, line_dir, num);

                vec2f start = upscaled_pos;

                sf::Color cl(col.x(), col.y(), col.z());

                for(int i=0; i<num + 1; i++)
                {
                    vec2f perp = line_dir.rot(M_PI/2);

                    vec2f perp_start = start - perp * thickness;
                    vec2f perp_end = start + perp * thickness;

                    int pnum;
                    vec2f pdir;

                    line_draw_helper(perp_start, perp_end, pdir, pnum);

                    for(int j=0; j<pnum+1; j++)
                    {
                        perp_start = clamp(perp_start, 0.f, 2046);

                        tex->c_image.setPixel(perp_start.x(), perp_start.y(), cl);
                        tex->c_image.setPixel(perp_start.x()+1, perp_start.y(), cl);
                        tex->c_image.setPixel(perp_start.x(), perp_start.y()+1, cl);
                        tex->c_image.setPixel(perp_start.x()+1, perp_start.y()+1, cl);

                        perp_start = perp_start + pdir;
                    }

                    tex->c_image.setPixel(start.x(), start.y(), cl);

                    start = start + line_dir;
                }


                last_mx = window.get_mouse_x();
                last_my = window.get_mouse_y();

                last_vt_upscaled = upscaled_pos;

                tex->update_me_to_gpu(secondary_context.fetch()->tex_gpu_ctx);
            }
        }
        #endif
        #endif // 0

        if(key.isKeyPressed(sf::Keyboard::Num1) && window.focus)
        {

            saved_c_pos[which_context] = window.c_pos;
            saved_c_rot[which_context] = window.c_rot;
            saved_keyboard_default[which_context] = window.c_rot_keyboard_only;

            which_context = 0;

            window.c_pos = saved_c_pos[which_context];
            window.c_rot = saved_c_rot[which_context];
            window.c_rot_keyboard_only = saved_keyboard_default[which_context];
        }

        if(key.isKeyPressed(sf::Keyboard::Num2) && window.focus)
        {
            saved_c_pos[which_context] = window.c_pos;
            saved_c_rot[which_context] = window.c_rot;
            saved_keyboard_default[which_context] = window.c_rot_keyboard_only;

            which_context = 1;

            window.c_pos = saved_c_pos[which_context];
            window.c_rot = saved_c_rot[which_context];
            window.c_rot_keyboard_only = saved_keyboard_default[which_context];
        }

        /*if(once<sf::Keyboard::F1>())
        {
            asset_manage.save(secondary_context, "save.txt");
        }

        if(once<sf::Keyboard::F2>())
        {
            asset_manage.load(secondary_context, "save.txt");
        }*/

        auto dat = light::build(&light_data);
        window.set_light_data(dat);

        compute::event event;

        //if(window.can_render())
        {
            window.generate_realtime_shadowing(*cctx.fetch());
            ///do manual async on thread
            event = window.draw_bulk_objs_n(*cctx.fetch());

            //event = window.do_pseudo_aa();

            window.set_render_event(event);

            cctx.fetch()->swap_buffers();

            window.increase_render_events();
        }

        window.blit_to_screen(*cctx.fetch());

        window.render_block();

        window.window.resetGLStates();

        #if 1

        asset_manage.do_ui();
        asset_manage.do_save_ui(secondary_context, "save.txt", level);

        asset_manage.do_interactivity(window);

        ImGui::Render();

        #endif

        window.flip();

        /*context.flush_locations();
        secondary_context.flush_locations();*/

        cctx.flush_locations();

        context.load_active();
        context.build_tick();
        context.flip();

        secondary_context.load_active();
        secondary_context.build_tick();
        secondary_context.flip();

        if(key.isKeyPressed(sf::Keyboard::M))
            std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }

    ImGui::SFML::Shutdown();
    ///if we're doing async rendering on the main thread, then this is necessary
    cl::cqueue.finish();
    glFinish();
}
