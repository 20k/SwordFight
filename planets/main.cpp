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

    std::vector<cl_float4> saved_pos;
    std::vector<std::array<int, 8>> saved_conn;

    cl_int num = 10000;

    vec3f get_fibonnacci_position(int id, int num)
    {

    }

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

    std::vector<triangle> get_tris(const std::vector<cl_float4>& pos, const std::vector<std::array<int, 8>>& connections)
    {
        std::vector<int> visited;

        ///fill with 0s
        visited.resize(pos.size());

        std::vector<triangle> tris;

        for(int i=0; i<pos.size(); i++)
        {
            cl_float4 my_pos = pos[i];
            std::array<int, 8> my_connections = connections[i];

            std::vector<std::pair<float, int>> connection_angles;

            if(visited[i])
                continue;

            vec3f vec = xyz_to_vec(my_pos);
            vec3f euler = vec.get_euler();

            ///get_euler gives us angle from +y upwards as the vertial axis
            ///so doing back rotate gives us this point centered on this line
            ///which means that x and z are 2d plane components
            ///and y is the depth

            vec3f centre_point = vec.back_rot(0.f, euler);

            ///xz
            vec2f centre_2d = (vec2f){centre_point.v[0], centre_point.v[2]};

            ///need to sort connections anticlockwise about mutual plane. Or something

            for(int j=0; j<8; j++)
            {
                cl_float4 conn_pos = pos[connections[i][j]];

                vec3f vec_pos = xyz_to_vec(conn_pos);

                vec3f rotated = vec_pos.back_rot(0.f, euler);

                vec2f rot_2d = (vec2f){rotated.v[0], rotated.v[2]};

                vec2f rel = rot_2d - centre_2d;

                float angle = rel.angle();

                connection_angles.push_back({angle, connections[i][j]});

                //printf("%f %f %f\n", EXPAND_3(rotated));
            }

            std::sort(connection_angles.begin(), connection_angles.end(),
                      [](auto i1, auto i2)
                      {
                          return i1.first < i2.first;
                      }
                      );

            ///connection angles now sorted anticlockwise

            for(int j=0; j<8; j++)
            {
                int nxt = (j + 1) % 8;

                triangle tri;
                tri.vertices[0].set_pos(pos[connection_angles[nxt].second]);
                tri.vertices[1].set_pos(pos[connection_angles[j].second]);
                tri.vertices[2].set_pos(my_pos);

                tri.vertices[0].set_vt({0.7, 0.1});
                tri.vertices[1].set_vt({0.1, 0.1});
                tri.vertices[2].set_vt({0.5, 0.5});

                vec3f flat_normal = generate_flat_normal(xyz_to_vec(tri.vertices[0].get_pos()),
                                                         xyz_to_vec(tri.vertices[1].get_pos()),
                                                         xyz_to_vec(tri.vertices[2].get_pos()));

                tri.vertices[0].set_normal({flat_normal.v[0], flat_normal.v[1], flat_normal.v[2]});
                tri.vertices[1].set_normal({flat_normal.v[0], flat_normal.v[1], flat_normal.v[2]});
                tri.vertices[2].set_normal({flat_normal.v[0], flat_normal.v[1], flat_normal.v[2]});

                visited[connection_angles[j].second] = 1;

                tris.push_back(tri);

                //printf("%f %f %f\n", tri.vertices[0].get_pos().x, tri.vertices[0].get_pos().y, tri.vertices[0].get_pos().z);
                //printf("%f %f %f\n", tri.vertices[1].get_pos().x, tri.vertices[1].get_pos().y, tri.vertices[1].get_pos().z);
                //printf("%f %f %f\n", tri.vertices[2].get_pos().x, tri.vertices[2].get_pos().y, tri.vertices[2].get_pos().z);
            }

        }

        return tris;
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

        saved_pos = pos;
        saved_conn = connections;

        //get_tris(pos, connections);

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

void load_asteroid(objects_container* pobj)
{
    if(pobj == nullptr)
        throw std::runtime_error("broken severely in load asteroid, nullptr obj");

    planet_builder planet;
    planet.load();

    ///bad code central
    auto tris = planet.get_tris(planet.saved_pos, planet.saved_conn);

    printf("Tri num: %i\n", tris.size());

    texture tex;
    tex.type = 0;
    tex.set_texture_location("res/red.png");
    tex.push();

    object obj;
    obj.tri_list = tris;
    obj.tri_num = obj.tri_list.size();

    obj.tid = tex.id;
    obj.has_bump = 0;

    obj.pos = {0,0,0,0};
    obj.rot = {0,0,0,0};

    obj.isloaded = true;

    pobj->objs.push_back(obj);

    pobj->isloaded = true;
}

///do 2d electron simulation
///but then modify it to be able to do terrain generation
///?
int main(int argc, char *argv[])
{
    engine window;
    window.load(1000, 800, 1000, "SwordFight", "../openclrenderer/cl2.cl", true);

    window.set_camera_pos({0, 0, -200});

    object_context context;

    auto asteroid = context.make_new();
    asteroid->set_load_func(load_asteroid);
    //asteroid->set_file("../openclrenderer/sp2/sp2.obj");
    asteroid->set_active(true);
    asteroid->cache = false;

    context.load_active();

    texture_manager::allocate_textures();

    auto tex_gpu = texture_manager::build_descriptors();
    window.set_tex_data(tex_gpu);

    context.build();
    auto object_dat = context.fetch();
    window.set_object_data(*object_dat);


    light l;
    l.set_col((cl_float4){1.0f, 1.0f, 1.0f, 0.0f});
    l.set_shadow_casting(0);
    l.set_brightness(1);
    l.radius = 100000;
    l.set_pos((cl_float4){-200, 5000, -100, 0});

    light::add_light(&l);

    auto light_data = light::build();

    window.set_light_data(light_data);


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

        window.draw_bulk_objs_n();

        //process.tick(window);

        window.render_block();
        window.display();

        std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }
}
