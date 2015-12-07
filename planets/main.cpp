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
#include "../openclrenderer/noise.hpp"

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

struct planet_builder
{
    compute::buffer screen_buf = engine::make_screen_buffer(sizeof(cl_uint4));

    compute::buffer positions;
    compute::buffer colours;

    cl_int num = 10000;

    ///something approximating local connectivity
    std::array<int, 8> find_nearest_points(const std::vector<cl_float4>& pos, cl_float4 my_pos)
    {
        std::array<float, 8> found_distances;
        std::array<int, 8> found_nums;

        for(auto& i : found_distances)
            i = FLT_MAX;

        for(auto& i : found_nums)
            i = -1;

        for(int i=0; i<pos.size(); i++)
        {
            cl_float4 found = pos[i];

            if((found.x == my_pos.x) && (found.y == my_pos.y) && (found.z == my_pos.z))
                continue;

            float len = dist(found, my_pos);

            for(int j=0; j<8; j++)
            {
                if(len < found_distances[j])
                {
                    int found_location = j;

                    ///push all elements forwards one
                    for(int k=8-1; k>j; k--)
                    {
                        found_distances[k] = found_distances[k-1];
                        found_nums[k] = found_nums[k-1];
                    }

                    found_distances[found_location] = len;
                    found_nums[found_location] = i;


                    break;
                    //printf("hi %i %i %f\n", found_location, i, len);
                }
            }
        }

        return found_nums;
    }

    void load()
    {
        positions = compute::buffer(cl::context, sizeof(cl_float4)*num, CL_MEM_READ_ONLY, nullptr);
        colours = compute::buffer(cl::context, sizeof(cl_uint)*num, CL_MEM_READ_ONLY, nullptr);

        std::vector<cl_float4> pos;
        std::vector<cl_uint> col;
        std::vector<std::array<int, 8>> connections;

        ///really, this wants to be mapped pcie accessible memory set to overwrite, alas
        pos.reserve(num);
        col.reserve(num);
        connections.reserve(num);

        float offset = 2.f / num;

        constexpr float increment = M_PI * (3.f - sqrtf(5.f));

        int rnd = 1;

        float mul = 100.f;

        ///need to get latitude and longitude out so i can feed them into random number generators
        ///or actually, just use angles, yz, xz
        for(int i=0; i<num; i++)
        {
            float y = ((i * offset) - 1) + offset/2.f; ///?why plus offset/2
            float r = sqrtf(1.f - powf(y, 2.f));

            float phi = ((i + rnd) % num) * increment;

            float x = cosf(phi) * r;
            float z = sinf(phi) * r;

            pos.push_back({x, y, z, 0.f});
            col.push_back(0xFF00FFFF);
        }

        for(int i=0; i<num; i++)
        {
            connections.push_back(find_nearest_points(pos, pos[i]));
        }

        /*auto test_arr = find_nearest_points(pos, pos[9999]);

        for(auto& i : test_arr)
        {
            col[i] = 0xFFFF00FF;
        }*/

        for(int i=0; i<num; i++)
        {
            vec3f p = xyz_to_vec(pos[i]);

            float yz = atan2(p.v[1], p.v[2]);
            float xz = atan2(p.v[0], p.v[2]);

            float ly = yz;
            float lx = xz;

            if(xz > 0)
                lx = -xz;
            if(yz > 0)
                ly = -yz;

            lx += 4 * M_PI;
            ly += 4 * M_PI;

            yz += 4*M_PI;
            xz += 4*M_PI;

            float v1 = noisemult_2d(lx, ly, noise_2d, 0, 4, 1.);
            float v2 = noisemult_2d(xz + M_PI, yz + M_PI, noise_2d, -3, -2, 1.f);
            //v2 += noisemult_2d(xz + M_PI, yz + 0, noise_2d, -3, -2, 1.f);
            //v2 += noisemult_2d(xz + 0, yz + M_PI, noise_2d, -3, -2, 1.f);
            //v2 += noisemult_2d(lx + 0, ly + M_PI, noise_2d, -3, -2, 1.f);

            float val = v1*2 + v2 * 0.25f;

            val = (val + 1) / 2.f;

            //val = (val + 3) / (1 + 3);

            float rad = p.length();

            float nrad = rad * val;

            p = (nrad / rad) * p;

            pos[i] = {p.v[0], p.v[1], p.v[2]};
        }


        for(int i=0; i<num; i++)
        {
            cl_float4 mypos = pos[i];

            cl_float4 accum = {0};

            for(int j=0; j<8; j++)
            {
                cl_float4 neighbour = pos[connections[i][j]];

                accum = add(accum, neighbour);
            }

            mypos = div(add(mypos, accum), 9.f);

            pos[i] = mypos;
        }


        for(auto& i : pos)
        {
            i = mult(i, mul);
        }

        cl::cqueue.enqueue_write_buffer(positions, 0, sizeof(cl_float4)*num, &pos[0]);
        cl::cqueue.enqueue_write_buffer(colours, 0, sizeof(cl_uint)*num, &col[0]);
    }

    void tick(engine& window)
    {
        arg_list c_args;
        c_args.push_back(&screen_buf);

        run_kernel_with_string("clear_screen_buffer", {window.width * window.height}, {128}, 1, c_args);

        /*arg_list render_args;
        render_args.push_back(&num);
        render_args.push_back(&positions);
        render_args.push_back(&colours);
        render_args.push_back(&engine::c_pos);
        render_args.push_back(&engine::c_rot);
        render_args.push_back(&screen_buf);

        run_kernel_with_string("render_naive_points", {num}, {128}, 1, render_args);*/

        ///Need to fix the explosion code so I don't have to use this hack
        cl_float frac = 1/40.f;
        cl_float brightness = 1.f;

        arg_list gaussian_args;
        gaussian_args.push_back(&num);
        gaussian_args.push_back(&positions);
        //gaussian_args.push_back(&frac);
        //gaussian_args.push_back(&frac);
        gaussian_args.push_back(&colours);
        gaussian_args.push_back(&brightness);
        gaussian_args.push_back(&engine::c_pos);
        gaussian_args.push_back(&engine::c_rot);
        gaussian_args.push_back(&engine::c_pos);
        gaussian_args.push_back(&engine::c_rot);
        gaussian_args.push_back(&screen_buf);

        run_kernel_with_string("render_gaussian_normal", {num}, {128}, 1, gaussian_args);


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
    }
};

///do 2d electron simulation
///but then modify it to be able to do terrain generation
///?
int main(int argc, char *argv[])
{
    engine window;
    window.load(1000, 800, 1000, "SwordFight", "../openclrenderer/cl2.cl", true);

    window.set_camera_pos({0, 0, -200});

    planet_builder process;
    process.load();

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

        process.tick(window);

        window.render_block();
        window.display();
    }
}
