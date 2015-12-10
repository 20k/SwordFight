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
    std::vector<std::vector<int>> saved_conn;

    cl_int num = 10000;

    cl_float4 get_fibonnacci_position(int id, int num)
    {
        float offset = 2.f / num;

        constexpr float increment = M_PI * (3.f - sqrtf(5.f));

        int rnd = 1;

        float y = ((id * offset) - 1) + offset/2.f; ///?why plus offset/2
        float r = sqrtf(1.f - std::min(powf(y, 2.f), 1.f));

        float phi = ((id + rnd) % num) * increment;

        float x = cosf(phi) * r;
        float z = sinf(phi) * r;

        return {x, y, z, 0};
    }

    std::vector<int> get_nearest_n(const std::vector<cl_float4>& pos, int my_id, int N, std::vector<int> exclude = std::vector<int>())
    {
        cl_float4 my_pos = pos[my_id];

        std::vector<float> found_distances;
        std::vector<int> found_nums;

        found_distances.resize(N);
        found_nums.resize(N);

        for(auto& i : found_distances)
            i = FLT_MAX;

        for(auto& i : found_nums)
            i = -1;

        for(int i=0; i<pos.size(); i++)
        {
            cl_float4 found = pos[i];

            if(i == my_id)
                continue;

            bool skip = false;

            for(auto& j : exclude)
            {
                if(i == j)
                    skip = true;
            }

            if(skip)
                continue;

            float len = dist(found, my_pos);

            for(int j=0; j<N; j++)
            {
                if(len < found_distances[j])
                {
                    int found_location = j;

                    ///push all elements forwards one
                    for(int k=N-1; k>j; k--)
                    {
                        found_distances[k] = found_distances[k-1];
                        found_nums[k] = found_nums[k-1];
                    }

                    found_distances[found_location] = len;
                    found_nums[found_location] = i;

                    break;
                }
            }
        }


        return found_nums;
    }

    ///something approximating local connectivity
    ///hmm. We want to maximise distance between points
    ///while staying within the other constraint of being minimal to the centre
    ///or we could do an angle constraint based version
    ///divide up into 8 slots, and then assign to them based on angle/distance min
    ///hmm
    ///we could assume that nearest 4 are always correct (reasonable), then discover the other's based
    ///off this
    ///average two vectors together to find vec -> corner, then find nearest to that
    ///within angle tolerance
    ///might not work for the really irregular bits though
    std::vector<int> find_nearest_points(const std::vector<cl_float4>& pos, int my_id, std::vector<int> exclude = std::vector<int>())
    {
        return get_nearest_n(pos, my_id, 8, exclude);
    }

    template<int N>
    std::vector<triangle> get_tris(const std::vector<cl_float4>& pos, const std::vector<std::vector<int>>& connections)
    {
        std::vector<int> visited;

        ///fill with 0s
        visited.resize(pos.size());

        ///probably want to run a smoothing pass over this or something
        std::vector<vec3f> smoothed_normals;
        smoothed_normals.reserve(pos.size());

        //for(auto& i : pos)
        for(int i=0; i<pos.size(); i++)
        {
            std::vector<int> my_connections = connections[i];

            std::vector<vec3f> in_connections;
            in_connections.reserve(my_connections.size());

            for(int j=0; j<my_connections.size(); j++)
            {
                int c = my_connections[j];

                in_connections.push_back({pos[c].x, pos[c].y, pos[c].z});
            }

            vec3f my_pos = {pos[i].x, pos[i].y, pos[i].z};

            std::vector<vec3f> sorted = sort_anticlockwise(in_connections, my_pos);

            vec3f normal_accum = 0.f;

            for(int j=0; j<sorted.size(); j++)
            {
                int nxt = (j + 1) % sorted.size();

                vec3f flat_normal = generate_flat_normal(sorted[nxt], sorted[j], my_pos);

                normal_accum += flat_normal;
            }

            normal_accum = normal_accum / (float)sorted.size();

            smoothed_normals.push_back(normal_accum);
        }

        std::vector<triangle> tris;

        for(int i=0; i<pos.size(); i++)
        {
            cl_float4 my_pos = pos[i];
            std::vector<int> my_connections = connections[i];

            //if(visited[i])
            //    continue;

            std::vector<vec3f> in_connections;

            //for(auto& j : my_connections)
            for(int j=0; j<my_connections.size(); j++)
            {
                int c = my_connections[j];

                in_connections.push_back({pos[c].x, pos[c].y, pos[c].z});
            }

            std::vector<std::pair<float, int>> out_ids;

            std::vector<vec3f> sorted = sort_anticlockwise(in_connections, {my_pos.x, my_pos.y, my_pos.z}, &out_ids);

            ///connection angles now sorted anticlockwise

            for(int j=0; j<sorted.size(); j++)
            {
                int nxt = (j + 1) % sorted.size();

                triangle tri;
                tri.vertices[0].set_pos({sorted[nxt].v[0], sorted[nxt].v[1], sorted[nxt].v[2]});
                tri.vertices[1].set_pos({sorted[j].v[0], sorted[j].v[1], sorted[j].v[2]});
                tri.vertices[2].set_pos(my_pos);

                tri.vertices[0].set_vt({0.7, 0.1});
                tri.vertices[1].set_vt({0.1, 0.1});
                tri.vertices[2].set_vt({0.5, 0.5});

                /*vec3f flat_normal = generate_flat_normal(xyz_to_vec(tri.vertices[0].get_pos()),
                                                         xyz_to_vec(tri.vertices[1].get_pos()),
                                                         xyz_to_vec(tri.vertices[2].get_pos()));

                tri.vertices[0].set_normal({flat_normal.v[0], flat_normal.v[1], flat_normal.v[2]});
                tri.vertices[1].set_normal({flat_normal.v[0], flat_normal.v[1], flat_normal.v[2]});
                tri.vertices[2].set_normal({flat_normal.v[0], flat_normal.v[1], flat_normal.v[2]});*/

                int id_1 = my_connections[out_ids[nxt].second];
                int id_2 = my_connections[out_ids[j].second];
                int id_3 = i;

                tri.vertices[0].set_normal({smoothed_normals[id_1].v[0], smoothed_normals[id_1].v[1], smoothed_normals[id_1].v[2]});
                tri.vertices[1].set_normal({smoothed_normals[id_2].v[0], smoothed_normals[id_2].v[1], smoothed_normals[id_2].v[2]});
                tri.vertices[2].set_normal({smoothed_normals[id_3].v[0], smoothed_normals[id_3].v[1], smoothed_normals[id_3].v[2]});


                //visited[my_connections[j]] = 1;

                ///we're double creating triangles atm
                ///need to use the visited step after we've processed outselves, then skip
                ///tris that are that
                tris.push_back(tri);

                //printf("%f %f %f\n", tri.vertices[0].get_pos().x, tri.vertices[0].get_pos().y, tri.vertices[0].get_pos().z);
                //printf("%f %f %f\n", tri.vertices[1].get_pos().x, tri.vertices[1].get_pos().y, tri.vertices[1].get_pos().z);
                //printf("%f %f %f\n", tri.vertices[2].get_pos().x, tri.vertices[2].get_pos().y, tri.vertices[2].get_pos().z);
            }

            visited[i] = 1;
        }

        return tris;
    }

    std::vector<int> replace_ref(const std::vector<int>& in, int orig, int change)
    {
        std::vector<int> ret;

        for(auto& i : in)
        {
            if(i != orig)
                ret.push_back(i);
            else
                ret.push_back(change);
        }

        return ret;
    }

    std::tuple<std::vector<cl_float4>, std::vector<std::vector<int>>>
    subdivide(const std::vector<cl_float4>& pos, const std::vector<std::vector<int>>& connections, float rad)
    {
        std::vector<cl_float4> new_pos;
        std::vector<std::vector<int>> new_connections;

        new_pos.reserve(pos.size()*5);
        new_connections.reserve(pos.size()*5);

        std::vector<int> visited;
        visited.resize(pos.size());

        std::vector<int> near_num;

        for(int i=0; i<pos.size(); i++)
        {
            cl_float4 my_pos = pos[i];
            std::vector<int> my_connections = connections[i];

            std::vector<vec3f> in_connections;

            for(int j=0; j<my_connections.size(); j++)
            {
                int c = my_connections[j];

                if(visited[c])
                    continue;

                in_connections.push_back({pos[c].x, pos[c].y, pos[c].z});
            }

            //std::vector<std::pair<float, int>> pass_out;

            ///alas, the sorting is useless
            //std::vector<vec3f> sorted = sort_anticlockwise(in_connections, {my_pos.x, my_pos.y, my_pos.z}, &pass_out);

            ///connection angles now sorted anticlockwise

            //if(i != 0)
            //    continue;

            for(int j=0; j<in_connections.size(); j++)
            {
                int nxt = (j + 1) % in_connections.size();

                ///remember to radius normalise d1/2/3
                //vec3f p1 = in_connections[nxt];
                //vec3f p2 = in_connections[j];
                //vec3f p3 = xyz_to_vec(my_pos);

                vec3f p1 = in_connections[j];
                vec3f p2 = xyz_to_vec(my_pos);

                vec3f d1 = (p1 + p2) / 2.f;
                //vec3f d2 = (p2 + p3) / 2.f;
                //vec3f d3 = (p3 + p1) / 2.f;

                //new_pos.push_back({p1.v[0], p1.v[1], p1.v[2]});
                //new_pos.push_back({p2.v[0], p2.v[1], p2.v[2]});
                //new_pos.push_back({p3.v[0], p3.v[1], p3.v[2]});

                float old_rad = d1.length();

                d1 = d1 * rad / old_rad;

                new_pos.push_back({d1.v[0], d1.v[1], d1.v[2]});
                near_num.push_back(6);
                //new_pos.push_back({d2.v[0], d2.v[1], d2.v[2]});
                //new_pos.push_back({d3.v[0], d3.v[1], d3.v[2]});

                //int p1_id = in_ids[pass_out[nxt].second];
                //int p2_id = in_ids[pass_out[j].second];
                //int p3_id = i;

                //visited[p1_id] = 1;
                //visited[p2_id] = 1;
                //visited[p3_id] = 1;

                /*int d1_id = next_free_id++;
                int d2_id = next_free_id++;
                int d3_id = next_free_id++;

                new_connections[p1_id] = replace_ref(new_connections, p2_id, )*/

                //printf("%f %f %f\n", tri.vertices[0].get_pos().x, tri.vertices[0].get_pos().y, tri.vertices[0].get_pos().z);
                //printf("%f %f %f\n", tri.vertices[1].get_pos().x, tri.vertices[1].get_pos().y, tri.vertices[1].get_pos().z);
                //printf("%f %f %f\n", tri.vertices[2].get_pos().x, tri.vertices[2].get_pos().y, tri.vertices[2].get_pos().z);
            }

            new_pos.push_back(my_pos);

            if(connections[i].size() == 5)
                near_num.push_back(5);
            else
                near_num.push_back(6);

            visited[i] = 1;
        }

        ///need to deal with staggered 5/6 problem
        for(int i=0; i<new_pos.size(); i++)
        {
            new_connections.push_back(get_nearest_n(new_pos, i, near_num[i]));
        }

        return std::make_tuple(new_pos, new_connections);
    }

    void load()
    {

        //constexpr int nearest = 5;

        std::vector<cl_float4> pos;
        std::vector<cl_uint> col;
        std::vector<std::vector<int>> connections;

        ///really, this wants to be mapped pcie accessible memory set to overwrite, alas
        pos.reserve(num);
        col.reserve(num);
        connections.reserve(num);

        float tao = 1.61803399f;

        ///http://donhavey.com/blog/tutorials/tutorial-3-the-icosahedron-sphere/
        vec3f icosahedron[] = {{1.f, tao, 0.f}, {-1.f, tao, 0.f}, {1.f, -tao, 0.f}, {-1.f, -tao, 0.f},
                              {0,1,tao},{0,-1,tao},{0,1,-tao},{0,-1,-tao},
                              {tao,0,1},{-tao,0,1},{tao,0,-1},{-tao,0,-1}};

        float mul = 100.f;

        ///need to get latitude and longitude out so i can feed them into random number generators
        ///or actually, just use angles, yz, xz
        ///maybe I should do one iteration of electron repulsion on this?
        ///then again, that ruins all my guarantees
        //for(int i=0; i<num; i++)
        for(int i=0; i<12; i++)
        {
            //pos.push_back(get_fibonnacci_position(i, num));
            pos.push_back({icosahedron[i].v[0], icosahedron[i].v[1], icosahedron[i].v[2]});
        }

        for(int i=0; i<num; i++)
        {
            connections.push_back(get_nearest_n(pos, i, 5));
        }

        float rad = sqrtf(1*1 + tao*tao);

        int subdivision_nums = 5;

        for(int i=0; i<subdivision_nums; i++)
            std::tie(pos, connections) = subdivide(pos, connections, rad);

        /*saved_pos = pos;

        for(auto& i : saved_pos)
        {
            i = mult(i, 100.f);
        }*/

        /*for(int i=0; i<10; i++)
        {
            auto ret = do_repulse(pos);

            std::swap(ret, pos);
        }*/

        //connections = stable_merge(connections, fixed_connections);

        /*auto test_arr = find_nearest_points(pos, pos[9999]);

        for(auto& i : test_arr)
        {
            col[i] = 0xFFFF00FF;
        }*/

        for(int i=0; i<pos.size(); i++)
        {
            vec3f p = xyz_to_vec(pos[i]);

            float yz = atan2(p.v[1], p.v[2]);
            float xz = atan2(p.v[0], p.v[2]);

            float lx = xz;
            float ly = yz;

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

            //p = xyz_to_vec(pos[i]);

            pos[i] = {p.v[0], p.v[1], p.v[2]};
        }

        auto backup = pos;

        for(int i=0; i<backup.size(); i++)
        {
            cl_float4 mypos = backup[i];

            cl_float4 accum = {0};

            int num = 0;

            for(int j=0; j<connections[i].size(); j++)
            {
                cl_float4 neighbour = backup[connections[i][j]];

                accum = add(accum, neighbour);

                num++;
            }

            mypos = div(add(mypos, accum), num+1);

            pos[i] = mypos;
        }

        for(auto& i : pos)
        {
            i = mult(i, mul);
        }

        saved_pos = pos;
        saved_conn = connections;

        for(auto& i : pos)
        {
            col.push_back(0xFF00FFFF);
        }

        //get_tris(pos, connections);

        positions = compute::buffer(cl::context, sizeof(cl_float4)*saved_pos.size(), CL_MEM_READ_ONLY, nullptr);
        colours = compute::buffer(cl::context, sizeof(cl_uint)*saved_pos.size(), CL_MEM_READ_ONLY, nullptr);


        cl::cqueue.enqueue_write_buffer(positions, 0, sizeof(cl_float4)*saved_pos.size(), &saved_pos[0]);
        cl::cqueue.enqueue_write_buffer(colours, 0, sizeof(cl_uint)*saved_pos.size(), &col[0]);
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

        cl_int nnum = saved_pos.size();

        ///Need to fix the explosion code so I don't have to use this hack
        cl_float frac = 1/40.f;
        cl_float brightness = 1.f;

        arg_list gaussian_args;
        gaussian_args.push_back(&nnum);
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

        run_kernel_with_string("render_gaussian_normal", {nnum}, {128}, 1, gaussian_args);


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
    auto tris = planet.get_tris<5>(planet.saved_pos, planet.saved_conn);

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

    //planet_builder test;
    //test.load();


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

        //test.tick(window);

        window.render_block();
        window.display();

        std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }
}
