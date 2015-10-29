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

struct particle_info
{
    float density;
    float temp;
};

struct particle_vec
{
    std::vector<cl_float4> p1;
    std::vector<cl_float4> p2;
    std::vector<cl_uint> colours;

    compute::buffer bufs[2];
    compute::buffer g_col;

    int which;
    int nwhich;

    cl_int num;

    particle_vec(std::vector<cl_float4> v1, std::vector<cl_float4> v2, std::vector<cl_uint> c1)
    {
        p1 = v1;
        p2 = v2;
        colours = c1;

        bufs[0] = engine::make_read_write(sizeof(cl_float4)*p1.size(), p1.data());
        bufs[1] = engine::make_read_write(sizeof(cl_float4)*p2.size(), p2.data());

        g_col =  engine::make_read_write(sizeof(cl_uint)*colours.size(), colours.data());

        which = 0;
        nwhich = 1;

        num = v1.size();
    }

    void flip()
    {
        nwhich = which;

        which = (which + 1) % 2;
    }
};

struct particle_intermediate
{
    std::vector<cl_float4> p1;
    std::vector<cl_float4> p2;
    std::vector<cl_uint> colours;

    particle_intermediate append(const particle_intermediate& n)
    {
        p1.insert(p1.end(), n.p1.begin(), n.p1.end());
        p2.insert(p2.end(), n.p2.begin(), n.p2.end());
        colours.insert(colours.end(), n.colours.begin(), n.colours.end());

        return *this;
    }

    particle_vec build()
    {
        return particle_vec(p1, p2, colours);
    }
};

particle_intermediate make_particle_jet(int num, vec3f start, vec3f dir, float len, float angle, vec3f col, float speedmod)
{
    std::vector<cl_float4> p1;
    std::vector<cl_float4> p2;
    std::vector<cl_uint> colours;

    for(uint32_t i = 0; i<num; i++)
    {
        float len_pos = randf_s(0, len);

        float len_frac = len_pos / len;

        vec3f euler = dir.get_euler();

        euler = euler + (vec3f){0, randf_s(-angle, angle), randf_s(-angle, angle)};

        vec3f rot_pos = (vec3f){0.f, len_pos, 0.f}.rot({0.f, 0.f, 0.f}, euler);

        vec3f final_pos = start + rot_pos;

        final_pos = final_pos + randf<3, float>(-len/40.f, len/40.f);


        float mod = speedmod;

        p1.push_back({final_pos.v[0], final_pos.v[1], final_pos.v[2]});

        vec3f pos_2 = final_pos * mod;

        p2.push_back({pos_2.v[0], pos_2.v[1], pos_2.v[2]});

        col = clamp(col, 0.f, 1.f);

        colours.push_back(rgba_to_uint(col));
    }

    return particle_intermediate{p1, p2, colours};
}

void process_pvec(particle_vec& v1, float friction, compute::buffer& screen_buf, float frac_remaining)
{
    cl_float brightness = frac_remaining * 3.f + (1.f - frac_remaining) * 0.0f;

    //brightness = sqrtf(brightness);

    //printf("%f\n", brightness);

    int which = v1.which;
    int nwhich = v1.nwhich;

    cl_int num = v1.num;

    arg_list p_args;
    p_args.push_back(&num);
    p_args.push_back(&v1.bufs[which]);
    p_args.push_back(&v1.bufs[nwhich]);
    p_args.push_back(&friction);

    run_kernel_with_string("particle_explode", {num}, {128}, 1, p_args);

    arg_list r_args;
    r_args.push_back(&num);
    r_args.push_back(&v1.bufs[nwhich]);
    r_args.push_back(&v1.bufs[which]);
    r_args.push_back(&v1.g_col);
    r_args.push_back(&brightness);
    r_args.push_back(&engine::c_pos);
    r_args.push_back(&engine::c_rot);
    r_args.push_back(&engine::old_pos);
    r_args.push_back(&engine::old_rot);
    r_args.push_back(&screen_buf);

    ///render a 2d gaussian for particle effects
    run_kernel_with_string("render_gaussian_points", {num}, {128}, 1, r_args);

    v1.flip();
}

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

    compute::buffer screen_buf = engine::make_screen_buffer(sizeof(cl_uint4));

    sf::Mouse mouse;
    sf::Keyboard key;

    int which = 0;
    int nwhich = 1;

    float start_friction = 1.f;
    float end_friction = 0.9f;

    const float end_time = 30000.f;
    const float fadeout_time = 3000.f;

    //cl::cqueue.finish();

    sf::Clock explosion_clock;

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

        float explode_time = explosion_clock.getElapsedTime().asMicroseconds() / (1000.f);

        float frac = explode_time / end_time;

        if(frac > 1)
            frac = 1;

        //frac *= frac;// * frac;

        frac = pow(frac, 1.1);

        float friction = (1.f - frac) * start_friction + frac * end_friction;

        //window.input();

        arg_list c_args;
        c_args.push_back(&screen_buf);

        run_kernel_with_string("clear_screen_buffer", {window.width * window.height}, {128}, 1, c_args);

        process_pvec(v1, friction, screen_buf, 1.f - clamp(explode_time / fadeout_time, 0.f, 1.f));

        arg_list b_args;
        b_args.push_back(&engine::g_screen);
        b_args.push_back(&screen_buf);

        run_kernel_with_string("blit_unconditional", {window.width * window.height}, {128}, 1, b_args);

        window.old_pos = window.c_pos;
        window.old_rot = window.c_rot;

        window.render_me = true;
        window.current_frametype = frametype::RENDER;

        window.render_block();

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

        window.display();


        std::cout << c.getElapsedTime().asMicroseconds() << std::endl;

        nwhich = which;
        which = (which + 1) % 2;
    }
}
