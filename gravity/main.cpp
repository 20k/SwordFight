#include "../openclrenderer/proj.hpp"
#include "../openclrenderer/ocl.h"

#include "../openclrenderer/text_handler.hpp"
#include <sstream>
#include <string>
#include "../openclrenderer/vec.hpp"

#include <vec/vec.hpp>

#include "gpu_particles.hpp"

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

struct render_info
{
    compute::buffer screen;
    compute::buffer bufs_p[2];
    compute::buffer bufs_v[2];
    compute::buffer bufs_a[2];
    int which;

    cl_int num;

    void load(int _num)
    {
        num = _num;

        which = 0;

        bufs_p[0] = engine::make_read_write(sizeof(cl_float4) * num);
        bufs_p[1] = engine::make_read_write(sizeof(cl_float4) * num);

        bufs_v[0] = engine::make_read_write(sizeof(cl_float4) * num);
        bufs_v[1] = engine::make_read_write(sizeof(cl_float4) * num);

        bufs_a[0] = engine::make_read_write(sizeof(cl_float4) * num);
        bufs_a[1] = engine::make_read_write(sizeof(cl_float4) * num);

        screen = engine::make_screen_buffer(sizeof(cl_uint4));

        std::vector<cl_float4> cur, old;
        std::vector<cl_float4> cur_v, old_v;
        std::vector<cl_float4> cur_a, old_a;

        for(int i=0; i<num; i++)
        {
            float rad = 1000.f;

            vec3f pos = randf<3, float>(-rad, rad);

            ///I cannot be arsed to actually solve this
            while(pos.length() > rad)
            {
                pos = randf<3, float>(-rad, rad);
            }

            ///want to introduce spin in the z, x plane

            vec2f plane = {pos.v[0], pos.v[2]};

            float angle_change = 0.0001f;//0.004f;

            float angle = plane.angle();

            float new_angle = angle - angle_change;

            float len = plane.length();

            vec2f new_xz = (vec2f){len * cos(new_angle), len * sin(new_angle)};

            vec3f last_pos = pos;// * 0.99999f;

            last_pos.v[0] = new_xz.v[0];
            last_pos.v[2] = new_xz.v[1];

            cur.push_back({pos.v[0], pos.v[1], pos.v[2]});
            old.push_back({last_pos.v[0], last_pos.v[1], last_pos.v[2]});

            vec3f diff = pos - last_pos;

            cur_v.push_back({diff.v[0], diff.v[1], diff.v[2]});
            old_v.push_back({diff.v[0], diff.v[1], diff.v[2]});

            cur_a.push_back({0.f, 0.f, 0.f});
            old_a.push_back({0.f, 0.f, 0.f});
        }

        cl::cqueue.enqueue_write_buffer(bufs_p[0], 0, sizeof(cl_float4) * num, &cur[0]);
        cl::cqueue.enqueue_write_buffer(bufs_p[1], 0, sizeof(cl_float4) * num, &old[0]);

        cl::cqueue.enqueue_write_buffer(bufs_v[0], 0, sizeof(cl_float4) * num, &cur_v[0]);
        cl::cqueue.enqueue_write_buffer(bufs_v[1], 0, sizeof(cl_float4) * num, &old_v[0]);

        cl::cqueue.enqueue_write_buffer(bufs_a[0], 0, sizeof(cl_float4) * num, &cur_a[0]);
        cl::cqueue.enqueue_write_buffer(bufs_a[1], 0, sizeof(cl_float4) * num, &old_a[0]);
    }

    void tick()
    {
        arg_list gar;

        gar.push_back(&num);
        gar.push_back(&bufs_p[which]);
        gar.push_back(&bufs_p[(which + 1) % 2]);
        gar.push_back(&bufs_v[which]);
        gar.push_back(&bufs_v[(which + 1) % 2]);
        gar.push_back(&bufs_a[which]);
        gar.push_back(&bufs_a[(which + 1) % 2]);
        gar.push_back(&engine::c_pos);
        gar.push_back(&engine::c_rot);
        gar.push_back(&screen);

        run_kernel_with_string("gravity_alt_first", {num}, {128}, 1, gar);

        arg_list gar2;

        gar2.push_back(&num);
        gar2.push_back(&bufs_p[(which + 1) % 2]);
        gar2.push_back(&bufs_p[which]);

        gar2.push_back(&bufs_v[(which + 1) % 2]);
        gar2.push_back(&bufs_v[which]);

        gar2.push_back(&bufs_a[(which + 1) % 2]);
        gar2.push_back(&bufs_a[which]);

        gar2.push_back(&engine::c_pos);
        gar2.push_back(&engine::c_rot);
        gar2.push_back(&screen);

        run_kernel_with_string("gravity_alt_render", {num}, {128}, 1, gar2);

        //which = (which + 1) % 2;
    }
};


int main(int argc, char *argv[])
{
    engine window;
    window.load(1000, 800, 1000, "SwordFight", "../openclrenderer/cl2.cl", true);

    window.set_camera_pos({-0.17, -94.6033, -1500.804});
    window.set_camera_rot({0, 0, 0});

    sf::Mouse mouse;
    sf::Keyboard key;

    render_info inf;
    inf.load(10000);


    while(window.window.isOpen())
    {
        sf::Clock c;

        sf::Event Event;

        while(window.window.pollEvent(Event))
        {
            if(Event.type == sf::Event::Closed)
                window.window.close();
        }

        /*int mx = window.get_mouse_x();
        int my = window.get_mouse_y();

        if(mx >= 0 && my >= 0 && mx < window.width && my < window.height)
        {
            cl_uint4 res;

            my = window.height - my;

            clEnqueueReadBuffer(cl::cqueue.get(), screen_buf.get(), CL_TRUE, (my*window.width + mx) * sizeof(cl_uint4), sizeof(cl_uint4), &res, 0, nullptr, nullptr);

            cl::cqueue.finish();

            printf("%i %i %i\n", res.x, res.y, res.z);
        }*/

        arg_list c_args;
        c_args.push_back(&inf.screen);

        run_kernel_with_string("clear_screen_buffer", {window.width * window.height}, {128}, 1, c_args);

        inf.tick();

        arg_list b_args;
        b_args.push_back(&engine::g_screen);
        b_args.push_back(&inf.screen);

        run_kernel_with_string("blit_unconditional", {window.width * window.height}, {128}, 1, b_args);

        window.old_pos = window.c_pos;
        window.old_rot = window.c_rot;

        window.render_me = true;
        window.current_frametype = frametype::RENDER;

        window.display();
        window.render_block();

        std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }
}
