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

    template<int N>
    std::array<int, N> get_nearest_n(const std::vector<cl_float4>& pos, int my_id, std::vector<int> exclude)
    {
        cl_float4 my_pos = pos[my_id];

        std::array<float, N> found_distances;
        std::array<int, N> found_nums;

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
                    //printf("hi %i %i %f\n", found_location, i, len);
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
    std::array<int, 8> find_nearest_points(const std::vector<cl_float4>& pos, int my_id, std::vector<int> exclude = std::vector<int>())
    {


        return get_nearest_n<8>(pos, my_id, exclude);

        ///this is built on false assumptions
        /*cl_float4 my_pos = pos[my_id];

        std::array<int, 8> ret;

        ///not ideal, but better than segfaulting
        ///later we'll fix the first and last element
        ret[6] = (my_id - 1 + pos.size()) % pos.size();
        ret[7] = (my_id + 1) % pos.size();

        std::array<float, 2> found_distances;
        std::array<int, 2> found_nums;

        for(auto& i : found_distances)
            i = FLT_MAX;

        for(auto& i : found_nums)
            i = -1;

        for(int i=0; i<pos.size(); i++)
        {
            cl_float4 found = pos[i];

            ///skip the forwards and back elements
            if(i == my_id || i == ret[6] || i == ret[7])
                continue;

            float len = dist(found, my_pos);

            for(int j=0; j<2; j++)
            {
                if(len < found_distances[j])
                {
                    int found_location = j;

                    ///push all elements forwards one
                    for(int k=2-1; k>j; k--)
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

        ///found 2 closest

        ret[0] = found_nums[0];
        ret[1] = found_nums[1];

        ret[2] = (found_nums[0] - 1 + pos.size()) % pos.size();
        ret[3] = (found_nums[0] + 1) % pos.size();

        ret[4] = (found_nums[1] - 1 + pos.size()) % pos.size();
        ret[5] = (found_nums[1] + 1) % pos.size();

        return ret;*/

        /*cl_float4 my_pos = pos[my_id];

        std::array<float, 8> found_distances;
        std::array<int, 8> found_nums;

        for(auto& i : found_distances)
            i = FLT_MAX;

        for(auto& i : found_nums)
            i = -1;

        for(int i=0; i<pos.size(); i++)
        {
            cl_float4 found = pos[i];

            if(i == my_id)
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
        }*/

        /*printf("%i ", my_id);

        for(int i=0; i<8; i++)
        {
            printf("%i ", found_nums[i]);
        }

        printf("\n");*/

        //return found_nums;
    }

    //std::vector<triangle get_tris

    #if 0
    std::vector<triangle> get_tris(const std::vector<cl_float4>& pos, std::vector<std::array<int, 8>> connections)
    {
        std::vector<int> visited;

        ///fill with 0s
        visited.resize(pos.size());

        std::vector<triangle> tris;

        std::vector<int> ref_count;
        ref_count.resize(pos.size());

        for(int i=0; i<pos.size(); i++)
        {
            cl_float4 my_pos = pos[i];
            std::array<int, 8> my_connections = connections[i];

            for(auto& j : my_connections)
            {
                ref_count[j]++;
            }
        }

        for(int i=0; i<pos.size(); i++)
        {
            int closest_id = -1;
            float closest_len = FLT_MAX;

            cl_float4 my_pos = pos[i];

            if(ref_count[i] > 8)
            {
                for(int j=0; j<pos.size(); j++)
                {
                    if(ref_count[j] < 8)
                    {
                        cl_float4 val = pos[j];

                        float len = dist(val, my_pos);

                        if(len < closest_len)
                        {
                            closest_len = len;
                            closest_id = j;
                        }
                    }
                }

                ///we need to find the point for who adopting this
                ///would cause minimal change
                //connections[i][]
            }
        }

        /*for(int i=0; i<ref_count.size(); i++)
        {
            printf("%i\n", ref_count[i]);
        }*/

    }
    #endif

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

            /*std::vector<std::pair<float, int>> connection_angles;

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
                      );*/

            if(visited[i])
                continue;

            std::vector<vec3f> in_connections;

            //for(auto& j : my_connections)
            for(int j=0; j<8; j++)
            {
                int c = my_connections[j];

                in_connections.push_back({pos[c].x, pos[c].y, pos[c].z});
            }

            std::vector<vec3f> sorted = sort_anticlockwise(in_connections, {my_pos.x, my_pos.y, my_pos.z});

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

                vec3f flat_normal = generate_flat_normal(xyz_to_vec(tri.vertices[0].get_pos()),
                                                         xyz_to_vec(tri.vertices[1].get_pos()),
                                                         xyz_to_vec(tri.vertices[2].get_pos()));

                tri.vertices[0].set_normal({flat_normal.v[0], flat_normal.v[1], flat_normal.v[2]});
                tri.vertices[1].set_normal({flat_normal.v[0], flat_normal.v[1], flat_normal.v[2]});
                tri.vertices[2].set_normal({flat_normal.v[0], flat_normal.v[1], flat_normal.v[2]});

                visited[my_connections[j]] = 1;

                tris.push_back(tri);

                //printf("%f %f %f\n", tri.vertices[0].get_pos().x, tri.vertices[0].get_pos().y, tri.vertices[0].get_pos().z);
                //printf("%f %f %f\n", tri.vertices[1].get_pos().x, tri.vertices[1].get_pos().y, tri.vertices[1].get_pos().z);
                //printf("%f %f %f\n", tri.vertices[2].get_pos().x, tri.vertices[2].get_pos().y, tri.vertices[2].get_pos().z);
            }

        }

        return tris;
    }

    const std::vector<cl_float4> do_repulse(const std::vector<cl_float4>& pos)
    {
        auto ret = pos;

        for(int i=0; i<pos.size(); i++)
        {
            vec3f mypos = xyz_to_vec(pos[i]);
            vec3f force = {0.f,0,0};
            float rad = mypos.length();

            for(int j=0; j<pos.size(); j++)
            {
                if(i == j)
                    continue;

                vec3f theirpos = xyz_to_vec(pos[j]);

                vec3f to_them = theirpos - mypos;

                float len = to_them.length();

                //if(len < 1.f)
                //    len = 1.f;

                float G = 0.01f;

                force = force - (G * to_them) / len*len*len;
            }

            vec3f fin = mypos + force;

            fin = fin.norm() * rad;

            ret[i] = {fin.v[0], fin.v[1], fin.v[2]};
        }

        return ret;


        #if 0
        for(int i=0; i<pos.size(); i++)
        {
            cl_float4 my_pos = pos[i];
            float rad = length(my_pos);

            cl_float4 change = {0,0,0,0};

            for(int j=0; j<pos.size(); j++)
            {
                if(i == j)
                    continue;

                cl_float4 their_pos = pos[j];

                cl_float4 to_them = sub(their_pos, my_pos);

                float len = length(to_them);

                if(len < 0.1f)
                    len = 0.1f;

                float G = 0.01f;

                change = add(change,
                             neg(
                             div(mult(to_them, G),
                                 len*len*len
                                 )
                             )
                             );
            }

            ret[i] = add(my_pos, change);

            float new_rad = length(ret[i]);

            float diff = rad / new_rad;

            ret[i] = mult(ret[i], diff);
        }
        #endif
    }

    /*std::vector<std::array<int, 8>> fix_connections(const std::vector<cl_float4>& pos, const std::vector<std::array<int, 8>>& connections, float extra_frac = 1.f, float len_frac = 1.f)
    {
        auto lpos = pos;

        ///connections is sorted from closest to furthest;
        auto ret = connections;

        for(int n=0; n<lpos.size(); n++)
        {
            cl_float4 my_pos = lpos[n];

            cl_float4 conn_pos[8];

            for(int i=0; i<8; i++)
            {
                conn_pos[i] = lpos[connections[n][i]];
            }

            std::vector<vec3f> to_sort;

            for(int i=0; i<4; i++)
                to_sort.push_back(xyz_to_vec(conn_pos[i]));

            std::vector<vec3f> sorted = sort_anticlockwise(to_sort, xyz_to_vec(my_pos));

            for(int i=0; i<4; i++)
                conn_pos[i] = {sorted[i].v[0], sorted[i].v[1], sorted[i].v[2]};

            cl_float4 diagonals[4];

            ///assume that the closest 4 points are the non diagonals
            for(int i=0; i<4; i++)
            {
                ///to diagonal
                diagonals[i] = div(add(sub(conn_pos[i], my_pos),
                                       sub(conn_pos[(i + 1) % 4], my_pos)
                                       ), 2.f);

                //printf("%i %f\n", i, length(sub(conn_pos[i], my_pos)));

                float len = length(diagonals[i]);

                ///difference from corner of rectangle to circle
                float extra = sqrtf(2.f*len*len) - len;

                //extra *= 2.f;
                extra *= extra_frac;

                float frac = (extra + len) / len;

                frac *= len_frac;

                ///this is the expected position roughly
                diagonals[i] = mult(diagonals[i], frac);

                //printf("%f\n", length(diagonals[i]));

                diagonals[i] = add(diagonals[i], my_pos);

                float flen = FLT_MAX;
                int id = -1;

                for(int k=0; k<lpos.size(); k++)
                {
                    bool allowed = true;

                    for(int jj=0; jj<4; jj++)
                    {
                        if(k == connections[k][jj])
                            allowed = false;
                    }

                    if(k == n)
                        allowed = false;

                    if(dist(diagonals[i], lpos[k]) < flen && allowed)
                    {
                        flen = dist(diagonals[i], lpos[k]);
                        id = k;
                    }
                }

                ret[n][i + 4] = id;
            }
        }

        return ret;
    }*/

    std::vector<int> get_refcount(const std::vector<std::array<int, 8>>& connections)
    {
        std::vector<int> refc;
        refc.resize(connections.size());

        for(int i=0; i<connections.size(); i++)
        {
            for(auto& j : connections[i])
            {
                refc[j]++;
            }
        }

        return refc;
    }

    std::vector<int> get_ids_who_reference(const std::vector<std::array<int, 8>>& connections, int id)
    {
        std::vector<int> ret;

        for(int i=0; i<connections.size(); i++)
        {
            for(int j=0; j<connections[i].size(); j++)
            {
                if(connections[i][j] == id)
                {
                    ret.push_back(i);
                }
            }
        }

        return ret;
    }

    std::array<int, 8> change_reference(std::array<int, 8> to_change, int original, int change)
    {
        bool found = false;

        for(int j=0; j<8; j++)
        {
            if(to_change[j] == original)
            {
                to_change[j] = change;
                found = true;
            }
        }

        if(!found)
            printf("sadface\n");

        return to_change;
    }

    std::vector<std::array<int, 8>> fix_connections(const std::vector<cl_float4>& pos, const std::vector<std::array<int, 8>>& connections, int tick)
    {
        //return connections;

        auto new_connect = connections;

        std::vector<int> rc = get_refcount(connections);

        /*for(int i=0; i<rc.size(); i++)
        {
            if(rc[i] > 8)
            {
                for(int j=0; j<8; j++)
                    new_connect[i][j] = 0;
            }
        }*/

        int bc = 0;

        std::array<int, 8> blank;

        for(auto& i : blank)
            i = 0;

        /*int lrc = rc.size();

        for(int i=0; i<rc.size(); i++)
        {
            int ncs = new_connect[i].size();

            if((tick % 10) == 9)
                for(int j=0; j<ncs; j++)
                    new_connect[i][j] = rand() % lrc;
        }*/

        for(int i=0; i<rc.size(); i++)
        {
            if(rc[i] > 8)// || rc[i] < 8)
            {
                bc++;

                #if 1
                for(int j=0; j<8; j++)
                {
                    if(rc[connections[i][j]] < 8)
                    {
                        //new_connect[i] = blank;
                        ///someone sharing these two points is wrong
                        ///somebody who references rc[i] shuld instead be referencing
                        ///rc[new_connect[i][j]]
                        ///so if we check everyone who references rc[i]
                        ///and we check of those, who doesn't reference rc[new_connect[i][j]]
                        ///and then we swap the closest

                        auto referencers = get_ids_who_reference(connections, i);



                        /*cl_float4 newpos = pos[connections[i][j]];

                        std::vector<cl_float4> ref_positions;

                        for(auto& k : referencers)
                        {
                            ref_positions.push_back(pos[k]);
                        }

                        int id = -1;
                        float len = FLT_MAX;

                        for(int k=0; k<referencers.size(); k++)
                        {
                            if(dist(ref_positions[k], newpos) < len)
                            {
                                len = dist(ref_positions[k], newpos);
                                id = referencers[k];
                            }
                        }*/
                        #if 0
                        int id = -1;

                        for(auto& k : referencers)
                        {
                            if(rc[k] > 8)
                            {
                                id = k;
                            }
                        }

                        if(id == -1)
                        {
                            //new_connect[connections[i][j]] = find_nearest_points(pos, connections[i][j], {i});
                            //new_connect[i] = find_nearest_points(pos, i, {connections[i][j]});

                            /*int rand_id = rand() % referencers.size();

                            new_connect[referencers[rand_id]] = find_nearest_points(pos, referencers[rand_id], {connections[i][j]});
                            new_connect[connections[i][j]][rc[connections[i][j]]] = referencers[rand_id];*/

                            cl_float4 newpos = pos[connections[i][j]];

                            std::vector<cl_float4> ref_positions;

                            for(auto& k : referencers)
                            {
                                ref_positions.push_back(pos[k]);
                            }

                            int id = -1;
                            float len = FLT_MAX;

                            for(int k=0; k<referencers.size(); k++)
                            {
                                if(dist(ref_positions[k], newpos) < len)
                                {
                                    len = dist(ref_positions[k], newpos);
                                    id = referencers[k];
                                }
                            }

                            //printf(":[\n");
                            continue;
                        }
                        //else
                        //    printf(":]\n");

                        new_connect[i] = find_nearest_points(pos, i, {id});//change_reference(new_connect[i], i, )
                        new_connect[id] = change_reference(connections[id], i, connections[i][j]);
                        #endif
                        //printf("hi\n");
                    }
                }
                #endif
            }
        }



        printf("Error: %i\n", bc);

        //printf("\n\n\n\n");

        ///need to fix the backwards connections too

        /*auto new_rc = get_refcount(new_connect);

        for(auto& i : new_rc)
        {
            if(i > 8)
                printf(":]\n");
        }*/

        return new_connect;
    }


    bool is_locally_well_formed(const std::vector<int>& refc, const std::vector<std::array<int, 8>>& connections, int id)
    {
        if(refc[id] > 8 || refc[id] < 8)
            return false;

        std::array<int, 8> conn = connections[id];

        bool well_formed = true;

        for(int i=0; i<8; i++)
        {
            //well_formed &= (refc[conn[i]] == 8);
            if(refc[conn[i]] != 8)
                well_formed = false;
        }

        return well_formed;
    }

    ///optimally merge two connection structures to produce
    ///one less broken one
    ///this will allow me to bruteforce the asteroids connections
    std::vector<std::array<int, 8>> stable_merge(const std::vector<std::array<int, 8>>& c1, const std::vector<std::array<int, 8>>& c2)
    {
        std::vector<std::array<int, 8>> merged = c1;

        std::vector<int> r1, r2;

        r1 = get_refcount(c1);
        r2 = get_refcount(c2);

        for(int i=0; i<r1.size(); i++)
        {
            if(r1[i] != 8)
            //if(is_locally_well_formed(r2, c2, i) || r1[i] != 8)
            //if(r1[i] != 8)
            //if(!is_locally_well_formed(r1, c1, i))
            {
                //if(is_locally_well_formed(r2, c2, i))
                //if(r2[i] == 8)
                {
                    merged[i] = c2[i];
                }
            }
        }

        return merged;
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


        float mul = 100.f;

        ///need to get latitude and longitude out so i can feed them into random number generators
        ///or actually, just use angles, yz, xz
        ///maybe I should do one iteration of electron repulsion on this?
        ///then again, that ruins all my guarantees
        for(int i=0; i<num; i++)
        {
            pos.push_back(get_fibonnacci_position(i, num));
            col.push_back(0xFF00FFFF);
        }

        /*for(int i=0; i<10; i++)
        {
            auto ret = do_repulse(pos);

            std::swap(ret, pos);
        }*/

        for(int i=0; i<num; i++)
        {
            connections.push_back(find_nearest_points(pos, i));
        }

        for(int i=0; i<100; i++)
            connections = fix_connections(pos, connections, i);

        //connections = stable_merge(connections, fixed_connections);

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
