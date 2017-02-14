#include "../openclrenderer/proj.hpp"

///todo eventually
///split into dynamic and static objects

///todo
///fix memory management to not be atrocious

///we're completely hampered by memory latency

///rift head movement is wrong

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

            vec3f world_pos_end = {x*scale, height*scale, y*scale};
            vec3f world_pos_start = world_pos_end;

            world_pos_start.v[1] = -1;

            objects_container temp_obj;
            temp_obj.parent = obj->parent;

            ///so the first 6 are always the lower section of the cube away from end, and the last 6 are towards the end
            load_object_cube_tex(&temp_obj, world_pos_start, world_pos_end, scale/2, *tex, true);

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

    ///we need a context.unload_inactive
    context.load_active();

    sponza->scale(100.f);

    sponza->set_specular(0.f);

    context.build();

    auto object_dat = context.fetch();
    window.set_object_data(*object_dat);

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

    objects_container* level = context.make_new();

    level->set_load_func(std::bind(load_level, std::placeholders::_1));
    level->set_active(true);
    level->cache = false;

    ///reallocating the textures of one context
    ///requires invaliding the textures of the second context
    ///this is because textures are global
    ///lets fix this... next time, with JAMES!!!!

    context.load_active();
    context.build(true);

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

        //if(window.can_render())
        {
            ///do manual async on thread
            event = window.draw_bulk_objs_n(*context.fetch());

            window.set_render_event(event);

            context.fetch()->swap_buffers();

            window.increase_render_events();
        }

        window.blit_to_screen(*context.fetch());

        window.render_block();

        window.flip();

        context.flush_locations();

        context.load_active();
        context.build_tick();
        context.flip();

        if(key.isKeyPressed(sf::Keyboard::M))
            std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }

    ///if we're doing async rendering on the main thread, then this is necessary
    cl::cqueue.finish();
    glFinish();
}