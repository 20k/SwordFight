#include "../openclrenderer/proj.hpp"

///todo eventually
///split into dynamic and static objects

///todo
///fix memory management to not be atrocious

///we're completely hampered by memory latency

///rift head movement is wrong

//compute::event render_tris(engine& eng, cl_float4 position, cl_float4 rotation, compute::opengl_renderbuffer& g_screen_out);

void callback (cl_event event, cl_int event_command_exec_status, void *user_data)
{
    //printf("%s\n", user_data);

    std::cout << (*(sf::Clock*)user_data).getElapsedTime().asMicroseconds()/1000.f << std::endl;
}

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

    auto sponza = context.make_new();
    //sponza->set_file("../openclrenderer/sp2/sp2.obj");
    sponza->set_file("../sword/res/low/bodypart_blue.obj");
    sponza->set_active(true);
    sponza->cache = false;

    engine window;

    window.load(1680,1050,1000, "turtles", "../openclrenderer/cl2.cl", true);

    window.set_camera_pos({709.88, 285.298, -759.853});
    //window.set_camera_pos((cl_float4){-0,0,-570});


    ///we need a context.unload_inactive
    context.load_active();

    sponza->scale(100.f);

    sponza->set_specular(0.f);

    context.build();

    auto object_dat = context.fetch();
    window.set_object_data(*object_dat);

    //window.set_tex_data(context.fetch()->tex_gpu);

    sf::Event Event;

    light l;
    l.set_col({1.0f, 1.0f, 1.0f, 0.0f});
    l.set_shadow_casting(0);
    l.set_brightness(1);
    l.radius = 100000;
    l.set_pos((cl_float4){-200, 500, -100, 0});

    light::add_light(&l);



    auto light_data = light::build();
    ///

    window.set_light_data(light_data);
    window.construct_shadowmaps();

    /*sf::Texture updated_tex;
    updated_tex.loadFromFile("../openclrenderer/Res/test.png");

    for(auto& i : sponza->objs)
    {
        texture* tex = i.get_texture();

        if(tex->c_image.getSize() == updated_tex.getSize())
        {
            tex->update_gpu_texture(updated_tex, context.fetch()->tex_gpu);
            tex->update_gpu_mipmaps(context.fetch()->tex_gpu);
        }
    }*/

    for(int i=0; i<20; i++)
    {
        auto ctr = context.make_new();

        ctr->set_file("../sword/res/low/bodypart_red.obj");
        ctr->cache = false;
        ctr->set_unique_textures(true);
        ctr->set_pos({(i + 1) * 100, i+1, 0});

        ctr->set_active(true);

        context.load_active();

        ctr->scale(100.f);

        context.build();

        context.flip();
    }

    /*texture trp;
    trp.set_create_colour(sf::Color(255, 255, 255, 200), 128, 128);
    trp.push();*/

    object_context tctx;

    texture* trp = tctx.tex_ctx.make_new();
    trp->set_create_colour(sf::Color(255, 255, 255, 200), 128, 128);

    objects_container* transp = tctx.make_new();
    transp->set_load_func(std::bind(obj_rect, std::placeholders::_1, *trp, (cl_float2){128, 128}));
    transp->set_active(true);

    tctx.load_active();

    transp->scale(2.f);
    transp->set_two_sided(true);

    tctx.build();

    tctx.flip();

    transp->set_pos({0, 0, -200});

    for(int i=0; i<5; i++)
    {
        auto ctr = context.make_new();

        ctr->set_file("../sword/res/low/bodypart_red.obj");
        ctr->cache = false;
        ctr->set_unique_textures(true);
        ctr->set_pos({(i + 1) * 100, (i+1) + 200, 0});

        ctr->set_active(true);

        context.load_active();

        ctr->scale(100.f);

        context.build();

        context.flip();
    }

    ///reallocating the textures of one context
    ///requires invaliding the textures of the second context
    ///this is because textures are global
    ///lets fix this... next time, with JAMES!!!!

    context.build(true);
    tctx.build(true);


    for(auto& i : context.containers)
    {
        //texture* tex = texture_manager::texture_by_id(i->objs[0].tid);

        //tex->update_gpu_mipmaps(context.fetch()->tex_gpu);

        printf("tid %i\n", i->objs[0].tid);
    }

    sf::Keyboard key;

    ///use event callbacks for rendering to make blitting to the screen and refresh
    ///asynchronous to actual bits n bobs
    ///clSetEventCallback
    while(window.window.isOpen() && !key.isKeyPressed(sf::Keyboard::F10))
    {
        sf::Clock c;

        while(window.window.pollEvent(Event))
        {
            if(Event.type == sf::Event::Closed)
                window.window.close();
        }

        compute::event event;

        if(window.can_render())
        {
            //window.clear_screen(*context.fetch());
            //window.clear_depth_buffer(*context.fetch());

            ///do manual async on thread
            window.draw_bulk_objs_n(*context.fetch());

            //window.set_render_event(event);

            context.fetch()->swap_buffers();

            //window.increase_render_events();
        }

        window.blit_to_screen(*context.fetch());

        window.flip();

        window.render_block();

        context.flush_locations();

        context.build_tick();

        if(key.isKeyPressed(sf::Keyboard::M))
            std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }

    ///if we're doing async rendering on the main thread, then this is necessary
    cl::cqueue.finish();
    glFinish();
}
