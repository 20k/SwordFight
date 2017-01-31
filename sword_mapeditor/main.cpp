#include "../openclrenderer/proj.hpp"
#include "tinydir/tinydir.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui-SFML.h"


///todo eventually
///split into dynamic and static objects

///todo
///fix memory management to not be atrocious

///we're completely hampered by memory latency

///rift head movement is wrong

#if 0
void load_map(objects_container* obj, int width, int height)
{
    texture* tex = obj->parent->tex_ctx.make_new();
    tex->set_create_colour(sf::Color(200, 200, 200), 128, 128);

    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            //vec3f world_pos_end = get_world_loc(map_def, {x, y}, {width, height});
            //vec3f world_pos_start = world_pos_end;

            float scale = 10.f;

            float height = sin(((float)x/width) * M_PI);

            height = 1 * scale/10;

            vec3f world_pos_end = {x*scale, height*scale, y*scale};
            vec3f world_pos_start = world_pos_end;

            world_pos_start.v[1] = -1;

            objects_container temp_obj;
            temp_obj.parent = obj->parent;

            ///so the first 6 are always the lower section of the cube away from end, and the last 6 are towards the end
            load_object_cube_tex(&temp_obj, world_pos_start, world_pos_end, scale/2, *tex, false);

            ///Ok. So the first two triangles are the base of the cube. We never need this ever
            temp_obj.objs[0].tri_list.erase(temp_obj.objs[0].tri_list.begin() + 0);
            temp_obj.objs[0].tri_list.erase(temp_obj.objs[0].tri_list.begin() + 0);

            /*if(get_map_loc(map_def, {x, y}, {width, height}) == 0)
            {
                for(int i=0; i<8; i++)
                {
                    temp_obj.objs[0].tri_list.pop_back();
                }
            }*/

            temp_obj.objs[0].tri_num = temp_obj.objs[0].tri_list.size();

            ///subobject position set by obj->set_pos in load_object_cube
            obj->objs.push_back(temp_obj.objs[0]);
        }
    }

    obj->independent_subobjects = true;
    obj->isloaded = true;
}

void load_level(objects_container* obj)
{
    return load_map(obj, 100, 100);
}
#endif

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


        //loaded_asset->set_load_func(obj_load_blank_tex);
    }
};

struct asset_manager
{
    std::vector<asset> assets;

    objects_container* last_hovered_object = nullptr;

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

        /*for(int i=0; i<assets.size(); i++)
        {
            if(assets[i].loaded_asset->file == c->file)
            {
                last_hovered = i;
                return;
            }
        }

        last_hovered = -1;*/

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

        /*for(int i=0; i<assets.size(); i++)
        {
            asset& a = assets[i];

            //ImGui::CollapsingHeader

            //ImGui::Button(a.loaded_asset->file.c_str(), ImVec2(0, 14));
        }*/

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

    void do_interactivity(engine& window)
    {
        sf::Keyboard key;
        sf::Mouse mouse;

        float dx = window.get_mouse_delta_x();
        float dy = window.get_mouse_delta_y();

        /*if(once<sf::Mouse::Left>() && last_hovered != -1)
        {
            asset& a = assets[last_hovered];
        }*/

        if(last_hovered_object == nullptr)
            return;

        float fov_const = calculate_fov_constant_from_hfov(window.horizontal_fov_degrees, window.width);

        //asset& a = assets[last_hovered];

        //cl_float4 pos = a.loaded_asset->pos;

        cl_float4 pos = last_hovered_object->pos;

        vec3f object_pos = {pos.x, pos.y, pos.z};

        vec3f c_pos = {window.c_pos.x, window.c_pos.y, window.c_pos.z};

        vec3f camera_to_object = object_pos - c_pos;

        float object_depth = camera_to_object.length();

        vec3f object_rot = object_pos.rot({window.c_pos.x, window.c_pos.y, window.c_pos.z}, {window.c_rot.x, window.c_rot.y, window.c_rot.z});
        vec3f object_proj = {object_rot.x() * fov_const / object_rot.z(), object_rot.y() * fov_const / object_rot.z(), object_rot.z()};

        object_proj.x() = object_proj.x() + window.width/2.f;
        object_proj.y() = object_proj.y() + window.height/2.f;

        vec3f new_pos = {object_proj.x() + dx, object_proj.y() + -dy, object_proj.z()};

        float yplane = object_pos.y();

        vec3f back_projected_new_pos = {new_pos.x() - window.width/2.f, new_pos.y() - window.height/2, new_pos.z()};

        back_projected_new_pos = {back_projected_new_pos.x() * back_projected_new_pos.z() / fov_const, back_projected_new_pos.y() * back_projected_new_pos.z() / fov_const, back_projected_new_pos.z()};

        vec3f camera_ray = back_projected_new_pos.back_rot({0,0,0}, {window.c_rot.x, window.c_rot.y, window.c_rot.z});

        vec3f intersect_point = ray_plane_intersect(camera_ray, c_pos, (vec3f){0, 1, 0}, (vec3f){0, yplane, 0});

        ///really we want distance to projection plane (?) but... close enough, this is only for controls

        vec3f current_relative_mouse_projected_pos = {dx * object_depth / fov_const, dy * object_depth / fov_const, 0.f};

        float move_len = current_relative_mouse_projected_pos.xy().length();

        vec3f move_axis = {0, 0, 0};

        float len = sqrt(dx*dx + dy*dy);

        /*if(mouse.isButtonPressed(sf::Mouse::Left))
        {
            move_axis.x() = 1;
        }

        if(mouse.isButtonPressed(sf::Mouse::XButton1))
        {
            move_axis.y() = 1;
        }

        if(mouse.isButtonPressed(sf::Mouse::Right))
        {
            move_axis.z() = 1;
        }

        if(move_axis.x() == 1)
        {
            //if(dx < 0)
            //    move_axis.x() = -move_axis.x();

            move_axis.x() = move_axis.x() * current_relative_mouse_projected_pos.x();
        }

        if(move_axis.y() == 1)
        {
            //if(dy > 0)
            //    move_axis.y() = -move_axis.y();

            move_axis.y() = move_axis.y() * -current_relative_mouse_projected_pos.y();
        }

        if(move_axis.z() == 1)
        {
            move_axis.z() = move_axis.z() * current_relative_mouse_projected_pos.x();
        }*/

        ///new control scheme, hold left click for xz axis, right click for y axis

        /*if(mouse.isButtonPressed(sf::Mouse::Left))
        {
            move_axis.x() = 1;
            move_axis.z() = 1;
        }*/

        if(mouse.isButtonPressed(sf::Mouse::Right))
        {
            move_axis.y() = 1;
        }

        if(move_axis.x() == 1)
        {
            //if(dx < 0)
            //    move_axis.x() = -move_axis.x();

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

        //move_axis = move_axis.norm() * move_len;

        vec3f spos = object_pos + move_axis;

        //a.loaded_asset->set_pos({spos.x(), spos.y(), spos.z()});

        last_hovered_object->set_pos({spos.x(), spos.y(), spos.z()});

        if(mouse.isButtonPressed(sf::Mouse::Left))
        {
            spos = (vec3f){intersect_point.x(), intersect_point.y(), intersect_point.z()};

            vec2f bound = {15000, 15000};

            if(spos.x() >= -bound.x() && spos.x() < bound.x() && spos.z() >= -bound.y() && spos.z() < bound.y())
            {
                //a.loaded_asset->set_pos({spos.x(), spos.y(), spos.z()});

                last_hovered_object->set_pos({spos.x(), spos.y(), spos.z()});
            }
        }
    }

    ///have a save stack too

    objects_container* asset_to_copy_object = nullptr;

    void check_copy()
    {
        if(key_combo<sf::Keyboard::LControl, sf::Keyboard::C>())
        {
            //asset_to_copy = last_hovered;
            asset_to_copy_object = last_hovered_object;
        }
    }

    void check_paste_object(object_context& ctx, engine& window)
    {
        if(!key_combo<sf::Keyboard::LControl, sf::Keyboard::V>())
            return;

        if(asset_to_copy_object == nullptr)
            return;

        //asset& a = assets[asset_to_copy];

        float fov_const = calculate_fov_constant_from_hfov(window.horizontal_fov_degrees, window.width);

        ///depth of 1 FROM THE PROJECTION PLANE
        vec3f local_pos = {window.get_mouse_x(), window.height - window.get_mouse_y(), 1};

        vec3f unproj_pos = {local_pos.x() - window.width / 2.f, local_pos.y() - window.height / 2.f, local_pos.z()};

        unproj_pos.x() = unproj_pos.x() * local_pos.z() / fov_const;
        unproj_pos.y() = unproj_pos.y() * local_pos.z() / fov_const;

        vec3f camera_ray = unproj_pos.back_rot({0,0,0}, {window.c_rot.x, window.c_rot.y, window.c_rot.z});

        vec3f intersect = ray_plane_intersect(camera_ray, {window.c_pos.x, window.c_pos.y, window.c_pos.z}, {0, 1, 0}, {0, asset_to_copy_object->pos.y, 0});

        std::string file = asset_to_copy_object->file;

        objects_container* c = ctx.make_new();

        c->set_file(file);
        c->set_active(true);

        ctx.load_active();
        ctx.build_request();

        c->set_dynamic_scale(asset_to_copy_object->dynamic_scale);

        position_object(c);

        c->set_pos({intersect.x(), intersect.y(), intersect.z()});

        printf("PASTE PASTE PASTE\n");
    }

    void copy_ui()
    {
        ImGui::Begin("Copied");

        if(asset_to_copy_object != nullptr)
        {
            std::string aname = asset_to_copy_object->file;

            ///beware if button
            ImGui::Button(aname.c_str());
        }
        else
        {
            ImGui::Button("None");
        }

        ImGui::End();
    }
};

///gamma correct mipmap filtering
///7ish pre tile deferred
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
    window.load(1680,1050,1000, "turtles", "../openclrenderer/cl2.cl", true);

    window.set_camera_pos({0, 285.298, -900});

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
    l.set_brightness(2);
    l.radius = 100000;
    l.set_pos((cl_float4){5000, 3000, 300, 0});
    //l.set_pos((cl_float4){-200, 2000, -100, 0});

    light::add_light(&l);

    auto light_data = light::build();
    ///

    window.set_light_data(light_data);

    objects_container* level = context.make_new();

    level->set_load_func(std::bind(load_floor, std::placeholders::_1));
    level->set_active(true);
    level->cache = false;


    ///reallocating the textures of one context
    ///requires invaliding the textures of the second context
    ///this is because textures are global
    ///lets fix this... next time, with JAMES!!!!
    context.load_active();
    context.build(true);
    secondary_context.build(true);

    quaternion q;
    q.load_from_euler({-M_PI/2, 0, 0});
    level->set_rot_quat(q);
    level->set_dynamic_scale(1000.f);

    sf::Image crab;
    crab.loadFromFile("ico.png");

    window.window.setIcon(crab.getSize().x, crab.getSize().y, crab.getPixelsPtr());

    sf::Keyboard key;
    sf::Mouse mouse;

    sf::Clock deltaClock;

    objects_container* last_hovered = nullptr;

    int which_context = 0;

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
        }

        ImGui::SFML::Update(deltaClock.restart());

        object_context& cctx = which_context == 0 ? context : secondary_context;

        asset_manage.check_copy();
        asset_manage.check_paste_object(cctx, window);
        asset_manage.copy_ui();

        if(!mouse.isButtonPressed(sf::Mouse::Left) && !mouse.isButtonPressed(sf::Mouse::Right) && !mouse.isButtonPressed(sf::Mouse::XButton1))
        {
            cl_int frag_id = -1;
            cl_uint depth = -1;

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

            int object_id = cctx.translate_gpu_o_id_to_container_offset(o_id);

            for(auto& i : cctx.containers)
            {
                i->set_outlined(false);
            }

            if(object_id != -1 && depth != -1)
            {
                last_hovered = cctx.containers[object_id];
            }
            else
            {
                last_hovered = nullptr;
            }

            if(last_hovered)
            {
                last_hovered->set_outlined(true);
            }

            asset_manage.set_last_hovered(last_hovered);

            /*float actual_depth = ((float)depth / UINT_MAX) * 350000.f;

            float fov = 500.f; ///will need to use real value

            vec3f local_position = {((x - window.width/2.0f)*actual_depth/fov), ((y - window.height/2.0f)*actual_depth/fov), actual_depth};
            vec3f global_position = local_position.back_rot(0.f, {window.c_rot.x, window.c_rot.y, window.c_rot.z});
            global_position += (vec3f){window.c_pos.x, window.c_pos.y, window.c_pos.z};*/
        }

        if(key.isKeyPressed(sf::Keyboard::Num1))
        {
            which_context = 0;
        }

        if(key.isKeyPressed(sf::Keyboard::Num2))
        {
            which_context = 1;
        }

        compute::event event;

        //if(window.can_render())
        {
            window.generate_realtime_shadowing(*cctx.fetch());
            ///do manual async on thread
            event = window.draw_bulk_objs_n(*cctx.fetch());

            window.set_render_event(event);

            cctx.fetch()->swap_buffers();

            window.increase_render_events();
        }

        window.blit_to_screen(*cctx.fetch());

        window.render_block();

        window.window.resetGLStates();

        asset_manage.do_ui();
        asset_manage.do_interactivity(window);

        ImGui::Render();

        window.flip();

        context.flush_locations();
        secondary_context.flush_locations();

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
