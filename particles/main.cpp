#include "../openclrenderer/proj.hpp"
#include "../openclrenderer/ocl.h"
#include "../openclrenderer/texture_manager.hpp"

#include "../openclrenderer/text_handler.hpp"
#include <sstream>
#include <string>
#include "../openclrenderer/vec.hpp"

#include "../openclrenderer/ui_manager.hpp"

#include "../openclrenderer/network.hpp"

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

struct particle_processor
{
    float start_friction = 1.f;
    float end_friction = 0.9f;

    const float end_time = 30000.f;
    const float fadeout_time = 3000.f;

    sf::Clock explosion_clock;

    compute::buffer screen_buf = engine::make_screen_buffer(sizeof(cl_uint4));

    /*///we want to change this to be an analytical solution rather than an iterative one
    ///so that we can just plug in frac and not have to keep multiple copies on the gpu
    void tick(engine& window, particle_vec& v1)
    {
        float explode_time = explosion_clock.getElapsedTime().asMicroseconds() / (1000.f);

        float frac = explode_time / end_time;

        if(frac > 1)
            frac = 1;

        frac = pow(frac, 1.1);

        float friction = (1.f - frac) * start_friction + frac * end_friction;

        arg_list c_args;
        c_args.push_back(&screen_buf);

        run_kernel_with_string("clear_screen_buffer", {window.width * window.height}, {128}, 1, c_args);

        process_pvec(v1, friction, screen_buf, 1.f - clamp(explode_time / fadeout_time, 0.f, 1.f));

        arg_list b_args;
        b_args.push_back(&engine::g_screen);
        b_args.push_back(&screen_buf);

        run_kernel_with_string("blit_unconditional", {window.width * window.height}, {128}, 1, b_args);

        ///whole 3d rendering system is so broken i don't even
        ///there just isn't a proper way to blit textures atm
        ///whole async thing is just rubbish
        ///though we do get 1ms faster out of it so who am I to judge
        window.old_pos = window.c_pos;
        window.old_rot = window.c_rot;

        window.render_me = true;
        window.current_frametype = frametype::RENDER;
    }*/

    void tick(engine& window, particle_vec& v1)
    {
        float explode_time = explosion_clock.getElapsedTime().asMicroseconds() / (1000.f);

        float frac = explode_time / fadeout_time;
        static float old_frac = -0.1f;

        //if(frac > 1)
        //    frac = 1;

        float min_bound = 0.5f;

        frac += min_bound;

        frac = log2(frac) - log2(min_bound);

        frac = frac / (log2(1.f + min_bound) - log2(min_bound));

        //printf("%f\n", frac);

        //frac = pow(frac, 1.1);

        //frac = powf(frac, 0.4f);

        //printf("%f\n", frac);

        //float friction = (1.f - frac) * start_friction + frac * end_friction;

        arg_list c_args;
        c_args.push_back(&screen_buf);

        run_kernel_with_string("clear_screen_buffer", {window.width * window.height}, {128}, 1, c_args);

        process_pvec(v1, frac, old_frac, screen_buf, 1.f - clamp(explode_time / fadeout_time, 0.f, 1.f));

        arg_list b_args;
        b_args.push_back(&engine::g_screen);
        b_args.push_back(&screen_buf);

        run_kernel_with_string("blit_unconditional", {window.width * window.height}, {128}, 1, b_args);

        ///whole 3d rendering system is so broken i don't even
        ///there just isn't a proper way to blit textures atm
        ///whole async thing is just rubbish
        ///though we do get 1ms faster out of it so who am I to judge
        window.old_pos = window.c_pos;
        window.old_rot = window.c_rot;

        window.render_me = true;
        window.current_frametype = frametype::RENDER;

        old_frac = frac;
    }

    ///does not work due to me being an idiot
    void restart()
    {
        explosion_clock.restart();
    }
};

int main(int argc, char *argv[])
{
    engine window;
    window.load(1000, 800, 1000, "SwordFight", "../openclrenderer/cl2.cl", true);

    window.set_camera_pos({-0.17, -94.6033, -1500.804});
    window.set_camera_rot({0, 0, 0});

    vec3f red = {0.90, 0.55, 0.175};
    vec3f blue = {0.6, 0.6, 1.f};

    float rangle = M_PI/32.f;
    float bangle = (M_PI/4 - rangle);

    float fill_angle = (M_PI - bangle) / 2;

    float rspeed = 0.987f;
    float bspeed = 0.99f;

    int rnum = 80;
    int bnum = 40;

    particle_intermediate i1 =
               make_particle_jet(rnum, {0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, 100.f, rangle, red, rspeed);
    i1.append( make_particle_jet(rnum, {0.f, 0.f, 0.f}, {0.f, -1.f, 0.f}, 100.f, rangle, red, rspeed) );
    i1.append( make_particle_jet(rnum, {0.f, 0.f, 0.f}, {-1.f, 0.f, 0.f}, 100.f, rangle, red, rspeed) );
    i1.append( make_particle_jet(rnum, {0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, 100.f, rangle, red, rspeed) );
    i1.append( make_particle_jet(rnum, {0.f, 0.f, 0.f}, {0.f, 0.f, -1.f}, 100.f, rangle, red, rspeed) );
    i1.append( make_particle_jet(rnum, {0.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, 100.f, rangle, red, rspeed) );


    i1.append( make_particle_jet(bnum, {0.f, 0.f, 0.f}, {1.f, 1.f, 0.f}, 100.f, bangle, blue, bspeed) );
    i1.append( make_particle_jet(bnum, {0.f, 0.f, 0.f}, {1.f, -1.f, 0.f}, 100.f, bangle, blue, bspeed) );
    i1.append( make_particle_jet(bnum, {0.f, 0.f, 0.f}, {-1.f, -1.f, 0.f}, 100.f, bangle, blue, bspeed) );
    i1.append( make_particle_jet(bnum, {0.f, 0.f, 0.f}, {-1.f, 1.f, 0.f}, 100.f, bangle, blue, bspeed) );

    i1.append( make_particle_jet(bnum, {0.f, 0.f, 0.f}, {0, 0, 1.f}, 100.f, fill_angle, blue, bspeed) );
    i1.append( make_particle_jet(bnum, {0.f, 0.f, 0.f}, {0, 0, -1.f}, 100.f, fill_angle, blue, bspeed) );

    particle_vec v1 = i1.build();

    particle_processor process;

    sf::Mouse mouse;
    sf::Keyboard key;

    cl::cqueue.finish();
    cl::cqueue2.finish();

    while(window.window.isOpen())
    {
        sf::Clock c;

        sf::Event Event;

        while(window.window.pollEvent(Event))
        {
            if(Event.type == sf::Event::Closed)
                window.window.close();
        }

        process.tick(window, v1);

        if(once<sf::Keyboard::Q>())
        {
            process.restart();
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

        window.render_block();

        window.display();


        //std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }
}
