#include "../openclrenderer/proj.hpp"

#include "../openclrenderer/vec.hpp"

#include <vec/vec.hpp>

#include <math.h>

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

objects_container ctr;

void lobj()
{
    float dim = 4;

    texture tex;

    obj_cube_by_extents(&ctr, tex, {dim, dim, dim});
}

std::vector<triangle> tri_to_cubes(const triangle& tri)
{
    std::vector<triangle> new_tris;

    vec3f pos[3];

    for(int i=0; i<3; i++)
    {
        pos[i] = xyz_to_vec(tri.vertices[i].get_pos());
    }

    int c = 0;

    vec3f v1 = pos[1] - pos[0];
    vec3f v2 = pos[2] - pos[0];

    float len = v1.length();

    vec3f vn = v1.norm();

    //printf("L1 %f\n", len);

    for(float i=0; i<len; i += 50.f)
    {
        vec3f p = vn * i + pos[0];

        vec3f dist_to_v2 = point2line_shortest(pos[0], v2, p);

        float dv2l = dist_to_v2.length();

        //printf("L2 %f\n", dv2l);

        for(float j=0; j<dv2l; j += 50.f)
        {
            c++;

            vec3f p2 = p + j * dist_to_v2.norm();

            //printf("%f %f %f\n", EXPAND_3(p2));

            //if(c % 100 != 0)
            //    continue;

            if(std::isnan(p2.v[0]) || std::isnan(p2.v[1]) || std::isnan(p2.v[0]))
            {
                printf("nan\n");
                continue;
            }


            std::vector<triangle> cube_tris = ctr.objs[0].tri_list;

            for(auto& i : cube_tris)
            {
                for(auto& k : i.vertices)
                {
                    cl_float4 rel = add(k.get_pos(), {p2.v[0], p2.v[1], p2.v[2]});

                    k.set_pos(rel);

                    //k.set_pos(add(k.get_pos(), tri.vertices[0].get_pos()));
                    //k.set_pos({p2.v[0], p2.v[1], p2.v[2]});
                    k.set_vt(tri.vertices[0].get_vt());
                }

                new_tris.push_back(i);
            }
        }
    }

    return new_tris;
}

std::vector<triangle> cube_by_area(const std::vector<triangle>& in_tris)
{
    std::vector<triangle> new_tris;

    for(auto& i : in_tris)
    {
        auto tris = tri_to_cubes(i);

        for(auto& j : tris)
        {
            new_tris.push_back(j);
        }
    }

    return new_tris;
}

std::vector<triangle> cubulate(const std::vector<triangle>& in_tris)
{
    std::vector<triangle> tris = in_tris;

    float dim = 4;

    texture tex;

    objects_container ctr;

    int skip_c = 0;

    std::vector<triangle> new_tris;

    for(auto& base : tris)
    {
        for(auto& vert : base.vertices)
        {
            skip_c++;

            if(skip_c % 10 != 0)
                continue;

            obj_cube_by_extents(&ctr, tex, {dim, dim, dim});

            std::vector<triangle> cube_tris = ctr.objs[0].tri_list;

            for(auto& i : cube_tris)
            {
                for(auto& k : i.vertices)
                {
                    k.set_pos(add(k.get_pos(), vert.get_pos()));
                    k.set_vt(vert.get_vt());
                }

                new_tris.push_back(i);
            }

            ctr.objs.clear();
        }
    }

    return new_tris;
}

///gamma correct mipmap filtering
///7ish pre tile deferred
///try first bounce in SS, then go to global if fail
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
    sponza->set_file("../openclrenderer/sp2/sp2.obj");
    sponza->set_active(true);
    //sponza->cache = false;

    engine window;

    window.load(1680,1050,1000, "turtles", "../openclrenderer/cl2.cl", true);

    lobj();

    window.set_camera_pos((cl_float4){-800,150,-570});

    //window.window.setPosition({-20, -20});

    //#ifdef OCULUS
    //window.window.setVerticalSyncEnabled(true);
    //#endif

    ///we need a context.unload_inactive
    context.load_active();

    sponza->set_specular(0.f);

    int io = 0;

    int acc = 0;

    for(object& o : sponza->objs)
    {
        o.tri_list = cube_by_area(o.tri_list);

        o.tri_num = o.tri_list.size();

        printf("Io %i %i\n", io++, sponza->objs.size());

        acc += o.tri_num;
    }

    printf("TNUM %i\n\n\n\n", acc);


    context.build(true);


    ///if that is uncommented, we use a metric tonne less memory (300mb)
    ///I do not know why
    ///it might be opencl taking a bad/any time to reclaim. Investigate

    auto object_dat = context.fetch();
    window.set_object_data(*object_dat);

    sf::Event Event;

    light l;
    l.set_col((cl_float4){1.0f, 1.0f, 1.0f, 0.0f});
    l.set_shadow_casting(1);
    l.set_brightness(1);
    l.radius = 100000;
    l.set_pos((cl_float4){-200, 2000, -100, 0});
    //l.set_godray_intensity(1.f);
    //window.add_light(&l);

    light::add_light(&l);

    auto light_data = light::build();
    ///

    window.set_light_data(light_data);
    window.construct_shadowmaps();


    printf("load_time %f\n", load_time.getElapsedTime().asMicroseconds() / 1000.f);
    sf::Keyboard key;


    float avg_ftime = 6000;


    ///use event callbacks for rendering to make blitting to the screen and refresh
    ///asynchronous to actual bits n bobs
    ///clSetEventCallback
    while(window.window.isOpen())
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
            ///make a enforce_screensize method, rather than make these hackily do it
            event = window.draw_bulk_objs_n(*context.fetch());

            window.increase_render_events();

            context.fetch()->swap_depth_buffers();
        }

        window.set_render_event(event);

        window.blit_to_screen(*context.fetch());

        window.flip();

        window.render_block();

        context.build_tick();

        avg_ftime += c.getElapsedTime().asMicroseconds();

        avg_ftime /= 2;

        if(key.isKeyPressed(sf::Keyboard::M))
            std::cout << c.getElapsedTime().asMicroseconds() << std::endl;

        if(key.isKeyPressed(sf::Keyboard::Comma))
            std::cout << avg_ftime << std::endl;
    }

    ///if we're doing async rendering on the main thread, then this is necessary
    window.render_block();
    cl::cqueue.finish();
    glFinish();
}
