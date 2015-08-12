#include "../openclrenderer/proj.hpp"
#include "../openclrenderer/ocl.h"
#include "../openclrenderer/texture_manager.hpp"

#include "../openclrenderer/text_handler.hpp"
#include <sstream>
#include <string>
#include "../openclrenderer/vec.hpp"

#include "../openclrenderer/ui_manager.hpp"

#include "../openclrenderer/network.hpp"

#include "../openclrenderer/settings_loader.hpp"
#include "../sword/vec.hpp" ///really need to figure out where to put this

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

int main(int argc, char *argv[])
{
    /*objects_container c1;
    c1.set_load_func(std::bind(load_object_cube, std::placeholders::_1, (vec3f){0, 0, 0}, (vec3f){0, 100, -100}, 20.f));
    c1.cache = false;
    c1.set_active(true);

    objects_container c2;
    c2.set_load_func(std::bind(load_object_cube, std::placeholders::_1, (vec3f){0, 0, 0}, (vec3f){100, -100, 100}, 20.f));
    c2.cache = false;
    c2.set_active(true);*/

    objects_container floor;
    floor.set_file("../openclrenderer/sp2/sp2.obj");
    /*floor.set_load_func(std::bind(load_object_cube, std::placeholders::_1,
                                  (vec3f){0, bodypart::default_position[bodypart::LFOOT].v[1] - bodypart::scale/3, 0},
                                  (vec3f){0, bodypart::default_position[bodypart::LFOOT].v[1] - 42.f, 0},
                                  3000.f, "../sword/res/gray.png"));*/

    ///need to extend this to textures as well
    //floor.set_normal("./res/norm.png");clamped_ids
    floor.cache = false;
    floor.set_active(false);

    engine window;
    window.load(800, 600,1000, "SwordFight", "../openclrenderer/cl2.cl", true);

    //window.window.setFramerateLimit(24.f);

    printf("loaded\n");

    window.set_camera_pos({-1009.17, -94.6033, -317.804});
    window.set_camera_rot({0, 1.6817, 0});

    printf("preload\n");

    obj_mem_manager::load_active_objects();

    printf("postload\n");

    floor.set_specular(0.0f);
    floor.set_diffuse(1.f);

    texture_manager::allocate_textures();

    printf("textures\n");

    obj_mem_manager::g_arrange_mem();
    obj_mem_manager::g_changeover(true);

    printf("loaded memory\n");

    sf::Event Event;

    light l;
    //l.set_col({1.0, 1.0, 1.0, 0});
    l.set_col({1.0, 1.0, 1.0, 0});
    l.set_shadow_casting(0);
    l.set_brightness(0.8f);
    l.set_diffuse(1.f);
    l.set_pos({200, 1000, 0, 0});

    window.add_light(&l);

    printf("light\n");


    uint32_t num = 10000;

    std::vector<cl_float4> positions;
    std::vector<cl_uint> colours;

    for(uint32_t i = 0; i<num; i++)
    {
        vec3f pos = randf<3, float>(-100.f, 100.f);

        uint32_t col = 0xFF00FF00;


        positions.push_back({pos.v[0], pos.v[1], pos.v[2]});
        colours.push_back(col);
    }

    compute::buffer bufs[2];

    for(int i=0; i<2; i++)
        bufs[i] = engine::make_read_write(sizeof(cl_float4)*positions.size(), positions.data());

    auto g_col = engine::make_read_write(sizeof(cl_uint)*colours.size(), colours.data());

    auto screen_buf = engine::make_screen_buffer(sizeof(cl_uint4));

    sf::Mouse mouse;
    sf::Keyboard key;

    int which = 0;
    int nwhich = 1;


    while(window.window.isOpen())
    {
        sf::Clock c;

        if(window.window.pollEvent(Event))
        {
            if(Event.type == sf::Event::Closed)
                window.window.close();
        }

        window.input();

        arg_list c_args;
        c_args.push_back(&screen_buf);

        run_kernel_with_string("clear_screen_buffer", {window.width * window.height}, {128}, 1, c_args);

        arg_list g_args;
        g_args.push_back(&num);
        g_args.push_back(&bufs[which]);
        g_args.push_back(&bufs[nwhich]);

        run_kernel_with_string("gravity_attract", {num}, {128}, 1, g_args);

        arg_list r_args;
        r_args.push_back(&num);
        r_args.push_back(&bufs[which]);
        r_args.push_back(&g_col);
        r_args.push_back(&engine::c_pos);
        r_args.push_back(&engine::c_rot);
        r_args.push_back(&screen_buf);

        run_kernel_with_string("render_naive_points", {num}, {128}, 1, r_args);

        arg_list b_args;
        b_args.push_back(&engine::g_screen);
        b_args.push_back(&screen_buf);

        run_kernel_with_string("blit_unconditional", {window.width * window.height}, {128}, 1, b_args);


        //window.draw_bulk_objs_n();

        window.render_buffers();

        window.display();

        std::cout << c.getElapsedTime().asMicroseconds() << std::endl;

        nwhich = which;
        which = (which + 1) % 2;
    }
}