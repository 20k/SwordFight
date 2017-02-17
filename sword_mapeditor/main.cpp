#include "../openclrenderer/proj.hpp"
#include "tinydir/tinydir.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui-SFML.h"
#include "../openclrenderer/vec.hpp"
#include "asset_manager.hpp"

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

    texture* normal = ctx.tex_ctx.make_new();
    normal->set_location("random_normal_map.png");

    c->set_active(true);
    c->cache = false;
    c->set_load_func(std::bind(into_squares, std::placeholders::_1, *tex, (cl_float2){1000, 1000}, 10));

    ctx.load_active();

    quat q;
    q.load_from_euler({-M_PI/2, 0, 0});

    c->set_rot_quat(q);
    c->set_dynamic_scale(30.f);

    ctx.build_request();

    for(object& o : c->objs)
    {
        o.rid = normal->id;
    }

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
                    col = {0.2705 * 255.f, 0.407843 * 255.f, 0.4 * 255.f}; ///forest green
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

void abberate_vts(objects_container* obj)
{
    for(object& o : obj->objs)
    {
        for(triangle& t : o.tri_list)
        {
            bool all_low = true;

            for(vertex& v : t.vertices)
            {
                cl_float2 vt = v.get_vt();

                vt.x *= 2.f;
                vt.y *= 2.f;

                v.set_vt(vt);
            }

            t.vertices[0].set_pad(o.object_g_id);
        }

        int byte_base = o.gpu_tri_start * sizeof(triangle);

        clEnqueueWriteBuffer(cl::cqueue, obj->parent->fetch()->g_tri_mem.get(), CL_FALSE, byte_base, sizeof(triangle) * o.tri_list.size(), &o.tri_list[0], 0, nullptr, nullptr);
    }
}

void tri_ui(objects_container* obj)
{
    ImGui::Begin("Tri UI");

    if(ImGui::Button("Colour Floor") && obj != nullptr)
    {
        colour_object(obj);
    }

    if(ImGui::Button("Fiddle Vts") && obj != nullptr)
    {
        abberate_vts(obj);
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

    secondary_context.set_clear_colour({0, 204/255.f, 255/255.f});

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

    modify_texture_colour_dynamic(context);
    modify_texture_colour_dynamic(secondary_context);

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

        tri_ui(level);

        if(last_hovered == nullptr && window.focus && (&cctx == &secondary_context))
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
                clEnqueueReadBuffer(cl::cqueue.get(), window.g_tid_buf.get(), CL_FALSE, sizeof(cl_uint) * (frag_id * 5 + 4), sizeof(cl_uint), &o_id, 0, nullptr, nullptr);

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
